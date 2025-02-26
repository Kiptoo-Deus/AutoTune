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
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    std::fill(analysisBuffer, analysisBuffer + bufferSize, 0.0f); // Initialize buffer to zeros
    currentSampleRate = 0.0; // Will be set in prepareToPlay
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
    currentSampleRate = sampleRate; // Store sample rate for pitch calculations
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

    // Clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Get left channel data (assuming stereo or mono)
    auto* leftChannelData = buffer.getWritePointer(0);
    int numSamples = buffer.getNumSamples();

    // Copy input to analysis buffer with Hann window
    int samplesToCopy = juce::jmin(numSamples, bufferSize);
    for (int i = 0; i < samplesToCopy; ++i)
    {
        float window = 0.5f * (1.0f - cosf(2.0f * juce::MathConstants<float>::pi * i / (bufferSize - 1)));
        analysisBuffer[i] = leftChannelData[i] * window;
    }
    for (int i = samplesToCopy; i < bufferSize; ++i)
        analysisBuffer[i] = 0.0f; // Zero-pad if buffer is smaller

    // Autocorrelation for pitch detection
    float autocorr[bufferSize] = { 0 };
    const int minPeriod = static_cast<int>(currentSampleRate / 1000.0); // Min freq ~ 20 Hz
    const int maxPeriod = static_cast<int>(currentSampleRate / 50.0);   // Max freq ~ 1000 Hz

    for (int lag = minPeriod; lag < maxPeriod; ++lag)
    {
        for (int i = 0; i < bufferSize - lag; ++i)
        {
            autocorr[lag] += analysisBuffer[i] * analysisBuffer[i + lag];
        }
    }

    // Find peak with threshold and refinement
    float maxAutocorr = 0;
    int period = minPeriod;
    float threshold = 0.1f * autocorr[0]; // Basic energy threshold
    for (int lag = minPeriod; lag < maxPeriod; ++lag)
    {
        if (autocorr[lag] > maxAutocorr && autocorr[lag] > threshold)
        {
            maxAutocorr = autocorr[lag];
            period = lag;
        }
    }

    float detectedFreq = (maxAutocorr > threshold) ? static_cast<float>(currentSampleRate / period) : 0.0f;
    DBG("Detected Pitch: " << detectedFreq << " Hz");

    // Passthrough audio for now
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample < numSamples; ++sample)
        {
            channelData[sample] = buffer.getSample(channel, sample);
        }
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