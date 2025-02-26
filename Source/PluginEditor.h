/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class AutotuneAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    AutotuneAudioProcessorEditor(AutotuneAudioProcessor&);
    ~AutotuneAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    AutotuneAudioProcessor& audioProcessor;
    juce::Slider retuneSpeedSlider;
    juce::Label pitchLabel;
    float displayedPitch;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutotuneAudioProcessorEditor)
};
