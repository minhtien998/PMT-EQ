#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class SpectrumDisplay : public juce::Component, public juce::Timer
{
public:
    explicit SpectrumDisplay (SmartEQAudioProcessor& p) : proc (p) { startTimerHz (30); }
    void paint (juce::Graphics&) override;
    void timerCallback() override { repaint(); }
private:
    SmartEQAudioProcessor& proc;
};

class WarningPanel : public juce::Component, public juce::Timer
{
public:
    explicit WarningPanel (SmartEQAudioProcessor& p) : proc (p) { startTimerHz (4); }
    void paint (juce::Graphics&) override;
    void timerCallback() override { repaint(); }
private:
    SmartEQAudioProcessor& proc;
};

class SmartEQAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SmartEQAudioProcessorEditor (SmartEQAudioProcessor&);
    ~SmartEQAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SmartEQAudioProcessor& proc;
    SpectrumDisplay spectrum;
    WarningPanel    warnings;

    juce::OwnedArray<juce::Slider> freqSl, gainSl, qSl;
    juce::OwnedArray<juce::ToggleButton> onBtn;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> freqAtt, gainAtt, qAtt;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ButtonAttachment> onAtt;
};
