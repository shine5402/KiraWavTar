#include <QTest>
#include <utils/KfrHelper.h>

class TestSampleTypes : public QObject
{
    Q_OBJECT

private slots:
    void audio_sample_type_to_string_returnsValid();
    void audio_sample_type_to_string_returnsEmptyForUnknown();
    void audio_sample_type_to_precision_returnsCorrect();
    void audio_sample_type_to_precision_returnsZeroForUnknown();
    void to_flac_compatible_sample_type_keepsIntegers();
    void to_flac_compatible_sample_type_convertsFloats();
    void audio_sample_type_entries_containsExpectedTypes();
    void audio_sample_type_entries_for_flac_containsOnlyIntegers();
};

void TestSampleTypes::audio_sample_type_to_string_returnsValid()
{
    QVERIFY(kfr::audio_sample_type_to_string(kfr::audio_sample_type::i16).size() > 0);
    QVERIFY(kfr::audio_sample_type_to_string(kfr::audio_sample_type::i24).size() > 0);
    QVERIFY(kfr::audio_sample_type_to_string(kfr::audio_sample_type::i32).size() > 0);
    QVERIFY(kfr::audio_sample_type_to_string(kfr::audio_sample_type::f32).size() > 0);
    QVERIFY(kfr::audio_sample_type_to_string(kfr::audio_sample_type::f64).size() > 0);
}

void TestSampleTypes::audio_sample_type_to_string_returnsEmptyForUnknown()
{
    QCOMPARE(kfr::audio_sample_type_to_string(kfr::audio_sample_type::unknown).size(), 0ULL);
}

void TestSampleTypes::audio_sample_type_to_precision_returnsCorrect()
{
    QCOMPARE(kfr::audio_sample_type_to_precision(kfr::audio_sample_type::i16), 16);
    QCOMPARE(kfr::audio_sample_type_to_precision(kfr::audio_sample_type::i24), 24);
    QCOMPARE(kfr::audio_sample_type_to_precision(kfr::audio_sample_type::i32), 32);
    QCOMPARE(kfr::audio_sample_type_to_precision(kfr::audio_sample_type::f32), 24);
    QCOMPARE(kfr::audio_sample_type_to_precision(kfr::audio_sample_type::f64), 53);
}

void TestSampleTypes::audio_sample_type_to_precision_returnsZeroForUnknown()
{
    QCOMPARE(kfr::audio_sample_type_to_precision(kfr::audio_sample_type::unknown), 0);
}

void TestSampleTypes::to_flac_compatible_sample_type_keepsIntegers()
{
    QCOMPARE(kfr::to_flac_compatible_sample_type(kfr::audio_sample_type::i16),
             kfr::audio_sample_type::i16);
    QCOMPARE(kfr::to_flac_compatible_sample_type(kfr::audio_sample_type::i24),
             kfr::audio_sample_type::i24);
    QCOMPARE(kfr::to_flac_compatible_sample_type(kfr::audio_sample_type::i32),
             kfr::audio_sample_type::i32);
}

void TestSampleTypes::to_flac_compatible_sample_type_convertsFloats()
{
    QCOMPARE(kfr::to_flac_compatible_sample_type(kfr::audio_sample_type::f32),
             kfr::audio_sample_type::i32);
    QCOMPARE(kfr::to_flac_compatible_sample_type(kfr::audio_sample_type::f64),
             kfr::audio_sample_type::i32);
}

void TestSampleTypes::audio_sample_type_entries_containsExpectedTypes()
{
    bool hasI16 = false, hasI24 = false, hasI32 = false, hasF32 = false, hasF64 = false;

    for (const auto &[type, str] : kfr::audio_sample_type_entries) {
        if (type == kfr::audio_sample_type::i16)
            hasI16 = true;
        if (type == kfr::audio_sample_type::i24)
            hasI24 = true;
        if (type == kfr::audio_sample_type::i32)
            hasI32 = true;
        if (type == kfr::audio_sample_type::f32)
            hasF32 = true;
        if (type == kfr::audio_sample_type::f64)
            hasF64 = true;
    }

    QVERIFY(hasI16);
    QVERIFY(hasI24);
    QVERIFY(hasI32);
    QVERIFY(hasF32);
    QVERIFY(hasF64);
}

void TestSampleTypes::audio_sample_type_entries_for_flac_containsOnlyIntegers()
{
    for (const auto &[type, str] : kfr::audio_sample_type_entries_for_flac) {
        QVERIFY(type == kfr::audio_sample_type::i16 || type == kfr::audio_sample_type::i24 ||
                type == kfr::audio_sample_type::i32);
    }
}

QTEST_MAIN(TestSampleTypes)
#include "test_sample_types.moc"
