#include "PluginEditor.h"

namespace Colours
{
    const juce::Colour background { 0xff1b1d22 };
    const juce::Colour panel      { 0xff262a31 };
    const juce::Colour accent     { 0xffff8a3d };
    const juce::Colour text       { 0xffe6e6e6 };
    const juce::Colour dimText    { 0xff8a8f99 };
}

//==============================================================================
void GainReductionMeter::setValueDb (float newValueDb)
{
    newValueDb = juce::jlimit (0.0f, rangeDb, newValueDb);
    if (std::abs (newValueDb - valueDb) > 0.01f)
    {
        valueDb = newValueDb;
        repaint();
    }
}

void GainReductionMeter::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (Colours::panel);
    g.fillRoundedRectangle (bounds, 4.0f);

    const auto bar = bounds.reduced (3.0f);
    const float h = bar.getHeight() * (valueDb / rangeDb);
    g.setColour (Colours::accent);
    g.fillRoundedRectangle (bar.withHeight (h), 2.0f);

    g.setColour (Colours::dimText.withAlpha (0.5f));
    for (float db = 3.0f; db < rangeDb; db += 3.0f)
    {
        const float y = bar.getY() + bar.getHeight() * (db / rangeDb);
        g.drawHorizontalLine ((int) y, bar.getX(), bar.getX() + bar.getWidth() * 0.35f);
    }
}

//==============================================================================
TMRXCompressorEditor::TMRXCompressorEditor (TMRXCompressorProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    lookAndFeel.setColour (juce::Slider::rotarySliderFillColourId, Colours::accent);
    lookAndFeel.setColour (juce::Slider::rotarySliderOutlineColourId, Colours::panel.brighter (0.2f));
    lookAndFeel.setColour (juce::Slider::thumbColourId, Colours::text);
    lookAndFeel.setColour (juce::Slider::textBoxTextColourId, Colours::text);
    lookAndFeel.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    lookAndFeel.setColour (juce::ToggleButton::textColourId, Colours::text);
    lookAndFeel.setColour (juce::ToggleButton::tickColourId, Colours::accent);
    setLookAndFeel (&lookAndFeel);

    setupKnob (threshold, ParamIDs::threshold, "Threshold");
    setupKnob (ratio,     ParamIDs::ratio,     "Ratio");
    setupKnob (knee,      ParamIDs::knee,      "Knee");
    setupKnob (attack,    ParamIDs::attack,    "Attack");
    setupKnob (release,   ParamIDs::release,   "Release");
    setupKnob (makeup,    ParamIDs::makeup,    "Makeup");
    setupKnob (mix,       ParamIDs::mix,       "Mix");

    addAndMakeVisible (rmsButton);
    rmsAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.getState(), ParamIDs::rms, rmsButton);

    addAndMakeVisible (meter);
    meterLabel.setText ("GR", juce::dontSendNotification);
    meterLabel.setJustificationType (juce::Justification::centred);
    meterLabel.setColour (juce::Label::textColourId, Colours::dimText);
    addAndMakeVisible (meterLabel);

    setSize (760, 300);
    startTimerHz (30);
}

TMRXCompressorEditor::~TMRXCompressorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void TMRXCompressorEditor::setupKnob (Knob& k, const juce::String& paramId, const juce::String& name)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible (k.slider);

    k.label.setText (name, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setColour (juce::Label::textColourId, Colours::dimText);
    addAndMakeVisible (k.label);

    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.getState(), paramId, k.slider);
}

void TMRXCompressorEditor::paint (juce::Graphics& g)
{
    g.fillAll (Colours::background);

    g.setColour (Colours::text);
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText ("TMRX COMPRESSOR", 20, 12, 300, 28, juce::Justification::centredLeft);

    g.setColour (Colours::dimText);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("GR " + juce::String (displayedGrDb, 1) + " dB",
                getWidth() - 200, 12, 180, 28, juce::Justification::centredRight);
}

void TMRXCompressorEditor::resized()
{
    auto area = getLocalBounds().reduced (16);
    area.removeFromTop (36);

    auto meterArea = area.removeFromRight (40);
    meterLabel.setBounds (meterArea.removeFromBottom (20));
    meter.setBounds (meterArea.reduced (6, 0));
    area.removeFromRight (12);

    auto bottom = area.removeFromBottom (28);
    rmsButton.setBounds (bottom.removeFromLeft (100));

    Knob* knobs[] = { &threshold, &ratio, &knee, &attack, &release, &makeup, &mix };
    const int w = area.getWidth() / (int) std::size (knobs);

    for (auto* k : knobs)
    {
        auto col = area.removeFromLeft (w);
        k->label.setBounds (col.removeFromTop (20));
        k->slider.setBounds (col.reduced (4));
    }
}

void TMRXCompressorEditor::timerCallback()
{
    const float gr = processor.popGainReductionDb();

    // 빠르게 올라가고 천천히 떨어지는 미터 발리스틱
    displayedGrDb = gr > displayedGrDb ? gr : displayedGrDb * 0.85f + gr * 0.15f;

    meter.setValueDb (displayedGrDb);
    repaint (getWidth() - 200, 12, 180, 28);
}
