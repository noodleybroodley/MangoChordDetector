#pragma once

#include <JuceHeader.h>
#include <vector>
#include "NoteDetector.h"

struct DetectedChord
{
    juce::String root;           // Root note name (e.g., "C")
    juce::String chordType;      // Type (e.g., "Major", "Minor", "7")
    juce::String fullName;       // Full chord name (e.g., "C Major")
    float confidence;            // How certain we are about this chord
    std::vector<int> noteIntervals; // Intervals from root (semitones)
};

class ChordDetector
{
public:
    ChordDetector() = default;

    // Detect chord from list of detected notes
    DetectedChord detectChord (const std::vector<DetectedNote>& notes)
    {
        DetectedChord result { "", "Unknown", "Unknown", 0.0f, {} };

        if (notes.empty())
            return result;

        if (notes.size() == 1)
            return identifySingleNote (notes[0]);

        // Normalize notes relative to lowest note (root)
        std::vector<int> normalizedIntervals;
        int rootMidiNote = notes[0].midiNote;

        for (const auto& note : notes)
        {
            int interval = (note.midiNote - rootMidiNote) % 12;
            normalizedIntervals.push_back (interval);
        }

        // Calculate confidence as average of all notes
        float totalConfidence = 0.0f;
        for (const auto& note : notes)
            totalConfidence += note.confidence;
        float avgConfidence = totalConfidence / notes.size();

        // Identify chord by intervals
        std::sort (normalizedIntervals.begin(), normalizedIntervals.end());
        auto chordInfo = identifyChordByIntervals (normalizedIntervals);

        juce::String rootName = getMidiNoteName (rootMidiNote);

        result.root = rootName;
        result.chordType = chordInfo.type;
        result.fullName = rootName + " " + chordInfo.type;
        result.confidence = avgConfidence;
        result.noteIntervals = normalizedIntervals;

        return result;
    }

private:
    struct ChordInfo
    {
        juce::String type;
        float confidence;
    };

    // Identify chord from interval pattern
    ChordInfo identifyChordByIntervals (const std::vector<int>& intervals)
    {
        if (intervals.size() < 2)
            return { "Single Note", 1.0f };

        // Create interval set for pattern matching
        std::vector<bool> intervalSet (12, false);
        for (int interval : intervals)
            intervalSet[interval] = true;

        // Major chord: root, major 3rd (4 semitones), perfect 5th (7 semitones)
        if (intervalSet[4] && intervalSet[7])
            return { "Major", 0.95f };

        // Minor chord: root, minor 3rd (3 semitones), perfect 5th (7 semitones)
        if (intervalSet[3] && intervalSet[7])
            return { "Minor", 0.95f };

        // Dominant 7th: root, major 3rd, perfect 5th, minor 7th (10 semitones)
        if (intervalSet[4] && intervalSet[7] && intervalSet[10])
            return { "Dominant 7th", 0.90f };

        // Major 7th: root, major 3rd, perfect 5th, major 7th (11 semitones)
        if (intervalSet[4] && intervalSet[7] && intervalSet[11])
            return { "Major 7th", 0.90f };

        // Minor 7th: root, minor 3rd, perfect 5th, minor 7th (10 semitones)
        if (intervalSet[3] && intervalSet[7] && intervalSet[10])
            return { "Minor 7th", 0.90f };

        // Augmented: root, major 3rd, augmented 5th (8 semitones)
        if (intervalSet[4] && intervalSet[8])
            return { "Augmented", 0.85f };

        // Diminished: root, minor 3rd, diminished 5th (6 semitones)
        if (intervalSet[3] && intervalSet[6])
            return { "Diminished", 0.85f };

        // Sus4: root, perfect 4th (5 semitones), perfect 5th (7 semitones)
        if (intervalSet[5] && intervalSet[7])
            return { "Sus4", 0.80f };

        // Sus2: root, major 2nd (2 semitones), perfect 5th (7 semitones)
        if (intervalSet[2] && intervalSet[7])
            return { "Sus2", 0.80f };

        // Power chord: root, perfect 5th (7 semitones) only
        if (intervalSet[7] && intervals.size() == 2)
            return { "Power Chord", 0.85f };

        // Ambiguous - just list the intervals
        return { "Complex Chord", 0.50f };
    }

    // Single note detection
    DetectedChord identifySingleNote (const DetectedNote& note)
    {
        DetectedChord result;
        result.root = note.noteName;
        result.chordType = "Single Note";
        result.fullName = note.noteName;
        result.confidence = note.confidence;
        return result;
    }

    // Get MIDI note name
    juce::String getMidiNoteName (int midiNote)
    {
        const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        int octave = (midiNote / 12) - 1;
        int noteIndex = midiNote % 12;

        return juce::String (noteNames[noteIndex]) + juce::String (octave);
    }
};
