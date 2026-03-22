#include "FormatParser.h"

#include "utils/KfrHelper.h"

namespace cli {

std::optional<FormatField> parseSampleRate(const QString &value)
{
    if (value.isEmpty())
        return std::nullopt;

    auto lower = value.toLower();
    if (lower == "auto")
        return FormatField{.mode = FormatMode::Auto};
    if (lower == "as-src")
        return FormatField{.mode = FormatMode::AsSrc};
    if (lower == "as-input")
        return FormatField{.mode = FormatMode::AsInput};

    bool ok = false;
    double rate = value.toDouble(&ok);
    if (!ok || rate <= 0)
        return std::nullopt;

    return FormatField{.mode = FormatMode::Explicit, .sampleRate = rate};
}

std::optional<FormatField> parseSampleFormat(const QString &value)
{
    if (value.isEmpty())
        return std::nullopt;

    auto lower = value.toLower();
    if (lower == "auto")
        return FormatField{.mode = FormatMode::Auto};
    if (lower == "as-src")
        return FormatField{.mode = FormatMode::AsSrc};
    if (lower == "as-input")
        return FormatField{.mode = FormatMode::AsInput};

    static const struct {
        const char *name;
        kfr::audio_sample_type type;
    } map[] = {
        {"s16", kfr::audio_sample_type::i16},
        {"s24", kfr::audio_sample_type::i24},
        {"s32", kfr::audio_sample_type::i32},
        {"f32", kfr::audio_sample_type::f32},
        {"f64", kfr::audio_sample_type::f64},
    };

    for (const auto &entry : map) {
        if (lower == entry.name)
            return FormatField{.mode = FormatMode::Explicit, .sampleType = entry.type};
    }
    return std::nullopt;
}

std::optional<FormatField> parseChannelCount(const QString &value)
{
    if (value.isEmpty())
        return std::nullopt;

    auto lower = value.toLower();
    if (lower == "auto")
        return FormatField{.mode = FormatMode::Auto};
    if (lower == "as-src")
        return FormatField{.mode = FormatMode::AsSrc};
    if (lower == "as-input")
        return FormatField{.mode = FormatMode::AsInput};

    bool ok = false;
    int channels = value.toInt(&ok);
    if (!ok || channels <= 0)
        return std::nullopt;

    return FormatField{.mode = FormatMode::Explicit, .channelCount = channels};
}

std::optional<FormatField> parseContainerFormat(const QString &value)
{
    if (value.isEmpty())
        return std::nullopt;

    auto lower = value.toLower();
    if (lower == "auto")
        return FormatField{.mode = FormatMode::Auto};
    if (lower == "as-src")
        return FormatField{.mode = FormatMode::AsSrc};
    if (lower == "as-input")
        return FormatField{.mode = FormatMode::AsInput};

    static const struct {
        const char *name;
        AudioIO::AudioFormat::Container container;
    } map[] = {
        {"riff", AudioIO::AudioFormat::Container::RIFF},
        {"wav", AudioIO::AudioFormat::Container::RIFF},
        {"rf64", AudioIO::AudioFormat::Container::RF64},
        {"w64", AudioIO::AudioFormat::Container::W64},
        {"flac", AudioIO::AudioFormat::Container::FLAC},
    };

    for (const auto &entry : map) {
        if (lower == entry.name)
            return FormatField{.mode = FormatMode::Explicit, .container = entry.container};
    }
    return std::nullopt;
}

bool adjustFlacFloatConflict(ParsedFormat &fmt)
{
    if (fmt.f.mode == FormatMode::Explicit && fmt.f.container == AudioIO::AudioFormat::Container::FLAC &&
        fmt.sf.mode == FormatMode::Explicit) {
        auto adjusted = kfr::to_flac_compatible_sample_type(fmt.sf.sampleType);
        if (adjusted != fmt.sf.sampleType) {
            fmt.sf.sampleType = adjusted;
            return true;
        }
    }
    return false;
}

} // namespace cli
