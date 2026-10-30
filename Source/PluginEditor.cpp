#include "PluginEditor.h"

void SpectrumDisplay::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff101018));
    auto b = getLocalBounds().toFloat();

    auto& an = proc.analyzer;
    auto xOf = [&](float f) {
        return juce::jmap (std::log10 (f), std::log10 (20.f), std::log10 (20000.f),
                           b.getX(), b.getRight());
    };
    auto yOf = [&](float db) {
        return juce::jmap (db, -90.f, 0.f, b.getBottom(), b.getY());
    };

    auto target   = an.getTargetDb();
    auto longTerm = an.getLongTermDb();
    auto fast     = an.getFastDb();

    int refBin = juce::jlimit (0, SmartAnalyzer::numBins - 1,
                               (int) std::round (1000.f * SmartAnalyzer::fftSize / proc.getSampleRate()));
    float offset = longTerm[refBin] - target[refBin];

    juce::Path pT; bool first = true;
    for (int i = 0; i < SmartAnalyzer::numBins; ++i) {
        float f = an.binToFreq (i);
        if (f < 20.f || f > 20000.f) continue;
        float db = target[i] + offset;
        if (db < -90.f || db > 0.f) continue;
        float x = xOf (f), y = yOf (db);
        if (first) { pT.startNewSubPath (x, y); first = false; }
        else pT.lineTo (x, y);
    }
    g.setColour (juce::Colours::orange.withAlpha (0.7f));
    g.strokePath (pT, juce::PathStrokeType (1.2f));

    juce::Path pF; first = true;
    for (int i = 0; i < SmartAnalyzer::numBins; ++i) {
        float f = an.binToFreq (i);
        if (f < 20.f || f > 20000.f) continue;
        float x = xOf (f), y = yOf (fast[i]);
        if (first) { pF.startNewSubPath (x, y); first = false; }
        else pF.lineTo (x, y);
    }
    g.setColour (juce::Colours::cyan.withAlpha (0.35f));
    g.strokePath (pF, juce::PathStrokeType (1.f));

    juce::Path pL; first = true;
    for (int i = 0; i < SmartAnalyzer::numBins; ++i) {
        float f = an.binToFreq (i);
        if (f < 20.f || f > 20000.f) continue;
        float x = xOf (f), y = yOf (longTerm[i]);
        if (first) { pL.startNewSubPath (x, y); first = false; }
        else pL.lineTo (x, y);
    }
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.strokePath (pL, juce::PathStrokeType (1.5f));

    const double sr = proc.getSampleRate();
    juce::Path pE; first = true;
    for (int i = 0; i <= 200; ++i) {
        float t = (float)i / 200.f;
        float f = std::pow (10.f, juce::jmap (t, std::log10 (20.f), std::log10 (20000.f)));
        float dbSum = 0.f;
        for (int bn = 0; bn < SmartEQAudioProcessor::numBands; ++bn) {
            auto& filt = proc.getFilter (bn, 0);
            if (filt.coefficients)
                dbSum += juce::Decibels::gainToDecibels (
                            filt.coefficients->getMagnitudeForFrequency (f, sr), -100.f);
        }
        float x = xOf (f), y = yOf (dbSum);
        if (first) { pE.startNewSubPath (x, y); first = false; }
        else pE.lineTo (x, y);
    }
    g.setColour (juce::Colours::yellow);
    g.strokePath (pE, juce::PathStrokeType (2.f));

    g.setColour (juce::Colours::grey);
    g.setFont (10.f);
    for (float f : {30.f, 100.f, 300.f, 1000.f, 3000.f, 10000.f}) {
        float x = xOf (f);
        juce::String t = f < 1000.f ? juce::String ((int)f)
                                    : juce::String (f / 1000.f, 1) + "k";
        g.drawText (t, (int)x - 15, (int)b.getBottom() - 14, 30, 12,
                    juce::Justification::centred);
    }

    for (auto& w : an.getWarnings()) {
        float x = xOf (w.centerFreq);
        g.setColour (w.isExcess ? juce::Colours::orange : juce::Colours::lightskyblue);
        g.drawLine (x, b.getY(), x, b.getBottom(), 1.f);
    }
}

void WarningPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1a22));
    auto& ws = proc.analyzer.getWarnings();

    if (ws.empty()) {
        g.setColour (juce::Colours::lightgreen);
        g.setFont (14.f);
        g.drawText ("OK - Can bang, khong phat hien bat thuong",
                    getLocalBounds(), juce::Justification::centred);
        return;
    }

    g.setFont (12.f);
    int y = 6;
    for (auto& w : ws) {
        g.setColour (w.isExcess ? juce::Colours::orange : juce::Colours::lightskyblue);
        g.drawText ((w.isExcess ? "! DU: " : "! THIEU: ") + w.message,
                    8, y, getWidth() - 16, 18, juce::Justification::left);
        y += 20;
        if (y > getHeight() - 20) break;
    }
}

SmartEQAudioProcessorEditor::SmartEQAudioProcessorEditor (SmartEQAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p), spectrum (p), warnings (p)
{
    addAndMakeVisible (spectrum);
    addAndMakeVisible (warnings);

    for (int b = 0; b < SmartEQAudioProcessor::numBands; ++b)
    {
        juce::String id = "band" + juce::String (b);

        auto* on = new juce::ToggleButton ("B" + juce::String (b + 1));
        onBtn.add (on); addAndMakeVisible (on);
        onAtt.add (new juce::AudioProcessorValueTreeState::ButtonAttachment (
                       proc.apvts, id + "_on", *on));

        auto* f = new juce::Slider (juce::Slider::RotaryHorizontalVerticalDrag,
                                    juce::Slider::TextBoxBelow);
        f->setRange (20.f, 20000.f, 0.01f);
        f->setSkewFactorFromMidPoint (1000.f);
        f->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 14);
        freqSl.add (f); addAndMakeVisible (f);
        freqAtt.add (new juce::AudioProcessorValueTreeState::SliderAttachment (
                         proc.apvts, id + "_freq", *f));

        auto* g = new juce::Slider (juce::Slider::RotaryHorizontalVerticalDrag,
                                    juce::Slider::TextBoxBelow);
        g->setRange (-24.f, 24.f, 0.01f);
        g->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 14);
        gainSl.add (g); addAndMakeVisible (g);
        gainAtt.add (new juce::AudioProcessorValueTreeState::SliderAttachment (
                         proc.apvts, id + "_gain", *g));

        auto* q = new juce::Slider (juce::Slider::RotaryHorizontalVerticalDrag,
                                    juce::Slider::TextBoxBelow);
        q->setRange (0.1f, 10.f, 0.01f);
        q->setSkewFactorFromMidPoint (1.f);
        q->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 14);
        qSl.add (q); addAndMakeVisible (q);
        qAtt.add (new juce::AudioProcessorValueTreeState::SliderAttachment (
                      proc.apvts, id + "_q", *q));
    }

    setSize (900, 620);
}

void SmartEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff08080c));
}

void SmartEQAudioProcessorEditor::resized()
{
    auto b = getLocalBounds().reduced (6);

    auto top = b.removeFromTop ((int)(b.getHeight() * 0.6f));
    spectrum.setBounds (top);

    warnings.setBounds (b.removeFromTop (110));

    b.removeFromTop (6);
    int n = SmartEQAudioProcessor::numBands;
    int colW = b.getWidth() / n;
    for (int i = 0; i < n; ++i)
    {
        auto c = b.removeFromLeft (colW);
        onBtn[i]->setBounds (c.removeFromTop (22).reduced (4, 0));
        int h = c.getHeight() / 3;
        freqSl[i]->setBounds (c.removeFromTop (h).reduced (2));
        gainSl[i]->setBounds (c.removeFromTop (h).reduced (2));
        qSl[i]->setBounds (c.reduced (2));
    }
}
