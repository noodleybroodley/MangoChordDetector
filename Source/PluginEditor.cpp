#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    addAndMakeVisible (spectrogram);
    addAndMakeVisible (musicalDisplay);
    addAndMakeVisible (thresholdSlider);
    addAndMakeVisible (thresholdLabel);
    addAndMakeVisible (chordDisplayLabel);

    thresholdLabel.setText ("Peak Threshold:", juce::dontSendNotification);
    thresholdLabel.attachToComponent (&thresholdSlider, true);

    thresholdSlider.setRange (0.001, 0.1, 0.001);
    thresholdSlider.setValue (processor.getPeakThreshold());
    thresholdSlider.onValueChange = [this]() {
        processor.setPeakThreshold ((float)thresholdSlider.getValue());
    };

    chordDisplayLabel.setText ("Ready to detect...", juce::dontSendNotification);
    chordDisplayLabel.setColour (juce::Label::textColourId, juce::Colours::white);

    setSize (1000, 700);
    startTimerHz (30); // Update UI 30 times per second
}

PluginEditor::~PluginEditor()
{
    stopTimer();
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void PluginEditor::resized()
{
    auto bounds = getLocalBounds().reduced (5);

    // Top section: chord display and threshold control
    auto topSection = bounds.removeFromTop (40);
    thresholdLabel.setBounds (topSection.removeFromLeft (120));
    thresholdSlider.setBounds (topSection.removeFromLeft (150));
    chordDisplayLabel.setBounds (topSection);

    // Middle-left section: spectrogram
    auto spectroRect = bounds.removeFromLeft (bounds.getWidth() / 2);
    spectrogram.setBounds (spectroRect);

    // Right section: musical display
    musicalDisplay.setBounds (bounds);
}

void PluginEditor::timerCallback()
{
    // Update musical display with current detected notes
    auto detectedNotes = processor.getDetectedNotes();
    musicalDisplay.setDetectedNotes (detectedNotes);

    // Update chord display
    auto detectedChord = processor.getDetectedChord();
    if (!detectedNotes.empty())
    {
        chordDisplayLabel.setText (detectedChord.fullName + " ("
            + juce::String (static_cast<int>(detectedChord.confidence * 100)) + "%)",
            juce::dontSendNotification);
    }
    else
    {
        chordDisplayLabel.setText ("Detecting...", juce::dontSendNotification);
    }
}
