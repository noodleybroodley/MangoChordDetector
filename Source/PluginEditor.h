#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "MusicalDisplay.h"

// Simple spectrogram visualization for plugin UI
class SimpleSpectrogramComponent : public juce::Component
{
public:
    SimpleSpectrogramComponent()
        : spectrogramImage (juce::Image::RGB, 300, 200, true)
    {
    }

    void setFFTData (const float* data, int size)
    {
        if (data == nullptr)
            return;

        // Shift image left
        int rightEdge = spectrogramImage.getWidth() - 1;
        spectrogramImage.moveImageSection (0, 0, 1, 0, rightEdge, spectrogramImage.getHeight());

        // Find max for scaling
        float maxLevel = 0.0f;
        for (int i = 0; i < size; ++i)
            maxLevel = std::max (maxLevel, std::abs (data[i]));

        if (maxLevel < 1e-7f)
            maxLevel = 1e-5f;

        // Draw spectrum line
        juce::Image::BitmapData bitmap (spectrogramImage, rightEdge, 0, 1, spectrogramImage.getHeight(),
                                        juce::Image::BitmapData::writeOnly);
        for (int y = 1; y < spectrogramImage.getHeight(); ++y)
        {
            float skewedProportionY = 1.0f - std::exp (std::log ((float)y / (float)spectrogramImage.getHeight()) * 0.2f);
            int fftDataIndex = (int)juce::jlimit (0, size, (int)(skewedProportionY * size));
            float level = juce::jmap (std::abs (data[fftDataIndex]), 0.0f, maxLevel, 0.0f, 1.0f);
            bitmap.setPixelColour (0, y, juce::Colour::fromHSV (level, 1.0f, level, 1.0f));
        }

        repaint();
    }

    void paint (juce::Graphics& g) override { g.drawImageAt (spectrogramImage, 0, 0); }

    void resized() override {}

private:
    juce::Image spectrogramImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleSpectrogramComponent)
};

//==============================================================================
class PluginEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    PluginEditor (PluginProcessor& p);
    ~PluginEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void timerCallback() override;

private:
    PluginProcessor& processor;

    SimpleSpectrogramComponent spectrogram;
    MusicalDisplayComponent musicalDisplay;
    juce::Slider thresholdSlider;
    juce::Label thresholdLabel;
    juce::Label chordDisplayLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
