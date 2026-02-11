#include "Utils.h"

#include <QDir>
#include <QStack>
#include <QStackedWidget>
#include <QThread>
#include <atomic>
#include <exception>

namespace utils {

static std::atomic<int> g_sampleRateConversionQuality{
    static_cast<int>(kfr::sample_rate_conversion_quality::normal),
};

kfr::sample_rate_conversion_quality getSampleRateConversionQuality()
{
    return static_cast<kfr::sample_rate_conversion_quality>(g_sampleRateConversionQuality.load());
}

void setSampleRateConversionQuality(kfr::sample_rate_conversion_quality quality)
{
    g_sampleRateConversionQuality.store(static_cast<int>(quality));
}

static std::atomic<int> g_maxLiveTokens{0};

int getMaxLiveTokens()
{
    return g_maxLiveTokens.load();
}

void setMaxLiveTokens(int value)
{
    g_maxLiveTokens.store(value);
}

int effectiveMaxLiveTokens()
{
    int v = g_maxLiveTokens.load();
    if (v <= 0)
        return std::clamp(QThread::idealThreadCount(), 2, 8);
    return v;
}

QString getDescFileNameFrom(const QString &WAVFileName)
{
    auto fileInfo = QFileInfo(WAVFileName);
    return fileInfo.absoluteDir().absoluteFilePath(fileInfo.completeBaseName() + ".kirawavtar-desc.json");
}

QString getVolumeFileName(const QString &baseWavFileName, int volumeIndex)
{
    if (volumeIndex == 0)
        return baseWavFileName;
    auto fileInfo = QFileInfo(baseWavFileName);
    auto dir = fileInfo.absoluteDir();
    auto baseName = fileInfo.completeBaseName();
    auto suffix = fileInfo.suffix();
    return dir.absoluteFilePath(QString("%1.%2.%3").arg(baseName).arg(volumeIndex, 3, 10, QChar('0')).arg(suffix));
}

} // namespace utils
