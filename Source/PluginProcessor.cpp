#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    juce::NormalisableRange<float> skewedRange (float min, float max, float interval, float centre)
    {
        juce::NormalisableRange<float> r (min, max, interval);
        r.setSkewForCentre (centre);
        return r;
    }

    juce::AudioParameterFloatAttributes withSuffix (const juce::String& suffix, int decimals = 1)
    {
        return juce::AudioParameterFloatAttributes()
            .withLabel (suffix)
            .withStringFromValueFunction ([suffix, decimals] (float v, int)
                                          { return juce::String (v, decimals) + " " + suffix; });
    }
}

TMRXCompressorProcessor::TMRXCompressorProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    thresholdParam = apvts.getRawParameterValue (ParamIDs::threshold);
    ratioParam     = apvts.getRawParameterValue (ParamIDs::ratio);
    kneeParam      = apvts.getRawParameterValue (ParamIDs::knee);
    attackParam    = apvts.getRawParameterValue (ParamIDs::attack);
    releaseParam   = apvts.getRawParameterValue (ParamIDs::release);
    makeupParam    = apvts.getRawParameterValue (ParamIDs::makeup);
    mixParam       = apvts.getRawParameterValue (ParamIDs::mix);
    rmsParam       = apvts.getRawParameterValue (ParamIDs::rms);
}

juce::AudioProcessorValueTreeState::ParameterLayout TMRXCompressorProcessor::createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::threshold, 1 }, "Threshold",
                                                       NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -18.0f,
                                                       withSuffix ("dB")));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::ratio, 1 }, "Ratio",
                                                       skewedRange (1.0f, 20.0f, 0.01f, 4.0f), 4.0f,
                                                       AudioParameterFloatAttributes()
                                                           .withStringFromValueFunction ([] (float v, int)
                                                                                         { return String (v, 1) + ":1"; })));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::knee, 1 }, "Knee",
                                                       NormalisableRange<float> (0.0f, 24.0f, 0.1f), 6.0f,
                                                       withSuffix ("dB")));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::attack, 1 }, "Attack",
                                                       skewedRange (0.1f, 200.0f, 0.01f, 10.0f), 10.0f,
                                                       withSuffix ("ms", 2)));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::release, 1 }, "Release",
                                                       skewedRange (5.0f, 2000.0f, 0.1f, 150.0f), 120.0f,
                                                       withSuffix ("ms", 0)));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::makeup, 1 }, "Makeup",
                                                       NormalisableRange<float> (-12.0f, 24.0f, 0.1f), 0.0f,
                                                       withSuffix ("dB")));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::mix, 1 }, "Mix",
                                                       NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
                                                       withSuffix ("%", 0)));

    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::rms, 1 }, "RMS Detector", false));

    return layout;
}

void TMRXCompressorProcessor::prepareToPlay (double sampleRate, int)
{
    updateDspParameters();
    compressor.prepare (sampleRate);
    gainReductionDb.store (0.0f);
}

bool TMRXCompressorProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return out == layouts.getMainInputChannelSet();
}

void TMRXCompressorProcessor::updateDspParameters()
{
    tmrx::CompressorDSP::Parameters p;
    p.thresholdDb = thresholdParam->load();
    p.ratio       = ratioParam->load();
    p.kneeDb      = kneeParam->load();
    p.attackMs    = attackParam->load();
    p.releaseMs   = releaseParam->load();
    p.makeupDb    = makeupParam->load();
    p.mix         = mixParam->load() * 0.01f;
    p.rmsDetector = rmsParam->load() >= 0.5f;
    compressor.setParameters (p);
}

void TMRXCompressorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numIn  = getTotalNumInputChannels();
    const auto numOut = getTotalNumOutputChannels();

    for (auto ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    updateDspParameters();

    const float gr = compressor.process (buffer.getArrayOfWritePointers(),
                                         juce::jmin (numIn, buffer.getNumChannels()),
                                         buffer.getNumSamples());

    // 미터가 읽어가기 전까지의 최대값을 유지
    float prev = gainReductionDb.load();
    while (gr > prev && ! gainReductionDb.compare_exchange_weak (prev, gr)) {}
}

juce::AudioProcessorEditor* TMRXCompressorProcessor::createEditor()
{
    return new TMRXCompressorEditor (*this);
}

void TMRXCompressorProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void TMRXCompressorProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TMRXCompressorProcessor();
}
