#pragma once

#include <JuceHeader.h>

class AutoTuneAudioProcessor : public juce::AudioProcessor
{
public:
    AutoTuneAudioProcessor();
    ~AutoTuneAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;
    void applyPitchShift(float* channelData, int numSamples, float pitchShiftRatio);
  
    float targetPitch = 440.0f; 

private:
   

    float detectPitch(const float* audioData, int numSamples, double sampleRate);
    float correctPitch(float detectedPitch, float targetPitch);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoTuneAudioProcessor)
};
