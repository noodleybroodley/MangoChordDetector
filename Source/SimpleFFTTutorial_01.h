/*
  ==============================================================================

   This file is part of the JUCE tutorials.
   Copyright (c) 2020 - Raw Material Software Limited

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:             SimpleFFTTutorial
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Displays an FFT spectrogram.

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_processors, juce_audio_utils, juce_core,
                   juce_data_structures, juce_dsp, juce_events, juce_graphics,
                   juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2019, linux_make

 type:             Component
 mainClass:        SpectrogramComponent

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/


#pragma once

#include "NoteDetector.h"

//==============================================================================
class SpectrogramComponent   : public juce::AudioAppComponent,
                               private juce::Timer
{
public:
    SpectrogramComponent()
        : forwardFFT (fftOrder),
          noteDetector (fftSize),
          spectrogramImage (juce::Image::RGB, 512, 512, true),
          sampleRate (44100.0),
          peakThreshold (0.02f),
          showPeaks (true)
    {
        setOpaque (true);
        setAudioChannels (2, 0);  // we want a couple of input channels but no outputs
        startTimerHz (60);
        setSize (700, 500);
    }

    ~SpectrogramComponent() override
    {
        shutdownAudio();
    }

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double newSampleRate) override
    {
        sampleRate = newSampleRate;
        samplesPerBlock = samplesPerBlockExpected;
        noteDetector.setPeakThreshold (peakThreshold);
    }

    void releaseResources() override          {}

    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        if (bufferToFill.buffer->getNumChannels() > 0)
        {
            auto* channelData = bufferToFill.buffer->getReadPointer (0, bufferToFill.startSample);
            for (auto i = 0; i < bufferToFill.numSamples; ++i)
                pushNextSampleIntoFifo (channelData[i]);
        }
    }

    //==============================================================================
    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::black);

        g.setOpacity (1.0f);
        g.drawImage (spectrogramImage, getLocalBounds().toFloat());
    }

    void timerCallback() override
    {
        if (nextFFTBlockReady) // [1]
        {
            drawNextLineOfSpectrogram(); // [2]
            nextFFTBlockReady = false;   // [3]
            repaint();                   // [4]
        }
    }

    void pushNextSampleIntoFifo (float sample) noexcept
    {
        // if the fifo contains enough data, set a flag to say
        // that the next line should now be rendered..
        if (fifoIndex == fftSize) // [8]
        {
            if (!nextFFTBlockReady) // [9]
            {
                std::fill (fftData.begin(), fftData.end(), 0.0f);
                std::copy (fifo.begin(), fifo.end(), fftData.begin());
                nextFFTBlockReady = true;
            }
            fifoIndex = 0;
        }
        fifo[(size_t) fifoIndex++] = sample; // [9]
    }

    void drawNextLineOfSpectrogram()
    {
        auto rightHandEdge = spectrogramImage.getWidth() - 1;
        auto imageHeight = spectrogramImage.getHeight();
        // first, shuffle our image leftwards by 1 pixel..
        spectrogramImage.moveImageSection (0, 0, 1, 0, rightHandEdge, imageHeight); // [1]

        // Apply windowing before FFT to reduce spectral leakage
        noteDetector.applyWindow (fftData.data(), fftSize);

        // then render our FFT data..
        forwardFFT.performFrequencyOnlyForwardTransform (fftData.data()); // [2]

        // Detect notes in the spectrum
        auto detectedNotes = noteDetector.detectNotes (fftData.data(), fftSize / 2, sampleRate);

        // Store detected notes in thread-safe buffer for UI display
        {
            juce::SpinLock::ScopedLockType lock (notesLock);
            lastDetectedNotes = detectedNotes;
        }

        // find the range of values produced, so we can scale our rendering to
        // show up the detail clearly
        auto maxLevel = juce::FloatVectorOperations::findMinAndMax (fftData.data(), fftSize / 2); // [3]
        juce::Image::BitmapData bitmap { spectrogramImage, rightHandEdge, 0, 1, imageHeight, juce::Image::BitmapData::writeOnly }; // [4]
        for (auto y = 1; y < imageHeight; ++y) // [5]
        {
            auto skewedProportionY = 1.0f - std::exp (std::log ((float) y / (float) imageHeight) * 0.2f);
            auto fftDataIndex = (size_t) juce::jlimit (0, fftSize / 2, (int) (skewedProportionY * fftSize / 2));
            auto level = juce::jmap (fftData[fftDataIndex], 0.0f, juce::jmax (maxLevel.getEnd(), 1e-5f), 0.0f, 1.0f);
            bitmap.setPixelColour (0, y, juce::Colour::fromHSV (level, 1.0f, level, 1.0f)); // [6]
        }

        // Draw vertical lines at detected note frequencies if enabled
        if (showPeaks && !lastDetectedNotes.empty())
        {
            juce::Graphics g (spectrogramImage);
            g.setColour (juce::Colours::red.withAlpha (0.7f));

            for (const auto& note : lastDetectedNotes)
            {
                // Map frequency to vertical position using same skewing as spectrum
                double binIndex = (note.frequency * fftSize) / (sampleRate * 0.5);
                if (binIndex >= 0 && binIndex < fftSize / 2)
                {
                    // Apply inverse skew transformation to get y position
                    float normalizedBin = static_cast<float>(binIndex) / (fftSize / 2);
                    float skewedY = imageHeight * (1.0f - std::pow (1.0f - normalizedBin, 5.0f));

                    if (skewedY >= 0 && skewedY < imageHeight)
                    {
                        int yPos = static_cast<int>(skewedY);
                        g.drawVerticalLine (rightHandEdge, yPos - 2, yPos + 2);
                    }
                }
            }
        }
    }

    // Get detected notes for UI display (thread-safe)
    std::vector<DetectedNote> getDetectedNotes() const
    {
        juce::SpinLock::ScopedLockType lock (notesLock);
        return lastDetectedNotes;
    }

    // Toggle peak visualization
    void setShowPeaks (bool show) { showPeaks = show; }
    bool getShowPeaks() const { return showPeaks; }
    static constexpr auto fftOrder  = 10;             // [1]
    static constexpr auto fftSize   = 1 << fftOrder;   // [2]

private:
    juce::dsp::FFT forwardFFT { fftOrder };  // [3]
    NoteDetector noteDetector;
    juce::Image spectrogramImage;
    std::array<float, fftSize> fifo {};        // [4]
    std::array<float, fftSize * 2> fftData {}; // [5]
    int fifoIndex = 0;                             // [6]
    bool nextFFTBlockReady = false;            // [7]
    double sampleRate;
    int samplesPerBlock;
    float peakThreshold;
    bool showPeaks;
    std::vector<DetectedNote> lastDetectedNotes;
    mutable juce::SpinLock notesLock;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrogramComponent)
};
