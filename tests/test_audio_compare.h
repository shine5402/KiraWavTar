#ifndef TEST_AUDIO_COMPARE_H
#define TEST_AUDIO_COMPARE_H

#include <QString>
#include <cmath>
#include <kfr/all.hpp>

namespace AudioCompare {

struct ComparisonResult
{
    double ncc;
    double rmse;
    double peakDiff;
    double maxPeakA;
    double maxPeakB;
    size_t comparedSamples;
    size_t lengthDiff;
    int bestOffset;
    bool passed;
    QString message;
};

template <typename T>
ComparisonResult compareAudioBuffers(const kfr::univector2d<T> &a, const kfr::univector2d<T> &b,
                                     double nccThreshold = 0.99, double rmseThreshold = 0.01,
                                     int maxOffsetSearch = 10)
{
    ComparisonResult result{};
    result.ncc = 0.0;
    result.rmse = 0.0;
    result.peakDiff = 0.0;
    result.comparedSamples = 0;
    result.lengthDiff = 0;
    result.bestOffset = 0;
    result.passed = false;

    if (a.size() != b.size()) {
        result.message = QString("Channel count mismatch: %1 vs %2").arg(a.size()).arg(b.size());
        return result;
    }

    if (a.empty()) {
        result.message = "Both buffers are empty";
        result.passed = true;
        return result;
    }

    size_t maxLen = 0;
    size_t minLen = SIZE_MAX;
    for (size_t ch = 0; ch < a.size(); ++ch) {
        maxLen = std::max(maxLen, std::max(a[ch].size(), b[ch].size()));
        minLen = std::min(minLen, std::min(a[ch].size(), b[ch].size()));
    }
    result.lengthDiff = maxLen - minLen;

    auto computeStatsAtOffset = [&](int offset) -> std::pair<double, double> {
        double sumA = 0.0, sumB = 0.0;
        double sumSqDiff = 0.0;
        size_t totalSamples = 0;

        for (size_t ch = 0; ch < a.size(); ++ch) {
            const auto &chA = a[ch];
            const auto &chB = b[ch];
            
            size_t startA = offset >= 0 ? offset : 0;
            size_t startB = offset < 0 ? -offset : 0;
            
            size_t lenA = chA.size() > startA ? chA.size() - startA : 0;
            size_t lenB = chB.size() > startB ? chB.size() - startB : 0;
            size_t compareLen = std::min(lenA, lenB);
            
            for (size_t i = 0; i < compareLen; ++i) {
                double valA = static_cast<double>(chA[startA + i]);
                double valB = static_cast<double>(chB[startB + i]);
                sumA += valA;
                sumB += valB;
                sumSqDiff += (valA - valB) * (valA - valB);
            }
            totalSamples += compareLen;
        }

        if (totalSamples == 0) return {0.0, 1.0};

        double meanA = sumA / totalSamples;
        double meanB = sumB / totalSamples;
        double rmse = std::sqrt(sumSqDiff / totalSamples);

        double varA = 0.0, varB = 0.0, cov = 0.0;
        for (size_t ch = 0; ch < a.size(); ++ch) {
            const auto &chA = a[ch];
            const auto &chB = b[ch];
            
            size_t startA = offset >= 0 ? offset : 0;
            size_t startB = offset < 0 ? -offset : 0;
            
            size_t lenA = chA.size() > startA ? chA.size() - startA : 0;
            size_t lenB = chB.size() > startB ? chB.size() - startB : 0;
            size_t compareLen = std::min(lenA, lenB);
            
            for (size_t i = 0; i < compareLen; ++i) {
                double valA = static_cast<double>(chA[startA + i]);
                double valB = static_cast<double>(chB[startB + i]);
                double devA = valA - meanA;
                double devB = valB - meanB;
                varA += devA * devA;
                varB += devB * devB;
                cov += devA * devB;
            }
        }

        varA /= totalSamples;
        varB /= totalSamples;
        cov /= totalSamples;

        double stdA = std::sqrt(varA);
        double stdB = std::sqrt(varB);

        double ncc = 0.0;
        if (stdA > 1e-10 && stdB > 1e-10) {
            ncc = cov / (stdA * stdB);
        } else if (stdA < 1e-10 && stdB < 1e-10) {
            ncc = 1.0;
        }

        return {ncc, rmse};
    };

    double bestNcc = -2.0;
    double bestRmse = 1.0;
    int bestOffset = 0;

    for (int offset = -maxOffsetSearch; offset <= maxOffsetSearch; ++offset) {
        auto [ncc, rmse] = computeStatsAtOffset(offset);
        if (ncc > bestNcc) {
            bestNcc = ncc;
            bestRmse = rmse;
            bestOffset = offset;
        }
    }

    result.ncc = bestNcc;
    result.rmse = bestRmse;
    result.bestOffset = bestOffset;

    double maxPeakA = 0.0, maxPeakB = 0.0;
    for (size_t ch = 0; ch < a.size(); ++ch) {
        for (size_t i = 0; i < a[ch].size(); ++i) {
            double absVal = std::abs(static_cast<double>(a[ch][i]));
            if (absVal > maxPeakA) maxPeakA = absVal;
        }
        for (size_t i = 0; i < b[ch].size(); ++i) {
            double absVal = std::abs(static_cast<double>(b[ch][i]));
            if (absVal > maxPeakB) maxPeakB = absVal;
        }
    }
    result.maxPeakA = maxPeakA;
    result.maxPeakB = maxPeakB;
    result.peakDiff = std::abs(maxPeakA - maxPeakB);

    bool nccOk = result.ncc >= nccThreshold;
    bool rmseOk = result.rmse <= rmseThreshold;
    result.passed = nccOk && rmseOk;

    result.message = QString("NCC=%1 (threshold>=%2), RMSE=%3 (threshold<=%4), Offset=%5")
                         .arg(result.ncc, 0, 'f', 6)
                         .arg(nccThreshold)
                         .arg(result.rmse, 0, 'f', 6)
                         .arg(rmseThreshold)
                         .arg(result.bestOffset);

    return result;
}

}

#endif
