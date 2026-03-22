#include "ExtractCommand.h"

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <print>

#include "CliUtils.h"
#include "FormatParser.h"
#include "ProgressReporter.h"
#include "utils/Utils.h"
#include "worker/AudioExtract.h"
#include "worker/AudioIO.h"

// TBB's profiling.h defines void emit() which conflicts with Qt's 'emit' macro
#pragma push_macro("emit")
#undef emit
#include <oneapi/tbb/task_group.h>
#pragma pop_macro("emit")

extern oneapi::tbb::task_group_context *g_tbbContext;

namespace cli {

static void printExtractHelp()
{
    std::println("Usage: kirawavtar-cli extract [options] <input-file> <output-folder>");
    std::println("");
    std::println("Extract audio files from a combined file using its description JSON.");
    std::println("");
    std::println("Format options (default: as-input = restore original per-file format):");
    std::println("  -ar <rate|as-src|as-input>         Sample rate");
    std::println("  -sf <fmt|as-src|as-input>          Sample format");
    std::println("  -ac <channels|as-src|as-input>     Channel count");
    std::println("  -f <format|as-src|as-input>        Container format");
    std::println("");
    std::println("Extract options:");
    std::println("  --desc-file <path>                 Path to description JSON");
    std::println("  --filter <glob>                    Filter entries by filename pattern");
    std::println("  --remove-dc-offset                 Apply DC offset removal");
    std::println("  --gap-mode <original|include>      Gap handling (default: original)");
    std::println("  --dry-run                          Preview extraction layout");
    std::println("  -h, --help                         Print this help");
}


// Convert a simple glob pattern to a QRegularExpression
// Supports * and ? wildcards, and ** for recursive matching
static QRegularExpression globToRegex(const QString &glob)
{
    QString pattern;
    int i = 0;
    while (i < glob.size()) {
        QChar c = glob[i];
        if (c == '*') {
            if (i + 1 < glob.size() && glob[i + 1] == '*') {
                pattern += ".*";
                i += 2;
                // Skip trailing / after **
                if (i < glob.size() && glob[i] == '/')
                    i++;
                continue;
            }
            pattern += "[^/]*";
        } else if (c == '?') {
            pattern += "[^/]";
        } else if (c == '.' || c == '(' || c == ')' || c == '[' || c == ']' ||
                   c == '{' || c == '}' || c == '+' || c == '^' || c == '$' || c == '|') {
            pattern += '\\';
            pattern += c;
        } else {
            pattern += c;
        }
        i++;
    }
    return QRegularExpression("^" + pattern + "$", QRegularExpression::CaseInsensitiveOption);
}

int runExtract(const QStringList &args)
{
    QString inputFile;
    QString outputFolder;
    QString descFile;
    ParsedFormat fmt;
    fmt.ar.mode = FormatMode::AsInput;
    fmt.sf.mode = FormatMode::AsInput;
    fmt.ac.mode = FormatMode::AsInput;
    fmt.f.mode = FormatMode::AsInput;

    QString filterGlob;
    bool removeDCOffset = false;
    AudioExtract::ExtractGapMode gapMode = AudioExtract::ExtractGapMode::OriginalRange;
    bool dryRun = false;
    bool quiet = false;

    QStringList positional;
    for (int i = 0; i < args.size(); ++i) {
        const auto &arg = args[i];

        if (arg == "-h" || arg == "--help") {
            printExtractHelp();
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
        } else if (arg == "--desc-file") {
            descFile = nextArg();
            if (descFile.isEmpty()) return 2;
        } else if (arg == "--filter") {
            filterGlob = nextArg();
            if (filterGlob.isEmpty()) return 2;
        } else if (arg == "--remove-dc-offset") {
            removeDCOffset = true;
        } else if (arg == "--gap-mode") {
            auto val = nextArg();
            if (val.isEmpty()) return 2;
            if (val == "original")
                gapMode = AudioExtract::ExtractGapMode::OriginalRange;
            else if (val == "include")
                gapMode = AudioExtract::ExtractGapMode::IncludeSpace;
            else {
                std::println(stderr, "Error: invalid gap mode '{}'. Use: original, include", val.toStdString());
                return 2;
            }
        } else if (arg == "--dry-run") {
            dryRun = true;
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
        std::println(stderr, "Error: expected <input-file> <output-folder>");
        std::println(stderr, "Run 'kirawavtar-cli extract --help' for usage.");
        return 2;
    }

    inputFile = QDir::current().absoluteFilePath(positional[0]);
    outputFolder = QDir(positional[1]).absolutePath();

    // Validate FLAC + float conflict
    if (adjustFlacFloatConflict(fmt)) {
        std::print(stderr, "Warning: FLAC does not support float sample formats; ");
        std::println(stderr, "auto-converted to integer equivalent.");
    }

    // Pre-check
    if (!quiet)
        std::println(stderr, "Running pre-check...");

    auto checkResult = AudioExtract::preCheck(inputFile, outputFolder, descFile);

    if (checkResult.pass == AudioExtract::CheckPassType::CRITICAL) {
        std::println(stderr, "Error: pre-check failed:");
        std::println(stderr, "{}", stripHtml(checkResult.reportString).toStdString());
        return 3;
    }

    if (checkResult.pass == AudioExtract::CheckPassType::WARNING) {
        std::println(stderr, "Warning:");
        std::println(stderr, "{}", stripHtml(checkResult.reportString).toStdString());
    }

    auto descRoot = checkResult.descRoot;
    auto descArray = descRoot["descriptions"].toArray();
    auto gapDurationTimecode = descRoot["gap_duration"].toString();

    // Apply filter
    QJsonArray filteredDescArray;
    if (!filterGlob.isEmpty()) {
        auto regex = globToRegex(filterGlob);
        for (const auto &val : descArray) {
            auto obj = val.toObject();
            auto fileName = obj["file_name"].toString();
            if (regex.match(fileName).hasMatch())
                filteredDescArray.append(obj);
        }
        if (filteredDescArray.isEmpty()) {
            std::println(stderr, "Error: no entries match filter '{}'", filterGlob.toStdString());
            return 3;
        }
        if (!quiet)
            std::println(stderr, "Filter matched {}/{} entries.", filteredDescArray.size(), descArray.size());
    } else {
        filteredDescArray = descArray;
    }

    // Dry run
    if (dryRun) {
        std::println("Dry run — extract layout:");
        std::println("  Input file:     {}", inputFile.toStdString());
        std::println("  Output folder:  {}", outputFolder.toStdString());
        std::println("  Entries:        {}/{}", filteredDescArray.size(), descArray.size());
        std::println("");

        for (const auto &val : filteredDescArray) {
            auto obj = val.toObject();
            auto fileName = obj["file_name"].toString();
            auto duration = obj["duration"].toString();
            auto beginTime = obj["begin_time"].toString();
            std::println("  {} (begin: {}, duration: {})", fileName.toStdString(),
                         beginTime.toStdString(), duration.toStdString());
        }
        return 0;
    }

    // Build target format
    std::optional<AudioIO::AudioFormat> targetFormat;

    bool anyExplicitOrAsSrc = false;
    for (const auto *field : {&fmt.ar, &fmt.sf, &fmt.ac, &fmt.f}) {
        if (field->mode == FormatMode::Explicit || field->mode == FormatMode::AsSrc)
            anyExplicitOrAsSrc = true;
    }

    if (anyExplicitOrAsSrc) {
        // Read source format once — needed for as-src and as-input fallback
        AudioIO::AudioFormat srcFormat;
        bool hasSrcFormat = false;
        try {
            srcFormat = AudioIO::readAudioFormat(inputFile);
            hasSrcFormat = true;
        } catch (const std::exception &e) {
            bool needSrc = (fmt.ar.mode == FormatMode::AsSrc) || (fmt.sf.mode == FormatMode::AsSrc) ||
                           (fmt.ac.mode == FormatMode::AsSrc) || (fmt.f.mode == FormatMode::AsSrc);
            if (needSrc) {
                std::println(stderr, "Error: failed to read source format: {}", e.what());
                return 1;
            }
        }

        // Start from source format, then override per field
        AudioIO::AudioFormat format = hasSrcFormat ? srcFormat : AudioIO::AudioFormat{};

        if (fmt.ar.mode == FormatMode::Explicit)
            format.kfr_format.samplerate = fmt.ar.sampleRate;
        if (fmt.sf.mode == FormatMode::Explicit)
            format.kfr_format.type = fmt.sf.sampleType;
        if (fmt.ac.mode == FormatMode::Explicit)
            format.kfr_format.channels = fmt.ac.channelCount;
        if (fmt.f.mode == FormatMode::Explicit)
            format.container = fmt.f.container;

        // For as-input fields when source format couldn't be read, fall back to desc root
        if (!hasSrcFormat) {
            if (fmt.ar.mode == FormatMode::AsInput)
                format.kfr_format.samplerate = descRoot["sample_rate"].toDouble();
            if (fmt.sf.mode == FormatMode::AsInput)
                format.kfr_format.type = static_cast<kfr::audio_sample_type>(descRoot["sample_type"].toInt());
            if (fmt.ac.mode == FormatMode::AsInput)
                format.kfr_format.channels = descRoot["channel_count"].toInt();
        }

        targetFormat = format;
    }
    // If all fields are as-input, targetFormat stays nullopt (pipeline inherits per-entry)

    // Run pipeline
    AudioExtract::ExtractPipelineParams params;
    params.srcWAVFileName = inputFile;
    params.descRoot = descRoot;
    params.dstDirName = outputFolder;
    params.targetFormat = targetFormat;
    params.removeDCOffset = removeDCOffset;
    params.gapMode = gapMode;
    params.gapDurationTimecode = gapDurationTimecode;
    params.filteredDescArray = filteredDescArray;

    std::atomic<int> progress{0};
    oneapi::tbb::task_group_context ctx;
    g_tbbContext = &ctx;

    ProgressReporter reporter(progress, filteredDescArray.size(), quiet);
    reporter.start();

    try {
        auto result = AudioExtract::runExtractPipeline(params, progress, ctx);
        reporter.finish();

        if (!result.errors.isEmpty()) {
            std::println(stderr, "Extraction completed with {} error(s):", result.errors.size());
            for (const auto &err : result.errors) {
                auto fileName = err.descObj["file_name"].toString();
                std::println(stderr, "  {}: {}", fileName.toStdString(), err.description.toStdString());
            }
            return 1;
        }

        if (!quiet)
            std::println(stderr, "Extraction complete. {} file(s) written to {}",
                         filteredDescArray.size(), outputFolder.toStdString());
        return 0;
    } catch (const std::exception &e) {
        reporter.finish();
        std::println(stderr, "Error: {}", e.what());
        return 1;
    } catch (...) {
        reporter.finish();
        std::println(stderr, "Error: unknown error occurred during extraction");
        return 1;
    }
}

} // namespace cli
