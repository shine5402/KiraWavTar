#ifndef WAVTAR_UTILS_H
#define WAVTAR_UTILS_H

#include <QStringList>

namespace wavtar_utils {
    QStringList getAbsoluteWAVFileNamesUnder(QString rootDirName, bool recursive = false);
    QString getDescFileNameFrom(const QString& WAVFileName);

    template <typename T>
    QString encodeBase64(const T& value){
        return QString::fromUtf8(QByteArray::fromRawData((const char*)&value, sizeof (T)).toBase64());
    }

    template <typename T>
    T decodeBase64(const QString& base64){
        if (base64.isEmpty())
            return {};
        return *((const T*) QByteArray::fromBase64(base64.toUtf8()).data());
    }
}

#include <kfr/all.hpp>
namespace wavtar_defines {
    //TODO: make these customizeable
    constexpr auto sample_process_type = kfr::audio_sample_type::f32;
    using sample_process_t = kfr::audio_sample_get_type<sample_process_type>::type;
    constexpr auto sample_rate_conversion_quality_for_process = kfr::sample_rate_conversion_quality::normal;

    constexpr auto desc_file_version = 2;

    constexpr auto reportTextStyle = R"(<style>
.critical{
    color:red;
}
.warning{
    color:orange;
}
</style>)";
}

#endif // WAVTAR_UTILS_H
