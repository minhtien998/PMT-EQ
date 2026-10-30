#include "PluginEditor.h"

SpectrumDisplay::SpectrumDisplay (SmartEQAudioProcessor& p) : proc (p)
{
    startTimerHz (30);
}

void SpectrumDisplay::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    g.setColour (juce::Colours::lightblue);
    g.drawText ("Spectrum", getLocalBounds(), juce::Justification::centred, true);
}

void SpectrumDisplay::timerCallback()
{
    repaint();
}

WarningPanel::WarningPanel (SmartEQAudioProcessor& p) : proc (p)
{
    startTimerHz (4);
}

void WarningPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkgrey);
    g.setColour (juce::Colours::white);
    g.drawText ("Warnings", getLocalBounds(), juce::Justification::centred, true);
}

void WarningPanel::timerCallback()
{
    repaint();
}

SmartEQAudioProcessorEditor::SmartEQAudioProcessorEditor (SmartEQAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      proc (p),
      spectrum (p),
      warnings (p)
{
    setSize (600, 400);
    addAndMakeVisible (spectrum);
    addAndMakeVisible (warnings);
}

void SmartEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkslategrey);
}

void SmartEQAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    spectrum.setBounds (area.removeFromTop (area.getHeight() * 0.7f));
    warnings.setBounds (area);
}
