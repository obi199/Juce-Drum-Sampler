/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class DrumSamplerAudioProcessorEditor  : public juce::AudioProcessorEditor,
    public juce::FileDragAndDropTarget
{
public:
    DrumSamplerAudioProcessorEditor (DrumSamplerAudioProcessor&);
    ~DrumSamplerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    juce::TextButton SButton;
    bool FileDragAndDropTarget::isInterestedInFileDrag (const juce::StringArray& files);
    void FileDragAndDropTarget::filesDropped (const StringArray& files, int x, int y);


private:

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    DrumSamplerAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumSamplerAudioProcessorEditor)
};


