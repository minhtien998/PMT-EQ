#include "SmartAnalyzer.h"
#include <algorithm>
#include <cmath>

SmartAnalyzer::SmartAnalyzer()
    : fft (fftOrder),
      window (fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    fftData.resize (fftSize * 2, 0.0f);
    ring.resize (fftSize, 0.0f);
    fastDb.assign (numBins, -100.0f);
    longTermDb.assign (numBins, -100.0f);
    targetDb.assign (numBins, 0.0f);
}

void SmartAnalyzer::prepare (double sr)
{
    std::lock_guard<std::mutex> g (lock);
    sampleRate = sr;
    recomputeTarget();
    std::fill (fastDb.begin(), fastDb.end(), -100.0f);
    std::fill (longTermDb.begin(), longTermDb.end(), -100.0f);
    std::fill (ring.begin(), ring.end(), 0.0f);
    ringWrite = hopCount = 0;
}

void SmartAnalyzer::reset()
{
    std::lock_guard<std::mutex> g (lock);
    std::fill (fastDb.begin(), fastDb.end(), -100.0f);
    std::fill (longTermDb.begin(), longTermDb.end(), -100.0f);
    std::fill (ring.begin(), ring.end(), 0.0f);
    ringWrite = 0;
    hopCount = 0;
    warnings.clear();
}

void SmartAnalyzer::pushAudio (const float* L, const float* R, int n)
{
    for (int i = 0; i < n; ++i)
    {
        ring[ringWrite] = 0.5f * (L[i] + R[i]);
        ringWrite = (ringWrite + 1) % fftSize;
        if (++hopCount >= hopSize) { hopCount = 0; computeFrame(); }
    }
}

void SmartAnalyzer::recomputeTarget()
{
    const float slopeDbPerOct = -4.5f;
    for (int b = 0; b < numBins; ++b)
    {
        float f = juce::jmax (20.0f, binToFreq (b));
        targetDb[b] = slopeDbPerOct * std::log2 (f / 1000.0f);
    }
}

void SmartAnalyzer::computeFrame()
{
    std::lock_guard<std::mutex> g (lock);

    std::fill (fftData.begin(), fftData.end(), 0.0f);
    for (int i = 0; i < fftSize; ++i)
        fftData[i] = ring[(ringWrite + i) % fftSize];

    window.multiplyWithWindowingTable (fftData.data(), fftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    const float alpha = 0.02f;
    for (int b = 0; b < numBins; ++b)
    {
        float mag = fftData[b] / (float)fftSize * 2.0f;
        float db  = juce::Decibels::gainToDecibels (mag, -100.0f);
        fastDb[b] = db;
        if (longTermDb[b] < -99.0f) longTermDb[b] = db;
        else                        longTermDb[b] = alpha * db + (1.0f - alpha) * longTermDb[b];
    }

    updateWarnings();
}

void SmartAnalyzer::updateWarnings()
{
    warnings.clear();

    int refBin = juce::jlimit (1, numBins - 2, (int)std::round (1000.0f * fftSize / sampleRate));
    std::vector<float> dev (numBins);
    for (int b = 0; b < numBins; ++b) dev[b] = longTermDb[b] - targetDb[b];

    float refSum = 0; int refCnt = 0;
    for (int b = 0; b < numBins; ++b)
    {
        float f = binToFreq (b);
        if (f >= 800 && f <= 1200) { refSum += dev[b]; ++refCnt; }
    }
    float ref = refCnt > 0 ? refSum / refCnt : dev[refBin];
    for (auto& d : dev) d -= ref;

    struct BandDef {
        const char* name;
        float low, high;
        float excessThr, deficitThr;
        const char* excessMsg;
        const char* deficitMsg;
        float suggestFreq;
    };
    const BandDef bands[] = {
        {"Sub-bass", 20, 60,      7, 8,
            "Rung/u, thu HPF 30-40 Hz hoac cat quanh 45 Hz.",
            "Mong, thieu nen - thu boost nhe 50 Hz.", 45},
        {"Bass", 60, 250,         5, 6,
            "Bass bi boom - thu cut ~120 Hz.",
            "Bass thieu - thu boost ~100 Hz.", 120},
        {"Low-mid", 250, 500,     5, 6,
            "Bi muddy - cut 300-400 Hz.",
            "Bi rong - boost ~350 Hz.", 350},
        {"Mid", 500, 2000,        4, 5,
            "Nasal/honky - cut quanh 800 Hz.",
            "Bi lui - boost quanh 1 kHz.", 900},
        {"High-mid", 2000, 4000,  5, 5,
            "Harsh - cut quanh 3 kHz.",
            "Thieu presence - boost quanh 3 kHz.", 3000},
        {"Presence", 4000, 8000,  5, 6,
            "Sibilant/choi - cut quanh 6 kHz.",
            "Duc/dull - boost quanh 5 kHz.", 5500},
        {"Air", 8000, 20000,      6, 7,
            "Qua sang/choi - cut ~10 kHz.",
            "Thieu air - boost ~12 kHz.", 12000},
    };

    for (auto& bd : bands)
    {
        float sum = 0; int cnt = 0;
        for (int b = 0; b < numBins; ++b)
        {
            float f = binToFreq (b);
            if (f >= bd.low && f <= bd.high) { sum += dev[b]; ++cnt; }
        }
        if (cnt == 0) continue;
        float avg = sum / cnt;

        if (avg > bd.excessThr)
        {
            Warning w;
            w.category   = bd.name;
            w.message    = juce::String (bd.name) + " DU (" + juce::String (avg, 1) + " dB): " + bd.excessMsg;
            w.lowFreq    = bd.low; w.highFreq = bd.high;
            w.centerFreq = bd.suggestFreq;
            w.amountDb   = avg;
            w.isExcess   = true;
            warnings.push_back (w);
        }
        else if (avg < -bd.deficitThr)
        {
            Warning w;
            w.category   = bd.name;
            w.message    = juce::String (bd.name) + " THIEU (" + juce::String (avg, 1) + " dB): " + bd.deficitMsg;
            w.lowFreq    = bd.low; w.highFreq = bd.high;
            w.centerFreq = bd.suggestFreq;
            w.amountDb   = avg;
            w.isExcess   = false;
            warnings.push_back (w);
        }
    }

    std::sort (warnings.begin(), warnings.end(), [](auto& a, auto& b) {
        return std::abs (a.amountDb) > std::abs (b.amountDb);
    });
}

std::vector<float> SmartAnalyzer::getLongTermDb() const { std::lock_guard<std::mutex> g (lock); return longTermDb; }
std::vector<float> SmartAnalyzer::getFastDb()     const { std::lock_guard<std::mutex> g (lock); return fastDb; }
std::vector<float> SmartAnalyzer::getTargetDb()   const { std::lock_guard<std::mutex> g (lock); return targetDb; }
std::vector<SmartAnalyzer::Warning> SmartAnalyzer::getWarnings() const
{ std::lock_guard<std::mutex> g (lock); return warnings; }
