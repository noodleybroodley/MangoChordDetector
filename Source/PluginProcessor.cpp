#include "PluginProcessor.h"
#include "PluginEditor.h"

PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties()
          .withInput ("Input", juce::AudioChannelSet::stereo())
          .withOutput ("Output", juce::AudioChannelSet::stereo()))
{
    noteDetector.setPeakThreshold (peakThreshold);
}

PluginProcessor::~PluginProcessor()
{
}

void PluginProcessor::prepareToPlay (double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    noteDetector.setPeakThreshold (peakThreshold);
}

void PluginProcessor::releaseResources()
{
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Process input channel 0
    if (buffer.getNumChannels() > 0)
    {
        auto* channelData = buffer.getReadPointer (0);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            pushSampleIntoFifo (channelData[i]);
    }

    // Copy input to output (pass-through)
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Store peak threshold in state
    juce::MemoryOutputStream stream (destData, true);
    stream.writeFloat (peakThreshold);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Restore peak threshold from state
    juce::MemoryInputStream stream (data, (size_t)sizeInBytes, false);
    peakThreshold = stream.readFloat();
    noteDetector.setPeakThreshold (peakThreshold);
}

std::vector<DetectedNote> PluginProcessor::getDetectedNotes() const
{
    juce::SpinLock::ScopedLockType lock (notesLock);
    return lastDetectedNotes;
}

DetectedChord PluginProcessor::getDetectedChord() const
{
    juce::SpinLock::ScopedLockType lock (notesLock);
    return lastDetectedChord;
}

void PluginProcessor::setPeakThreshold (float threshold)
{
    peakThreshold = threshold;
    noteDetector.setPeakThreshold (threshold);
}

void PluginProcessor::pushSampleIntoFifo (float sample) noexcept
{
    if (fifoIndex == FFT_SIZE)
    {
        if (!nextFFTBlockReady)
        {
            std::fill (fftData.begin(), fftData.end(), 0.0f);
            std::copy (fifo.begin(), fifo.end(), fftData.begin());
            nextFFTBlockReady = true;
            performFFTAnalysis();
        }
        fifoIndex = 0;
    }
    fifo[(size_t)fifoIndex++] = sample;
}

void PluginProcessor::performFFTAnalysis()
{
    // Apply windowing
    noteDetector.applyWindow (fftData.data(), FFT_SIZE);

    // Perform FFT
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    // Detect notes
    auto detectedNotes = noteDetector.detectNotes (fftData.data(), FFT_SIZE / 2, sampleRate);

    // Detect chord
    auto detectedChord = chordDetector.detectChord (detectedNotes);

    // Update in thread-safe manner
    {
        juce::SpinLock::ScopedLockType lock (notesLock);
        lastDetectedNotes = detectedNotes;
        lastDetectedChord = detectedChord;
    }

    nextFFTBlockReady = false;
}

// Plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
