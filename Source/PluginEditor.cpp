/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

AutotuneAudioProcessorEditor::AutotuneAudioProcessorEditor(AutotuneAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), displayedPitch(0.0f)
{
    startTimer(100); // Update every 100ms

    retuneSpeedSlider.setRange(0.0, 1.0, 0.01);
    retuneSpeedSlider.setValue(0.1);
    addAndMakeVisible(retuneSpeedSlider);

    pitchLabel.setText("Pitch: 0 Hz", juce::dontSendNotification);
    addAndMakeVisible(pitchLabel);

    setSize(400, 300);
}

AutotuneAudioProcessorEditor::~AutotuneAudioProcessorEditor()
{
    stopTimer();
}

void AutotuneAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    g.setColour(juce::Colours::white);
    g.setFont(15.0f);
    g.drawText("Autotune", 10, 10, 200, 20, juce::Justification::left);
}

void AutotuneAudioProcessorEditor::resized()
{
    retuneSpeedSlider.setBounds(10, 40, 380, 20);
    pitchLabel.setBounds(10, 70, 380, 20);
}

void AutotuneAudioProcessorEditor::timerCallback()
{
    displayedPitch = audioProcessor.previousPitch; 
    pitchLabel.setText("Pitch: " + juce::String(displayedPitch, 1) + " Hz", juce::dontSendNotification);
}