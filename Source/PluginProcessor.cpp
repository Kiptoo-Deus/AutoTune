/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AutotuneAudioProcessor::AutotuneAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    outputBuffer(2, bufferSize) // Stereo output buffer
{
    std::fill(analysisBuffer, analysisBuffer + bufferSize, 0.0f);
    currentSampleRate = 0.0;
    previousPitch = 0.0f;
    outputBuffer.clear();
}

AutotuneAudioProcessor::~AutotuneAudioProcessor()
{
}

//==============================================================================
const juce::String AutotuneAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AutotuneAudioProcessor::acceptsMidi() const
{
    return true; // Enable MIDI for potential scale/key input later
}

bool AutotuneAudioProcessor::producesMidi() const
{
    return false;
}

bool AutotuneAudioProcessor::isMidiEffect() const
{
    return false;
}

double AutotuneAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AutotuneAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs
}

int AutotuneAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AutotuneAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String AutotuneAudioProcessor::getProgramName(int index)
{
    return {};
}

void AutotuneAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void AutotuneAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    outputBuffer.setSize(2, samplesPerBlock); 
    outputBuffer.clear();
}

void AutotuneAudioProcessor::releaseResources()
{
    // Nothing to release yet
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AutotuneAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Support stereo in/out for simplicity
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}
#endif

void AutotuneAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    auto* leftChannelData = buffer.getWritePointer(0);
    int numSamples = buffer.getNumSamples();
    int samplesToCopy = juce::jmin(numSamples, bufferSize);
    for (int i = 0; i < samplesToCopy; ++i)
    {
        float window = 0.5f * (1.0f - cosf(2.0f * juce::MathConstants<float>::pi * i / (bufferSize - 1)));
        analysisBuffer[i] = leftChannelData[i] * window;
    }
    for (int i = samplesToCopy; i < bufferSize; ++i)
        analysisBuffer[i] = 0.0f;

    // Autocorrelation
    float autocorr[bufferSize] = { 0 };
    const int minPeriod = static_cast<int>(currentSampleRate / 1000.0);
    const int maxPeriod = static_cast<int>(currentSampleRate / 50.0);

    for (int lag = minPeriod; lag < maxPeriod; ++lag)
    {
        for (int i = 0; i < bufferSize - lag; ++i)
        {
            autocorr[lag] += analysisBuffer[i] * analysisBuffer[i + lag];
        }
    }

    // Find peak
    float maxAutocorr = 0;
    int period = minPeriod;
    float threshold = 0.1f * autocorr[0];
    for (int lag = minPeriod; lag < maxPeriod; ++lag)
    {
        if (autocorr[lag] > maxAutocorr && autocorr[lag] > threshold)
        {
            maxAutocorr = autocorr[lag];
            period = lag;
        }
    }

    float detectedFreq = (maxAutocorr > threshold) ? static_cast<float>(currentSampleRate / period) : 0.0f;

    // Smooth pitch to avoid jumps
    if (detectedFreq > 0.0f)
        detectedFreq = 0.9f * previousPitch + 0.1f * detectedFreq; // Basic smoothing
    previousPitch = detectedFreq;

    DBG("Detected Pitch: " << detectedFreq << " Hz");

    // Pitch correction to nearest semitone
    float midiNote = (detectedFreq > 0.0f) ? 12.0f * log2f(detectedFreq / 440.0f) + 69.0f : 0.0f;
    int targetNote = static_cast<int>(roundf(midiNote));
    float targetFreq = (midiNote > 0.0f) ? 440.0f * powf(2.0f, (targetNote - 69.0f) / 12.0f) : 0.0f;
    float pitchRatio = (detectedFreq > 0.0f && targetFreq > 0.0f) ? targetFreq / detectedFreq : 1.0f;

    DBG("Target Pitch: " << targetFreq << " Hz, Ratio: " << pitchRatio);

    // Basic pitch shifting (resampling)
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        auto* outData = outputBuffer.getWritePointer(channel);
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float srcPos = sample * pitchRatio;
            int intPos = static_cast<int>(srcPos);
            float frac = srcPos - intPos;

            if (intPos + 1 < numSamples)
            {
                // Linear interpolation
                float sampleA = buffer.getSample(channel, intPos);
                float sampleB = buffer.getSample(channel, intPos + 1);
                outData[sample] = sampleA + frac * (sampleB - sampleA);
            }
            else
                outData[sample] = buffer.getSample(channel, sample);
        }

        // Copy to output
        for (int sample = 0; sample < numSamples; ++sample)
            channelData[sample] = outData[sample];
    }
}

//==============================================================================
bool AutotuneAudioProcessor::hasEditor() const
{
    return true; 
}

juce::AudioProcessorEditor* AutotuneAudioProcessor::createEditor()
{
    return new AutotuneAudioProcessorEditor(*this);
}

//==============================================================================
void AutotuneAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Add parameter saving later 
}

void AutotuneAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Add parameter loading later 
}


juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AutotuneAudioProcessor();
}