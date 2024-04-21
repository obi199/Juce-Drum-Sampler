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
    public juce::FileDragAndDropTarget, public juce::AudioTransportSource
{
public:
    DrumSamplerAudioProcessorEditor (DrumSamplerAudioProcessor&);
    ~DrumSamplerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    bool FileDragAndDropTarget::isInterestedInFileDrag (const juce::StringArray &files);
    void FileDragAndDropTarget::filesDropped(const juce::StringArray& files, int x, int y);
    bool isMouseOver(juce::TextButton& button);
    void SButtonClicked(juce::Button* button);
    juce::File audioFile;
    juce::AudioTransportSource transportSource;
    enum TransportState
    {
        Stop,
        Play,
    };

    TransportState state;

private:

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    DrumSamplerAudioProcessor& audioProcessor;
    juce::TextButton SButton;


   
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumSamplerAudioProcessorEditor)
};


