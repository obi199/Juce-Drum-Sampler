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


class DragAndDropButton : public juce::TextButton, public juce::FileDragAndDropTarget
{
public:
    //DragAndDropButton();
    //DragAndDropButton(DrumSamplerAudioProcessor&);
    DragAndDropButton::DragAndDropButton(DrumSamplerAudioProcessor&);
    ~DragAndDropButton() override;

    // FileDragAndDropTarget interface methods
    bool FileDragAndDropTarget::isInterestedInFileDrag(const juce::StringArray& files);
   
    void FileDragAndDropTarget::filesDropped(const juce::StringArray& files, int x, int y);


private:
    DrumSamplerAudioProcessor& Processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DragAndDropButton)
};




class DrumSamplerAudioProcessorEditor  : public juce::AudioProcessorEditor
//    public juce::DragAndDropContainer //, juce::FileDragAndDropTarget
{
public:
    DrumSamplerAudioProcessorEditor (DrumSamplerAudioProcessor&);
    ~DrumSamplerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    //bool FileDragAndDropTarget::isInterestedInFileDrag (const juce::StringArray &files);
    //void FileDragAndDropTarget::filesDropped(const juce::StringArray& files, int x, int y);
    void ButtonClicked(juce::Button* button, int noteNumber);    
    juce::File audioFile;
   

private:

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.

    DrumSamplerAudioProcessor& audioProcessor;
    //juce::TextButton SButton;
    //juce::TextButton SButton2;
    DragAndDropButton myButton { audioProcessor };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumSamplerAudioProcessorEditor)
};



