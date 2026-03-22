#include <QTest>
#include <cli/FormatParser.h>

using namespace cli;

class TestFormatParser : public QObject
{
    Q_OBJECT

private slots:
    // parseSampleRate
    void parseSampleRate_explicit();
    void parseSampleRate_modes();
    void parseSampleRate_caseInsensitive();
    void parseSampleRate_invalid();

    // parseSampleFormat
    void parseSampleFormat_explicit();
    void parseSampleFormat_modes();
    void parseSampleFormat_invalid();

    // parseChannelCount
    void parseChannelCount_explicit();
    void parseChannelCount_modes();
    void parseChannelCount_invalid();

    // parseContainerFormat
    void parseContainerFormat_explicit();
    void parseContainerFormat_modes();
    void parseContainerFormat_invalid();

    // adjustFlacFloatConflict
    void adjustFlacFloat_triggers();
    void adjustFlacFloat_noTrigger();
};

// --- parseSampleRate ---

void TestFormatParser::parseSampleRate_explicit()
{
    auto result = parseSampleRate("44100");
    QVERIFY(result.has_value());
    QCOMPARE(result->mode, FormatMode::Explicit);
    QCOMPARE(result->sampleRate, 44100.0);

    result = parseSampleRate("48000.5");
    QVERIFY(result.has_value());
    QCOMPARE(result->mode, FormatMode::Explicit);
    QCOMPARE(result->sampleRate, 48000.5);

    result = parseSampleRate("22050");
    QVERIFY(result.has_value());
    QCOMPARE(result->sampleRate, 22050.0);
}

void TestFormatParser::parseSampleRate_modes()
{
    auto result = parseSampleRate("auto");
    QVERIFY(result.has_value());
    QCOMPARE(result->mode, FormatMode::Auto);

    result = parseSampleRate("as-src");
    QVERIFY(result.has_value());
    QCOMPARE(result->mode, FormatMode::AsSrc);

    result = parseSampleRate("as-input");
    QVERIFY(result.has_value());
    QCOMPARE(result->mode, FormatMode::AsInput);
}

void TestFormatParser::parseSampleRate_caseInsensitive()
{
    QCOMPARE(parseSampleRate("AUTO")->mode, FormatMode::Auto);
    QCOMPARE(parseSampleRate("As-Src")->mode, FormatMode::AsSrc);
    QCOMPARE(parseSampleRate("AS-INPUT")->mode, FormatMode::AsInput);
}

void TestFormatParser::parseSampleRate_invalid()
{
    QVERIFY(!parseSampleRate("").has_value());
    QVERIFY(!parseSampleRate("abc").has_value());
    QVERIFY(!parseSampleRate("-100").has_value());
    QVERIFY(!parseSampleRate("0").has_value());
}

// --- parseSampleFormat ---

void TestFormatParser::parseSampleFormat_explicit()
{
    auto r = parseSampleFormat("s16");
    QVERIFY(r.has_value());
    QCOMPARE(r->mode, FormatMode::Explicit);
    QCOMPARE(r->sampleType, kfr::audio_sample_type::i16);

    QCOMPARE(parseSampleFormat("s24")->sampleType, kfr::audio_sample_type::i24);
    QCOMPARE(parseSampleFormat("s32")->sampleType, kfr::audio_sample_type::i32);
    QCOMPARE(parseSampleFormat("f32")->sampleType, kfr::audio_sample_type::f32);
    QCOMPARE(parseSampleFormat("f64")->sampleType, kfr::audio_sample_type::f64);

    // Case insensitive
    QCOMPARE(parseSampleFormat("S16")->sampleType, kfr::audio_sample_type::i16);
    QCOMPARE(parseSampleFormat("F32")->sampleType, kfr::audio_sample_type::f32);
}

void TestFormatParser::parseSampleFormat_modes()
{
    QCOMPARE(parseSampleFormat("auto")->mode, FormatMode::Auto);
    QCOMPARE(parseSampleFormat("as-src")->mode, FormatMode::AsSrc);
    QCOMPARE(parseSampleFormat("as-input")->mode, FormatMode::AsInput);
}

void TestFormatParser::parseSampleFormat_invalid()
{
    QVERIFY(!parseSampleFormat("").has_value());
    QVERIFY(!parseSampleFormat("s8").has_value());
    QVERIFY(!parseSampleFormat("i16").has_value());
    QVERIFY(!parseSampleFormat("xyz").has_value());
}

// --- parseChannelCount ---

void TestFormatParser::parseChannelCount_explicit()
{
    auto r = parseChannelCount("1");
    QVERIFY(r.has_value());
    QCOMPARE(r->mode, FormatMode::Explicit);
    QCOMPARE(r->channelCount, 1);

    QCOMPARE(parseChannelCount("2")->channelCount, 2);
    QCOMPARE(parseChannelCount("8")->channelCount, 8);
}

