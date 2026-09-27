#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "dsp/CompressorDSP.h"

namespace ParamIDs
{
    inline constexpr auto threshold = "threshold";
    inline constexpr auto ratio     = "ratio";
    inline constexpr auto knee      = "knee";
    inline constexpr auto attack    = "attack";
    inline constexpr auto release   = "release";
    inline constexpr auto makeup    = "makeup";
    inline constexpr auto mix       = "mix";
    inline constexpr auto rms       = "rms";
}

class TMRXCompressorProcessor final : public juce::AudioProcessor
{
public:
    TMRXCompressorProcessor();
    ~TMRXCompressorProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override                             { return 1; }
    int getCurrentProgram() override                          { return 0; }
    void setCurrentProgram (int) override                     {}
    const juce::String getProgramName (int) override          { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getState() noexcept { return apvts; }

    /** UI 미터용: 마지막으로 읽은 이후의 최대 게인 리덕션(dB)을 가져오고 리셋. */
    float popGainReductionDb() noexcept { return gainReductionDb.exchange (0.0f); }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    void updateDspParameters();

    juce::AudioProcessorValueTreeState apvts;
    tmrx::CompressorDSP compressor;
    std::atomic<float> gainReductionDb { 0.0f };

    std::atomic<float>* thresholdParam = nullptr;
    std::atomic<float>* ratioParam     = nullptr;
    std::atomic<float>* kneeParam      = nullptr;
    std::atomic<float>* attackParam    = nullptr;
    std::atomic<float>* releaseParam   = nullptr;
    std::atomic<float>* makeupParam    = nullptr;
    std::atomic<float>* mixParam       = nullptr;
    std::atomic<float>* rmsParam       = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TMRXCompressorProcessor)
};
