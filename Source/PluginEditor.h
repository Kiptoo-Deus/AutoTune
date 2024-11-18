#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class AutoTuneAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    AutoTuneAudioProcessorEditor(AutoTuneAudioProcessor&);
    ~AutoTuneAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    AutoTuneAudioProcessor& audioProcessor;
    juce::Label detectedPitchLabel;
    juce::Slider targetPitchSlider;

    juce::Slider pitchCorrectionSlider;
    juce::Slider formantShiftSlider;
    juce::Slider vibratoDepthSlider;
    juce::Slider vibratoRateSlider;

    juce::AudioProcessorValueTreeState::SliderAttachment pitchCorrectionAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment formantShiftAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment vibratoDepthAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment vibratoRateAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoTuneAudioProcessorEditor)
};
