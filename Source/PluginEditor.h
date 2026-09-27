#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

/** 세로형 게인 리덕션 미터 (위에서 아래로 채워짐). */
class GainReductionMeter final : public juce::Component
{
public:
    void setValueDb (float newValueDb);
    void paint (juce::Graphics&) override;

    static constexpr float rangeDb = 24.0f;

private:
    float valueDb = 0.0f;
};

class TMRXCompressorEditor final : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
public:
    explicit TMRXCompressorEditor (TMRXCompressorProcessor&);
    ~TMRXCompressorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void setupKnob (Knob&, const juce::String& paramId, const juce::String& name);

    TMRXCompressorProcessor& processor;
    juce::LookAndFeel_V4 lookAndFeel;

    Knob threshold, ratio, knee, attack, release, makeup, mix;
    juce::ToggleButton rmsButton { "RMS" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> rmsAttachment;

    GainReductionMeter meter;
    juce::Label meterLabel;
    float displayedGrDb = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TMRXCompressorEditor)
};
