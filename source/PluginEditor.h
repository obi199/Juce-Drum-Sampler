/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "waveFormEditor.h"
#include "DragAndDropButton.h"
#include "sliderController.h"

//==============================================================================
/**Drum Sampler Plugin
*/


//main component//

class DrumSamplerAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    DrumSamplerAudioProcessorEditor (DrumSamplerAudioProcessor&);
    ~DrumSamplerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void ButtonClicked(juce::Button* button, int noteNumber);
    void switchTopad(int padIndex);
    void timerCallback() override;
    juce::File audioFile;

private:
    DrumSamplerAudioProcessor& audioProcessor;

    // 16 pad buttons created dynamically
    std::vector<std::unique_ptr<DragAndDropButton>> padButtons;

    controlSlidersBlock CBlock{ audioProcessor };
    waveFormEditor waveComponent{ audioProcessor };
    positionLine position{ audioProcessor };
    startLine start{ audioProcessor };
    ADSROverlay adsrOverlay{ audioProcessor };

    int lastDisplayedPadIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumSamplerAudioProcessorEditor)
};



