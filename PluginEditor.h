/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "WaveFormEditor.h"
#include "DragAndDropButton.h"
#include "sliderController.h"

//==============================================================================
/**Drum Sampler Plugin
*/

//main component//

class DrumSamplerAudioProcessorEditor  : public juce::AudioProcessorEditor
    
    
{
public:
    DrumSamplerAudioProcessorEditor (DrumSamplerAudioProcessor&);
    ~DrumSamplerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void ButtonClicked(juce::Button* button, int noteNumber);    
    juce::File audioFile;
   
   
private:

    DrumSamplerAudioProcessor& audioProcessor;
    DragAndDropButton myButton { audioProcessor };
    waveFormEditor waveForm{ audioProcessor };
    controlSlidersBlock CBlock{ audioProcessor };



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumSamplerAudioProcessorEditor)
};



