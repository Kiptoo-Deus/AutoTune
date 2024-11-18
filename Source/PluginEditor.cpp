#include "PluginProcessor.h"
#include "PluginEditor.h"

AutoTuneAudioProcessorEditor::AutoTuneAudioProcessorEditor(AutoTuneAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
    pitchCorrectionAttachment(p.parameters, "pitchCorrection", pitchCorrectionSlider),
    formantShiftAttachment(p.parameters, "formantShift", formantShiftSlider),
    vibratoDepthAttachment(p.parameters, "vibratoDepth", vibratoDepthSlider),
    vibratoRateAttachment(p.parameters, "vibratoRate", vibratoRateSlider)
{
    addAndMakeVisible(pitchCorrectionSlider);
    addAndMakeVisible(formantShiftSlider);
    addAndMakeVisible(vibratoDepthSlider);
    addAndMakeVisible(vibratoRateSlider);

    addAndMakeVisible(detectedPitchLabel);
    detectedPitchLabel.setText("Detected Pitch: ", juce::dontSendNotification);
    detectedPitchLabel.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(targetPitchSlider);
    targetPitchSlider.setRange(20.0, 2000.0); // Pitch range in Hz
    targetPitchSlider.setValue(440.0); // Default to A4
    targetPitchSlider.onValueChange = [this] {
        audioProcessor.targetPitch = targetPitchSlider.getValue();
    };

    setSize(400, 300);
}

AutoTuneAudioProcessorEditor::~AutoTuneAudioProcessorEditor() {}

void AutoTuneAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::white);
    g.setFont(15.0f);
    g.drawFittedText("AutoTune Plugin", getLocalBounds(), juce::Justification::centred, 1);
}

void AutoTuneAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    pitchCorrectionSlider.setBounds(area.removeFromTop(50));
    formantShiftSlider.setBounds(area.removeFromTop(50));
    vibratoDepthSlider.setBounds(area.removeFromTop(50));
    vibratoRateSlider.setBounds(area.removeFromTop(50));
    detectedPitchLabel.setBounds(10, 10, 150, 20);
    targetPitchSlider.setBounds(10, 40, 150, 20);

}
