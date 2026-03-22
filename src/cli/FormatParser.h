#ifndef CLI_FORMATPARSER_H
#define CLI_FORMATPARSER_H

#include <QString>
#include <optional>

#include "worker/AudioIO.h"

namespace cli {

// Special format mode keywords
enum class FormatMode {
    Explicit, // User provided a concrete value
    Auto,     // "auto" — detect max from input (combine only)
    AsSrc,    // "as-src" — inherit from combined file (extract only)
    AsInput,  // "as-input" — inherit per-file from desc metadata (extract only)
};

struct FormatField {
    FormatMode mode = FormatMode::Explicit;

    // Only meaningful when mode == Explicit
    double sampleRate = 0;
    kfr::audio_sample_type sampleType = kfr::audio_sample_type::i16;
    int channelCount = 0;
    AudioIO::AudioFormat::Container container = AudioIO::AudioFormat::Container::RIFF;
};

struct ParsedFormat {
    FormatField ar;   // -ar
    FormatField sf;   // -sf / --sample-format
    FormatField ac;   // -ac
    FormatField f;    // -f (container)
};

// Parse sample rate string. Returns nullopt on invalid input.
std::optional<FormatField> parseSampleRate(const QString &value);

// Parse sample format string (s16/s24/s32/f32/f64). Returns nullopt on invalid input.
std::optional<FormatField> parseSampleFormat(const QString &value);

// Parse channel count string. Returns nullopt on invalid input.
std::optional<FormatField> parseChannelCount(const QString &value);

// Parse container format string (riff/rf64/w64/flac). Returns nullopt on invalid input.
std::optional<FormatField> parseContainerFormat(const QString &value);

// Validate FLAC + float combination; returns true if adjusted (caller should warn).
bool adjustFlacFloatConflict(ParsedFormat &fmt);

} // namespace cli

#endif // CLI_FORMATPARSER_H
