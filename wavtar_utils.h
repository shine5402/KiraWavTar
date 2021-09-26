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

#include <kfr/io.hpp>
namespace wavtar_defines {
    using sample_process_t = float;
    using sample_o_t = int16_t;
    constexpr auto sample_type_o = kfr::audio_sample_type::i16;
    constexpr auto sample_rate_o = 44100;
    constexpr kfr::audio_format output_format{1, sample_type_o, sample_rate_o, false};

    constexpr auto desc_file_version = 2;
}

#endif // WAVTAR_UTILS_H
