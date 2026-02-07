#ifndef UTILS_H
#define UTILS_H

#include <QCoreApplication>
#include <QFuture>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QtMath>
#include <kfr/all.hpp>
class QStackedWidget;

namespace utils {
QString getDescFileNameFrom(const QString &WAVFileName);

// Convert sample count to timecode string "HH:MM:SS.fff" (millisecond precision)
inline QString samplesToTimecode(qint64 samples, double sampleRate)
{
    double totalSeconds = static_cast<double>(samples) / sampleRate;
    qint64 hours = static_cast<qint64>(totalSeconds) / 3600;
    qint64 minutes = (static_cast<qint64>(totalSeconds) % 3600) / 60;
    double seconds = totalSeconds - hours * 3600 - minutes * 60;
    return QString("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 6, 'f', 3, QChar('0'));
}

// Parse timecode string "HH:MM:SS.fff" back to sample count
inline qint64 timecodeToSamples(const QString &timecode, double sampleRate)
{
    if (timecode.isEmpty())
        return 0;
    auto parts = timecode.split(':');
    if (parts.size() != 3)
        return 0;
    double hours = parts[0].toDouble();
    double minutes = parts[1].toDouble();
    double seconds = parts[2].toDouble();
    double totalSeconds = hours * 3600.0 + minutes * 60.0 + seconds;
    return qRound64(totalSeconds * sampleRate);
}

template <typename T> bool checkFutureExceptionAndWarn(QFuture<T> future)
{
    try {
        future.waitForFinished();
        return true;
    } catch (const std::exception &e) {
        QMessageBox::critical(nullptr, {}, e.what());
        return false;
    } catch (...) {
        QMessageBox::critical(nullptr, {}, QCoreApplication::translate("Utils", "Unknown error occurred."));
        return false;
    }
}

// TODO: make these customizable
constexpr auto sample_process_type = kfr::audio_sample_type::f32;
using sample_process_t = kfr::audio_sample_get_type<sample_process_type>::type;
constexpr auto sample_rate_conversion_quality_for_process = kfr::sample_rate_conversion_quality::normal;

constexpr auto desc_file_version = 3;

constexpr auto reportTextStyle = R"(<style>
.critical{
    color:red;
}
.warning{
    color:orange;
}
</style>)";
} // namespace utils

#endif // UTILS_H
