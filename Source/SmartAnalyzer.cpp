#include "SmartAnalyzer.h"

SmartAnalyzer::SmartAnalyzer() = default;

void SmartAnalyzer::prepare(double sampleRate)
{
    std::lock_guard<std::mutex> lockGuard (lock);
    this->sampleRate = sampleRate;
    fftData.assign (numBins, 0.0f);
    fastDb.assign (numBins, 0.0f);
    longTermDb.assign (numBins, 0.0f);
    targetDb.assign (numBins, 0.0f);
    warnings.clear();
}

void SmartAnalyzer::pushAudio (const float* left, const float* right, int numSamples)
{
    (void) left;
    (void) right;
    (void) numSamples;
}

void SmartAnalyzer::reset()
{
    std::lock_guard<std::mutex> lockGuard (lock);
    fftData.clear();
    fastDb.assign (numBins, 0.0f);
    longTermDb.assign (numBins, 0.0f);
    targetDb.assign (numBins, 0.0f);
    warnings.clear();
}

std::vector<float> SmartAnalyzer::getLongTermDb() const
{
    std::lock_guard<std::mutex> lockGuard (lock);
    return longTermDb;
}

std::vector<float> SmartAnalyzer::getFastDb() const
{
    std::lock_guard<std::mutex> lockGuard (lock);
    return fastDb;
}

std::vector<float> SmartAnalyzer::getTargetDb() const
{
    std::lock_guard<std::mutex> lockGuard (lock);
    return targetDb;
}

std::vector<SmartAnalyzer::Warning> SmartAnalyzer::getWarnings() const
{
    std::lock_guard<std::mutex> lockGuard (lock);
    return warnings;
}
