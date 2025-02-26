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
    circularBuffer(2, bufferSize * 2)  // Double bufferSize for safety
{
    std::fill(analysisBuffer, analysisBuffer + bufferSize, 0.0f);
    currentSampleRate = 0.0;
    writePosition = 0;
    readPosition = 0.0f;
    previousPitch = 0.0f;
    circularBuffer.clear();
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
    circularBuffer.setSize(2, samplesPerBlock * 2); // Ensure enough room
    circularBuffer.clear();
    writePosition = 0;
    readPosition = 0.0f;
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

    // Pitch detection
    int samplesToCopy = juce::jmin(numSamples, bufferSize);
    for (int i = 0; i < samplesToCopy; ++i)
    {
        float window = 0.5f * (1.0f - cosf(2.0f * juce::MathConstants<float>::pi * i / (bufferSize - 1)));
        analysisBuffer[i] = leftChannelData[i] * window;
    }
    for (int i = samplesToCopy; i < bufferSize; ++i)
        analysisBuffer[i] = 0.0f;

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

    // Adjust pitch smoothing with retune speed (hardcoded for now)
    float retuneSpeed = 0.1f; // 0.0 = instant, 1.0 = no correction (will connect to slider later)
    if (detectedFreq > 0.0f)
        detectedFreq = (1.0f - retuneSpeed) * previousPitch + retuneSpeed * detectedFreq;
    previousPitch = detectedFreq;

    // Pitch correction to C major scale
    float midiNote = (detectedFreq > 0.0f) ? 12.0f * log2f(detectedFreq / 440.0f) + 69.0f : 0.0f;
    int targetNote = (midiNote > 0.0f) ? snapToCMajor(midiNote) : 0; // Snap to C major instead of chromatic
    float targetFreq = (midiNote > 0.0f) ? 440.0f * powf(2.0f, (targetNote - 69.0f) / 12.0f) : 0.0f;
    float pitchRatio = (detectedFreq > 0.0f && targetFreq > 0.0f) ? targetFreq / detectedFreq : 1.0f;

    DBG("Detected: " << detectedFreq << " Hz, Target: " << targetFreq << " Hz, Ratio: " << pitchRatio);

    // Write input to circular buffer
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* inData = buffer.getWritePointer(channel);
        auto* circData = circularBuffer.getWritePointer(channel);
        for (int i = 0; i < numSamples; ++i)
        {
            int pos = (writePosition + i) % circularBuffer.getNumSamples();
            circData[pos] = inData[i];
        }
    }

    // Read from circular buffer with pitch shift
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* outData = buffer.getWritePointer(channel);
        auto* circData = circularBuffer.getReadPointer(channel);
        for (int i = 0; i < numSamples; ++i)
        {
            int intPos = static_cast<int>(readPosition);
            float frac = readPosition - intPos;
            int nextPos = (intPos + 1) % circularBuffer.getNumSamples();

            float sampleA = circData[intPos];
            float sampleB = circData[nextPos];
            outData[i] = sampleA + frac * (sampleB - sampleA);

            readPosition += pitchRatio;
            if (readPosition >= circularBuffer.getNumSamples())
                readPosition -= circularBuffer.getNumSamples();
        }
    }

    writePosition = (writePosition + numSamples) % circularBuffer.getNumSamples();
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