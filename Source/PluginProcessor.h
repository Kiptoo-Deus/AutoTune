/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class AutotuneAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    AutotuneAudioProcessor();
    ~AutotuneAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    float getPreviousPitch() const { return previousPitch; }
    float previousPitch;                    // Track pitch for smoothing
   
private:
    //==============================================================================
    static const int bufferSize = 2048;     // Size of buffer for pitch analysis
    float analysisBuffer[bufferSize];       // Buffer to hold audio samples for analysis
    double currentSampleRate;               // Store the sample rate for calculations
    juce::AudioBuffer<float> circularBuffer;// Circular buffer for pitch shifting
    int writePosition;                      // Current position in circular buffer
    float readPosition;                     // Fractional read position for shifting
   
    bool isInCMajorScale(int note)
    {
        int noteInOctave = note % 12;
        return noteInOctave == 0 || noteInOctave == 2 || noteInOctave == 4 ||
            noteInOctave == 5 || noteInOctave == 7 || noteInOctave == 9 || noteInOctave == 11; // C D E F G A B
    }

    int snapToCMajor(float midiNote)
    {
        int nearest = static_cast<int>(roundf(midiNote));
        while (!isInCMajorScale(nearest) && nearest > 0 && nearest < 127)
        {
            if (midiNote > nearest) nearest++;
            else nearest--;
        }
        return nearest;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutotuneAudioProcessor)
};