void TestFormatParser::parseChannelCount_modes()
{
    QCOMPARE(parseChannelCount("auto")->mode, FormatMode::Auto);
    QCOMPARE(parseChannelCount("as-src")->mode, FormatMode::AsSrc);
    QCOMPARE(parseChannelCount("as-input")->mode, FormatMode::AsInput);
}

void TestFormatParser::parseChannelCount_invalid()
{
    QVERIFY(!parseChannelCount("").has_value());
    QVERIFY(!parseChannelCount("0").has_value());
    QVERIFY(!parseChannelCount("-1").has_value());
    QVERIFY(!parseChannelCount("abc").has_value());
}

// --- parseContainerFormat ---

void TestFormatParser::parseContainerFormat_explicit()
{
    auto r = parseContainerFormat("riff");
    QVERIFY(r.has_value());
    QCOMPARE(r->mode, FormatMode::Explicit);
    QCOMPARE(r->container, AudioIO::AudioFormat::Container::RIFF);

    // "wav" is an alias for RIFF
    QCOMPARE(parseContainerFormat("wav")->container, AudioIO::AudioFormat::Container::RIFF);
    QCOMPARE(parseContainerFormat("rf64")->container, AudioIO::AudioFormat::Container::RF64);
    QCOMPARE(parseContainerFormat("w64")->container, AudioIO::AudioFormat::Container::W64);
    QCOMPARE(parseContainerFormat("flac")->container, AudioIO::AudioFormat::Container::FLAC);

    // Case insensitive
    QCOMPARE(parseContainerFormat("FLAC")->container, AudioIO::AudioFormat::Container::FLAC);
    QCOMPARE(parseContainerFormat("Riff")->container, AudioIO::AudioFormat::Container::RIFF);
}

void TestFormatParser::parseContainerFormat_modes()
{
    QCOMPARE(parseContainerFormat("auto")->mode, FormatMode::Auto);
    QCOMPARE(parseContainerFormat("as-src")->mode, FormatMode::AsSrc);
    QCOMPARE(parseContainerFormat("as-input")->mode, FormatMode::AsInput);
}

void TestFormatParser::parseContainerFormat_invalid()
{
    QVERIFY(!parseContainerFormat("").has_value());
    QVERIFY(!parseContainerFormat("mp3").has_value());
    QVERIFY(!parseContainerFormat("aac").has_value());
}

// --- adjustFlacFloatConflict ---

void TestFormatParser::adjustFlacFloat_triggers()
{
    ParsedFormat fmt;
    fmt.f.mode = FormatMode::Explicit;
    fmt.f.container = AudioIO::AudioFormat::Container::FLAC;
    fmt.sf.mode = FormatMode::Explicit;
    fmt.sf.sampleType = kfr::audio_sample_type::f32;

    bool adjusted = adjustFlacFloatConflict(fmt);
    QVERIFY(adjusted);
    QVERIFY(fmt.sf.sampleType != kfr::audio_sample_type::f32);
}

void TestFormatParser::adjustFlacFloat_noTrigger()
{
    // RIFF + f32 -> no conflict
    {
        ParsedFormat fmt;
        fmt.f.mode = FormatMode::Explicit;
        fmt.f.container = AudioIO::AudioFormat::Container::RIFF;
        fmt.sf.mode = FormatMode::Explicit;
        fmt.sf.sampleType = kfr::audio_sample_type::f32;
        QVERIFY(!adjustFlacFloatConflict(fmt));
        QCOMPARE(fmt.sf.sampleType, kfr::audio_sample_type::f32);
    }

    // FLAC + i16 -> no conflict
    {
        ParsedFormat fmt;
        fmt.f.mode = FormatMode::Explicit;
        fmt.f.container = AudioIO::AudioFormat::Container::FLAC;
        fmt.sf.mode = FormatMode::Explicit;
        fmt.sf.sampleType = kfr::audio_sample_type::i16;
        QVERIFY(!adjustFlacFloatConflict(fmt));
    }

    // Non-Explicit mode -> no conflict
    {
        ParsedFormat fmt;
        fmt.f.mode = FormatMode::Explicit;
        fmt.f.container = AudioIO::AudioFormat::Container::FLAC;
        fmt.sf.mode = FormatMode::Auto;
        fmt.sf.sampleType = kfr::audio_sample_type::f32;
        QVERIFY(!adjustFlacFloatConflict(fmt));
    }
}

QTEST_MAIN(TestFormatParser)
#include "test_format_parser.moc"
