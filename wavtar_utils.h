#ifndef WAVTAR_UTILS_H
#define WAVTAR_UTILS_H

#include <QStringList>

namespace wavtar_utils {
    QStringList getAbsoluteWAVFileNamesUnder(QString rootDirName, bool recursive = false);
    QString getDescFileNameFrom(const QString& WAVFileName);
}

#include <kfr/io.hpp>
namespace wavtar_defines {
    using sample_process_t = float;
    using sample_o_t = int16_t;
    constexpr auto sample_type_o = kfr::audio_sample_type::i16;
    constexpr auto sample_rate_o = 44100;
    constexpr auto o_w64 = false;
    constexpr auto output_channel_count = 1;
}

#endif // WAVTAR_UTILS_H
