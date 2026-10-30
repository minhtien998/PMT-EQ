#pragma once
#include <JuceHeader.h>
#include <vector>
#include <mutex>

class SmartAnalyzer
{
public:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize  = 1 << fftOrder;
    static constexpr int numBins  = fftSize / 2 + 1;

    struct Warning
    {
        juce::String category;
        juce::String message;
        float lowFreq = 0, highFreq = 0;
        float centerFreq = 0;
        float amountDb = 0;
        bool  isExcess = false;
    };

    SmartAnalyzer();

    void prepare (double sampleRate);
    void pushAudio (const float* left, const float* right, int numSamples);
    void reset();

    std::vector<float> getLongTermDb() const;
    std::vector<float> getFastDb() const;
    std::vector<float> getTargetDb() const;
    std::vector<Warning> getWarnings() const;

    float binToFreq (int bin) const
    { return bin * (float)sampleRate / (float)fftSize; }

private:
    double sampleRate = 48000.0;
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;

    std::vector<float> fftData;
    std::vector<float> ring;
    int ringWrite = 0;
    int hopCount  = 0;
    static constexpr int hopSize = 512;

    std::vector<float> fastDb;
    std::vector<float> longTermDb;
    std::vector<float> targetDb;

    std::vector<Warning> warnings;

    mutable std::mutex lock;

    void computeFrame();
    void recomputeTarget();
    void updateWarnings();
};
