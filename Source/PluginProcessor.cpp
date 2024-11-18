#include "PluginProcessor.h"
#include "PluginEditor.h"

AutoTuneAudioProcessor::AutoTuneAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ), parameters(*this, nullptr, "Parameters",
        {
            std::make_unique<juce::AudioParameterFloat>("pitchCorrection", "Pitch Correction", 0.0f, 1.0f, 0.5f),
            std::make_unique<juce::AudioParameterFloat>("formantShift", "Formant Shift", -12.0f, 12.0f, 0.0f),
            std::make_unique<juce::AudioParameterFloat>("vibratoDepth", "Vibrato Depth", 0.0f, 10.0f, 0.0f),
            std::make_unique<juce::AudioParameterFloat>("vibratoRate", "Vibrato Rate", 0.1f, 10.0f, 1.0f)
        })
#endif
{
}

AutoTuneAudioProcessor::~AutoTuneAudioProcessor() {}

const juce::String AutoTuneAudioProcessor::getName() const { return JucePlugin_Name; }
bool AutoTuneAudioProcessor::acceptsMidi() const { return false; }
bool AutoTuneAudioProcessor::producesMidi() const { return false; }
bool AutoTuneAudioProcessor::isMidiEffect() const { return false; }
double AutoTuneAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int AutoTuneAudioProcessor::getNumPrograms() { return 1; }
int AutoTuneAudioProcessor::getCurrentProgram() { return 0; }
void AutoTuneAudioProcessor::setCurrentProgram(int index) {}
const juce::String AutoTuneAudioProcessor::getProgramName(int index) { return {}; }
void AutoTuneAudioProcessor::changeProgramName(int index, const juce::String& newName) {}

void AutoTuneAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {}
void AutoTuneAudioProcessor::releaseResources() {}

bool AutoTuneAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void AutoTuneAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        int numSamples = buffer.getNumSamples();
        // Detect the pitch
        float detectedPitch = detectPitch(channelData, numSamples, getSampleRate());
        // Correct the pitch
        float targetPitch = 440.0f; 
        float correctedPitch = correctPitch(detectedPitch, targetPitch);
        // Calculate pitch shift ratio
        float pitchShiftRatio = detectedPitch > 0 ? correctedPitch / detectedPitch : 1.0f;
        // Apply pitch shift
        applyPitchShift(channelData, numSamples, pitchShiftRatio);
    }
}
void AutoTuneAudioProcessor::applyPitchShift(float* channelData, int numSamples, float pitchShiftRatio)
{
    juce::AudioBuffer<float> resampledBuffer(1, (int)(numSamples / pitchShiftRatio));
    auto* resampledData = resampledBuffer.getWritePointer(0);

    for (int i = 0; i < resampledBuffer.getNumSamples(); ++i)
    {
        float originalIndex = i * pitchShiftRatio;
        int indexA = (int)originalIndex;
        int indexB = juce::jmin(indexA + 1, numSamples - 1);
        float fraction = originalIndex - indexA;

        resampledData[i] = channelData[indexA] * (1.0f - fraction) + channelData[indexB] * fraction;
    }

    juce::FloatVectorOperations::copy(channelData, resampledData, resampledBuffer.getNumSamples());
}


juce::AudioProcessorEditor* AutoTuneAudioProcessor::createEditor()
{
    return new AutoTuneAudioProcessorEditor(*this);
}

bool AutoTuneAudioProcessor::hasEditor() const { return true; }
void AutoTuneAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {}
void AutoTuneAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {}

float AutoTuneAudioProcessor::detectPitch(const float* audioData, int numSamples, double sampleRate)
{
    const int minLag = sampleRate / 400;
    const int maxLag = sampleRate / 50;
    const float threshold = 0.1f;

    std::vector<float> difference(maxLag);
    std::vector<float> cumulativeDifference(maxLag);

    for (int lag = minLag; lag < maxLag; ++lag)
    {
        float sum = 0.0f;
        for (int i = 0; i < numSamples - lag; ++i)
            sum += std::pow(audioData[i] - audioData[i + lag], 2);
        difference[lag] = sum;
    }

    cumulativeDifference[minLag] = difference[minLag];
    for (int lag = minLag + 1; lag < maxLag; ++lag)
        cumulativeDifference[lag] = difference[lag] / ((lag + 1) / (float)lag);

    for (int lag = minLag; lag < maxLag; ++lag)
        if (cumulativeDifference[lag] < threshold)
            return sampleRate / lag;

    return 0.0f;
}

float AutoTuneAudioProcessor::correctPitch(float detectedPitch, float targetPitch)
{
    return targetPitch;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AutoTuneAudioProcessor();
}
