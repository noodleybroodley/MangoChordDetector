#pragma once

#include <JuceHeader.h>
#include <vector>
#include <cmath>
#include <algorithm>

struct DetectedNote
{
    int midiNote;          // 0-127 MIDI note number
    float confidence;      // 0.0-1.0, relative strength
    double frequency;      // Hz
    juce::String noteName; // "C4", "G#3", etc.

    bool operator<(const DetectedNote& other) const
    {
        return confidence > other.confidence; // Sort by confidence descending
    }
};

class NoteDetector
{
public:
    NoteDetector(int fftSize = 1024)
        : fftSize(fftSize),
          window(fftSize, juce::dsp::WindowingFunction<float>::hann),
          peakThreshold(0.01f),
          maxNotesToDetect(10)
    {
    }

    // Main detection function - called after FFT
    std::vector<DetectedNote> detectNotes(const float* spectrum, int spectrumSize, double sampleRate)
    {
        std::vector<DetectedNote> detectedNotes;

        if (sampleRate <= 0 || spectrum == nullptr)
            return detectedNotes;

        // Find the maximum magnitude to normalize
        float maxMagnitude = 0.0f;
        for (int i = 0; i < spectrumSize; ++i)
            maxMagnitude = std::max(maxMagnitude, std::abs(spectrum[i]));

        if (maxMagnitude < 1e-7f)
            return detectedNotes; // Silent or very quiet

        // Find local peaks in the spectrum
        std::vector<PeakInfo> peaks;
        for (int i = 1; i < spectrumSize - 1; ++i)
        {
            float magnitude = std::abs(spectrum[i]);
            float normalizedMagnitude = magnitude / maxMagnitude;

            // Local maximum test
            if (magnitude > std::abs(spectrum[i - 1]) &&
                magnitude > std::abs(spectrum[i + 1]) &&
                normalizedMagnitude > peakThreshold)
            {
                // Interpolate peak position for sub-bin accuracy
                double peakBin = interpolatePeakBin(spectrum, i);
                double frequency = peakBinToFrequency(peakBin, sampleRate);

                // Only consider frequencies in musical range (27.5 Hz = A0 to 4186 Hz = C8)
                if (frequency >= 20.0 && frequency <= 5000.0)
                {
                    peaks.push_back({ peakBin, magnitude, normalizedMagnitude, frequency });
                }
            }
        }

        // Sort peaks by magnitude, take top N
        std::sort(peaks.begin(), peaks.end(),
                  [](const PeakInfo& a, const PeakInfo& b) { return a.magnitude > b.magnitude; });

        int numPeaks = std::min((int)peaks.size(), maxNotesToDetect);
        for (int i = 0; i < numPeaks; ++i)
        {
            int midiNote = frequencyToMidiNote(peaks[i].frequency);
            juce::String noteName = midiNoteToName(midiNote);

            detectedNotes.push_back({
                midiNote,
                peaks[i].normalizedMagnitude,
                peaks[i].frequency,
                noteName
            });
        }

        // Sort by MIDI note (lowest to highest)
        std::sort(detectedNotes.begin(), detectedNotes.end(),
                  [](const DetectedNote& a, const DetectedNote& b) { return a.midiNote < b.midiNote; });

        return detectedNotes;
    }

    // Apply windowing to audio buffer (call before FFT)
    void applyWindow(float* buffer, int numSamples)
    {
        if (numSamples != fftSize)
            return;

        window.multiplyWithWindowingTable(buffer, numSamples);
    }

    // Configuration
    void setPeakThreshold(float threshold)
    {
        peakThreshold = juce::jlimit(0.001f, 0.5f, threshold);
    }

    float getPeakThreshold() const { return peakThreshold; }

    void setMaxNotesToDetect(int maxNotes)
    {
        maxNotesToDetect = juce::jlimit(1, 20, maxNotes);
    }

    int getMaxNotesToDetect() const { return maxNotesToDetect; }

private:
    struct PeakInfo
    {
        double peakBin;
        float magnitude;
        float normalizedMagnitude;
        double frequency;
    };

    int fftSize;
    juce::dsp::WindowingFunction<float> window;
    float peakThreshold;
    int maxNotesToDetect;

    // Convert FFT bin index to frequency in Hz
    double peakBinToFrequency(double bin, double sampleRate)
    {
        return (bin * sampleRate) / fftSize;
    }

    // Interpolate peak position using parabolic fitting
    // Provides sub-bin accuracy by fitting a parabola through 3 points
    double interpolatePeakBin(const float* spectrum, int peakBin)
    {
        float left = std::abs(spectrum[peakBin - 1]);
        float center = std::abs(spectrum[peakBin]);
        float right = std::abs(spectrum[peakBin + 1]);

        float denom = 2.0f * (left - 2.0f * center + right);
        if (std::abs(denom) < 1e-7f)
            return peakBin; // No interpolation possible

        float delta = (right - left) / denom;
        return peakBin + delta;
    }

    // Convert frequency (Hz) to MIDI note number (0-127)
    // Uses equal temperament tuning with A4 = 440 Hz
    int frequencyToMidiNote(double frequency)
    {
        const double A4_FREQUENCY = 440.0;
        const int A4_MIDI = 69;

        if (frequency <= 0)
            return 0;

        double semitones = 12.0 * std::log2(frequency / A4_FREQUENCY);
        int midiNote = A4_MIDI + static_cast<int>(std::round(semitones));

        return juce::jlimit(0, 127, midiNote);
    }

    // Convert MIDI note number to note name (e.g., "C4", "G#3")
    juce::String midiNoteToName(int midiNote)
    {
        const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        int octave = (midiNote / 12) - 1;
        int noteIndex = midiNote % 12;

        return juce::String(noteNames[noteIndex]) + juce::String(octave);
    }
};
