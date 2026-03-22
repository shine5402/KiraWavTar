#include "CombineCommand.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <print>

#include "CliUtils.h"
#include "FormatParser.h"
#include "ProgressReporter.h"
#include "utils/Filesystem.h"
#include "utils/KfrHelper.h"
#include "utils/Utils.h"
#include "worker/AudioCombine.h"
#include "worker/AudioIO.h"

// TBB's profiling.h defines void emit() which conflicts with Qt's 'emit' macro
#pragma push_macro("emit")
#undef emit
#include <oneapi/tbb/task_group.h>
#pragma pop_macro("emit")

extern oneapi::tbb::task_group_context *g_tbbContext;

namespace cli {

static void printCombineHelp()
{
    std::println("Usage: kirawavtar-cli combine [options] <input-folder> <output-file>");
    std::println("");
    std::println("Combine multiple audio files into a single file with a description JSON.");
    std::println("");
    std::println("Format options (default: auto = max from input):");
    std::println("  -ar <rate|auto>                 Sample rate in Hz");
    std::println("  -sf <s16|s24|s32|f32|f64|auto>  Sample format");
    std::println("  -ac <channels|auto>             Channel count");
    std::println("  -f <riff|rf64|w64|flac>         Container format (default: riff)");
    std::println("");
    std::println("Combine options:");
    std::println("  --non-recursive                 Don't scan subfolders");
    std::println("  --no-desc-file                  Don't write description JSON");
    std::println("  --desc-filename <path>          Write description JSON to custom path");
    std::println("  --gap <ms>                      Silence gap between entries in ms (default: 0)");
    std::println("  --split-by-entries <count>      Split into volumes by entry count");
    std::println("  --split-by-duration <secs>      Split into volumes by max duration");
    std::println("  --dry-run                       Preview layout without writing files");
    std::println("  --json                          With --dry-run: output desc JSON to stdout");
    std::println("  -h, --help                      Print this help");
}


static AudioIO::AudioFormat resolveAutoFormat(
    const ParsedFormat &fmt,
    const QStringList &wavFileNames,
    AudioIO::AudioFormat baseFormat)
{
    bool needScan = (fmt.ar.mode == FormatMode::Auto) ||
                    (fmt.sf.mode == FormatMode::Auto) ||
                    (fmt.ac.mode == FormatMode::Auto);

    if (needScan) {
        double maxSampleRate = 0;
        int maxSampleTypePrecision = 0;
        kfr::audio_sample_type maxSampleType = kfr::audio_sample_type::i16;
        int maxChannels = 1;

        for (const auto &fileName : wavFileNames) {
            try {
                auto format = AudioIO::readAudioFormat(fileName);
                maxSampleRate = std::max(maxSampleRate, format.kfr_format.samplerate);
                int precision = kfr::audio_sample_type_to_precision(format.kfr_format.type);
                if (precision > maxSampleTypePrecision) {
                    maxSampleTypePrecision = precision;
                    maxSampleType = format.kfr_format.type;
                }
                maxChannels = std::max(maxChannels, static_cast<int>(format.kfr_format.channels));
            } catch (...) {
                // Ignore unreadable files — preCheck will catch them
            }
        }

        if (fmt.ar.mode == FormatMode::Auto && maxSampleRate > 0)
            baseFormat.kfr_format.samplerate = maxSampleRate;
        if (fmt.sf.mode == FormatMode::Auto && maxSampleTypePrecision > 0) {
            baseFormat.kfr_format.type = maxSampleType;
            if (baseFormat.isFlac())
                baseFormat.kfr_format.type = kfr::to_flac_compatible_sample_type(baseFormat.kfr_format.type);
        }
        if (fmt.ac.mode == FormatMode::Auto)
            baseFormat.kfr_format.channels = maxChannels;
    }

    // Apply explicit overrides
    if (fmt.ar.mode == FormatMode::Explicit)
        baseFormat.kfr_format.samplerate = fmt.ar.sampleRate;
    if (fmt.sf.mode == FormatMode::Explicit)
        baseFormat.kfr_format.type = fmt.sf.sampleType;
    if (fmt.ac.mode == FormatMode::Explicit)
        baseFormat.kfr_format.channels = fmt.ac.channelCount;
    if (fmt.f.mode == FormatMode::Explicit)
        baseFormat.container = fmt.f.container;

    return baseFormat;
}

int runCombine(const QStringList &args)
{
    // Manual parse since QCommandLineParser doesn't handle -ar style well with subcommands
    // We parse known flags ourselves
    QString inputFolder;
    QString outputFile;
    ParsedFormat fmt;
    fmt.ar.mode = FormatMode::Auto;
    fmt.sf.mode = FormatMode::Auto;
    fmt.ac.mode = FormatMode::Auto;
    fmt.f.mode = FormatMode::Explicit;
    fmt.f.container = AudioIO::AudioFormat::Container::RIFF;

    bool recursive = true;
    bool noDescFile = false;
    QString descFilename;
    int gapMs = 0;
    utils::VolumeConfig volumeConfig;
    bool dryRun = false;
    bool jsonOutput = false;
    bool quiet = false;

    // Parse arguments
    QStringList positional;
    for (int i = 0; i < args.size(); ++i) {
        const auto &arg = args[i];

        if (arg == "-h" || arg == "--help") {
            printCombineHelp();
            return 0;
        }

        auto nextArg = [&]() -> QString {
            if (i + 1 >= args.size()) {
                std::println(stderr, "Error: {} requires a value", arg.toStdString());
                return {};
            }
            return args[++i];
        };

        if (arg == "-ar") {
            auto val = nextArg();
            if (val.isEmpty()) return 2;
            auto parsed = parseSampleRate(val);
            if (!parsed) {
                std::println(stderr, "Error: invalid sample rate '{}'", val.toStdString());
                return 2;
            }
            fmt.ar = *parsed;
        } else if (arg == "-sf" || arg == "--sample-format") {
            auto val = nextArg();
            if (val.isEmpty()) return 2;
            auto parsed = parseSampleFormat(val);
            if (!parsed) {
                std::println(stderr, "Error: invalid sample format '{}'. Use: s16, s24, s32, f32, f64", val.toStdString());
                return 2;
            }
            fmt.sf = *parsed;
        } else if (arg == "-ac") {
            auto val = nextArg();
            if (val.isEmpty()) return 2;
            auto parsed = parseChannelCount(val);
            if (!parsed) {
                std::println(stderr, "Error: invalid channel count '{}'", val.toStdString());
                return 2;
            }
            fmt.ac = *parsed;
        } else if (arg == "-f") {
            auto val = nextArg();
            if (val.isEmpty()) return 2;
            auto parsed = parseContainerFormat(val);
            if (!parsed) {
                std::println(stderr, "Error: invalid container format '{}'. Use: riff, rf64, w64, flac", val.toStdString());
                return 2;
            }
            fmt.f = *parsed;
        } else if (arg == "--non-recursive") {
            recursive = false;
        } else if (arg == "--no-desc-file") {
            noDescFile = true;
        } else if (arg == "--desc-filename") {
            descFilename = nextArg();
            if (descFilename.isEmpty()) return 2;
        } else if (arg == "--gap") {
            auto val = nextArg();
            if (val.isEmpty()) return 2;
            bool ok;
            gapMs = val.toInt(&ok);
            if (!ok || gapMs < 0) {
                std::println(stderr, "Error: invalid gap value '{}' (must be non-negative integer ms)", val.toStdString());
                return 2;
            }
        } else if (arg == "--split-by-entries") {
            auto val = nextArg();
            if (val.isEmpty()) return 2;
            bool ok;
            volumeConfig.maxEntriesPerVolume = val.toInt(&ok);
            if (!ok || volumeConfig.maxEntriesPerVolume <= 0) {
                std::println(stderr, "Error: invalid entry count '{}'", val.toStdString());
                return 2;
            }
            volumeConfig.mode = utils::VolumeSplitMode::ByCount;
        } else if (arg == "--split-by-duration") {
            auto val = nextArg();
            if (val.isEmpty()) return 2;
            bool ok;
            volumeConfig.maxDurationSeconds = val.toInt(&ok);
            if (!ok || volumeConfig.maxDurationSeconds <= 0) {
                std::println(stderr, "Error: invalid duration '{}'", val.toStdString());
                return 2;
            }
            volumeConfig.mode = utils::VolumeSplitMode::ByDuration;
        } else if (arg == "--dry-run") {
            dryRun = true;
        } else if (arg == "--json") {
            jsonOutput = true;
        } else if (arg == "-q" || arg == "--quiet") {
            quiet = true;
        } else if (arg.startsWith("-")) {
            std::println(stderr, "Error: unknown option '{}'", arg.toStdString());
            return 2;
        } else {
            positional.append(arg);
        }
    }

    if (positional.size() != 2) {
        std::println(stderr, "Error: expected <input-folder> <output-file>");
        std::println(stderr, "Run 'kirawavtar-cli combine --help' for usage.");
        return 2;
    }

    inputFolder = QDir(positional[0]).absolutePath();
    outputFile = QDir::current().absoluteFilePath(positional[1]);

    if (noDescFile && !descFilename.isEmpty()) {
        std::println(stderr, "Error: --no-desc-file and --desc-filename are mutually exclusive");
        return 2;
    }

    // Validate FLAC + float conflict
    if (adjustFlacFloatConflict(fmt)) {
        std::print(stderr, "Warning: FLAC does not support float sample formats; ");
        std::println(stderr, "auto-converted to integer equivalent.");
    }

    // Discover input files
    if (!quiet)
        std::println(stderr, "Scanning input files...");

    auto wavFileNames = getAbsoluteAudioFileNamesUnder(inputFolder, recursive);
    if (wavFileNames.isEmpty()) {
        std::println(stderr, "Error: no audio files found in '{}'", inputFolder.toStdString());
        return 3;
    }

    if (!quiet)
        std::println(stderr, "Found {} audio file(s).", wavFileNames.size());

    // Resolve format (handle auto modes)
    AudioIO::AudioFormat targetFormat;
    targetFormat.container = AudioIO::AudioFormat::Container::RIFF; // default
    targetFormat = resolveAutoFormat(fmt, wavFileNames, targetFormat);

    // Pre-check
    auto checkResult = AudioCombine::preCheck(inputFolder, outputFile, recursive, targetFormat);

    if (checkResult.pass == AudioCombine::CheckPassType::CRITICAL) {
        std::println(stderr, "Error: pre-check failed:");
        std::println(stderr, "{}", stripHtml(checkResult.reportString).toStdString());
        return 3;
    }

    if (checkResult.pass == AudioCombine::CheckPassType::WARNING) {
        std::println(stderr, "Warning:");
        std::println(stderr, "{}", stripHtml(checkResult.reportString).toStdString());
    }

    // Compute layout
    auto layout = AudioCombine::computeLayout(
        checkResult.wavFileNames, inputFolder, outputFile, targetFormat, gapMs, volumeConfig);

    // Dry run
    if (dryRun) {
        if (jsonOutput) {
            // Build a preview JSON similar to desc format but without actual processing
            QJsonObject preview;
            preview.insert("version", utils::desc_file_version);
            preview.insert("sample_rate", layout.targetFormat.kfr_format.samplerate);
            preview.insert("sample_type", static_cast<int>(layout.targetFormat.kfr_format.type));
            preview.insert("channel_count", static_cast<int>(layout.targetFormat.kfr_format.channels));
            preview.insert("gap_ms", gapMs);
            preview.insert("entry_count", layout.entries.size());
            preview.insert("volume_count", layout.volumes.size());

            QJsonArray entriesArray;
            for (const auto &entry : layout.entries) {
                QJsonObject obj;
                obj.insert("file_path", entry.relativePath);
                obj.insert("source_sample_rate", entry.sourceFormat.kfr_format.samplerate);
                obj.insert("source_sample_type", static_cast<int>(entry.sourceFormat.kfr_format.type));
                obj.insert("source_channels", static_cast<int>(entry.sourceFormat.kfr_format.channels));
                obj.insert("volume_index", entry.volumeIndex);
                entriesArray.append(obj);
            }
            preview.insert("entries", entriesArray);

            QJsonArray volumesArray;
            for (const auto &vol : layout.volumes) {
                QJsonObject obj;
                obj.insert("output_file", vol.outputFilePath);
                obj.insert("entry_count", vol.entryIndices.size());
                obj.insert("total_frames", vol.totalFrames);
                volumesArray.append(obj);
            }
            preview.insert("volumes", volumesArray);

            QJsonDocument doc(preview);
            std::println("{}", doc.toJson(QJsonDocument::Indented).toStdString());
        } else {
            std::println("Dry run — combine layout:");
            std::println("  Input folder:    {}", inputFolder.toStdString());
            std::println("  Output file:     {}", outputFile.toStdString());
            std::println("  Sample rate:     {} Hz", layout.targetFormat.kfr_format.samplerate);
            std::println("  Sample format:   {}", kfr::audio_sample_type_to_string(layout.targetFormat.kfr_format.type));
            std::println("  Channels:        {}", layout.targetFormat.kfr_format.channels);
            std::println("  Gap:             {} ms", gapMs);
            std::println("  Entries:         {}", layout.entries.size());
            std::println("  Volumes:         {}", layout.volumes.size());
            std::println("");

            for (int v = 0; v < layout.volumes.size(); ++v) {
                const auto &vol = layout.volumes[v];
                if (layout.volumes.size() > 1)
                    std::println("  Volume {} — {}:", v, vol.outputFilePath.toStdString());
                for (int idx : vol.entryIndices) {
                    const auto &entry = layout.entries[idx];
                    std::println("    {}", entry.relativePath.toStdString());
                }
            }
        }
        return 0;
    }

    // Determine desc file override
    std::optional<QString> descFileOverride;
    if (noDescFile) {
        descFileOverride = QString(); // empty = skip writing
    } else if (!descFilename.isEmpty()) {
        descFileOverride = QDir::current().absoluteFilePath(descFilename);
    }
    // else nullopt = default behavior

    // Run pipeline
    std::atomic<int> progress{0};
    oneapi::tbb::task_group_context ctx;
    g_tbbContext = &ctx;

    ProgressReporter reporter(progress, layout.entries.size(), quiet);
    reporter.start();

    try {
        auto descJson = AudioCombine::runCombinePipeline(layout, progress, ctx, descFileOverride);
        reporter.finish();

        if (!quiet) {
            std::println(stderr, "Combine complete.");
            for (const auto &vol : layout.volumes) {
                std::println(stderr, "  Output: {}", vol.outputFilePath.toStdString());
            }
            if (noDescFile) {
                std::println(stderr, "  Description file was not written (--no-desc-file).");
                std::println(stderr, "  Note: extraction will not be possible without a description file.");
            } else if (!descFilename.isEmpty()) {
                std::println(stderr, "  Description: {}", descFilename.toStdString());
            } else {
                auto defaultDesc = utils::getDescFileNameFrom(layout.volumes[0].outputFilePath);
                std::println(stderr, "  Description: {}", defaultDesc.toStdString());
            }
        }
        return 0;
    } catch (const std::exception &e) {
        reporter.finish();
        std::println(stderr, "Error: {}", e.what());
        return 1;
    } catch (...) {
        reporter.finish();
        std::println(stderr, "Error: unknown error occurred during combine");
        return 1;
    }
}

} // namespace cli
