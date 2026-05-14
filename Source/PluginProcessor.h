#pragma once

#include <JuceHeader.h>
#include "NoteDetector.h"
#include "ChordDetector.h"

class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MangoChordDetector"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Note detection interface
    std::vector<DetectedNote> getDetectedNotes() const;
    DetectedChord getDetectedChord() const;

    // FFT visualization data (thread-safe access)
    void getFFTData (float* dest, int maxSize) const;

    void setPeakThreshold (float threshold);
    float getPeakThreshold() const { return peakThreshold; }

private:
    NoteDetector noteDetector { 1024 };
    ChordDetector chordDetector;

    static constexpr int FFT_SIZE = 1024;
    static constexpr int FFT_ORDER = 10;

    juce::dsp::FFT fft { FFT_ORDER };
    std::array<float, FFT_SIZE> fifo {};
    std::array<float, FFT_SIZE * 2> fftData {};
    int fifoIndex = 0;
    bool nextFFTBlockReady = false;

    std::vector<DetectedNote> lastDetectedNotes;
    DetectedChord lastDetectedChord;
    mutable juce::SpinLock notesLock;

    float peakThreshold = 0.02f;
    double sampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
