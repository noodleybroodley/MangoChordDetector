/*
  ==============================================================================

    VST3 Plugin for real-time note and chord detection using FFT analysis.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"

// Plugin metadata
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
