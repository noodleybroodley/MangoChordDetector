#pragma once

#include <JuceHeader.h>
#include <vector>
#include "NoteDetector.h"

//==============================================================================
// Draws a 5-line musical staff with note positions
class StaffComponent : public juce::Component
{
public:
    StaffComponent()
        : clef (Treble)
    {
        setSize (400, 150);
    }

    enum Clef { Treble, Bass, Alto };

    void setClef (Clef newClef) { clef = newClef; repaint(); }

    void setDetectedNotes (const std::vector<DetectedNote>& notes)
    {
        detectedNotes = notes;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::white);
        g.setColour (juce::Colours::black);

        int x = 20;
        int y = 30;
        int spacing = 12;
        int lineCount = 5;

        // Draw staff lines
        for (int i = 0; i < lineCount; ++i)
        {
            g.drawLine (x, y + i * spacing, getWidth() - 20, y + i * spacing, 2.0f);
        }

        // Draw clef symbol (simplified as text for now)
        g.setFont (24.0f);
        g.drawText (clef == Treble ? "𝄞" : "𝄢", x + 5, y - 20, 30, 50, juce::Justification::centred);

        // Draw detected notes
        g.setColour (juce::Colours::red);
        for (const auto& note : detectedNotes)
        {
            int notePos = getMidiNoteStaffPosition (note.midiNote);
            float xPos = x + 50 + (note.midiNote % 12) * 15;
            float yPos = y + (lineCount - 1) * spacing / 2.0f - notePos * spacing / 2.0f;

            // Draw note head
            g.fillEllipse (xPos - 4, yPos - 4, 8, 8);

            // Draw confidence as note stem color intensity
            float alpha = 0.3f + note.confidence * 0.7f;
            g.setColour (juce::Colours::red.withAlpha (alpha));
            g.drawLine (xPos + 4, yPos, xPos + 4, yPos - 20, 2.0f);
        }
    }

    void resized() override {}

private:
    Clef clef;
    std::vector<DetectedNote> detectedNotes;

    // Calculate vertical position on staff (-4 to 4 with 0 at middle line)
    int getMidiNoteStaffPosition (int midiNote)
    {
        // For treble clef: middle C is below staff
        int noteInOctave = midiNote % 12;
        int octave = midiNote / 12;

        // C=0, D=2, E=4, F=5, G=7, A=9, B=11 (staff positions: E=4, F=5, G=6, A=7, B=8, C=9, D=10)
        const int staffPositions[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
        return staffPositions[noteInOctave] + (octave - 4) * 7;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StaffComponent)
};

//==============================================================================
// Displays detected note names with confidence meters
class NoteDisplayComponent : public juce::Component
{
public:
    NoteDisplayComponent()
    {
        setSize (200, 300);
    }

    void setDetectedNotes (const std::vector<DetectedNote>& notes)
    {
        detectedNotes = notes;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::darkGrey);
        g.setColour (juce::Colours::white);

        if (detectedNotes.empty())
        {
            g.setFont (16.0f);
            g.drawText ("No notes detected", getLocalBounds(), juce::Justification::centred);
            return;
        }

        g.setFont (14.0f);
        int yOffset = 10;
        int lineHeight = 40;

        for (size_t i = 0; i < detectedNotes.size() && i < 8; ++i)
        {
            const auto& note = detectedNotes[i];
            int yPos = yOffset + i * lineHeight;

            // Draw note name
            g.setColour (juce::Colours::white);
            g.drawText (note.noteName, 10, yPos, 80, 20, juce::Justification::left);

            // Draw frequency
            g.setColour (juce::Colours::lightGrey);
            g.setFont (11.0f);
            juce::String freqStr = juce::String (note.frequency, 1) + " Hz";
            g.drawText (freqStr, 10, yPos + 18, 80, 14, juce::Justification::left);

            // Draw confidence bar
            g.setColour (juce::Colours::darkGrey.brighter());
            float barWidth = 80.0f * note.confidence;
            g.fillRect (100.0f, (float)yPos + 4, barWidth, 12.0f);
            g.setColour (juce::Colours::white);
            g.drawRect (100.0f, (float)yPos + 4, 80.0f, 12.0f, 1.0f);

            // Draw confidence percentage
            g.setColour (juce::Colours::white);
            g.setFont (11.0f);
            int confidencePercent = static_cast<int>(note.confidence * 100);
            g.drawText (juce::String (confidencePercent) + "%", 185, yPos + 2, 30, 16, juce::Justification::right);
        }
    }

    void resized() override {}

private:
    std::vector<DetectedNote> detectedNotes;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoteDisplayComponent)
};

//==============================================================================
// Container component that combines staff and note display
class MusicalDisplayComponent : public juce::Component
{
public:
    MusicalDisplayComponent()
    {
        addAndMakeVisible (staffComponent);
        addAndMakeVisible (noteDisplayComponent);
        setSize (600, 400);
    }

    void setDetectedNotes (const std::vector<DetectedNote>& notes)
    {
        staffComponent.setDetectedNotes (notes);
        noteDisplayComponent.setDetectedNotes (notes);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        // Top half: staff
        staffComponent.setBounds (bounds.removeFromTop (getHeight() / 2).reduced (10));

        // Bottom half: note display
        noteDisplayComponent.setBounds (bounds.reduced (10));
    }

private:
    StaffComponent staffComponent;
    NoteDisplayComponent noteDisplayComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MusicalDisplayComponent)
};
