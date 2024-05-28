/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**Drum Sampler Plugin
*/

//this is your drag and drop button//
class waveFormEditor : public juce::Component, private juce::ChangeListener {
public:
    waveFormEditor::waveFormEditor(DrumSamplerAudioProcessor&);
    //~waveFormEditor() override;
    void paint(juce::Graphics&) override;
    void paintIfNoFileLoaded(juce::Graphics&, const juce::Rectangle<int>&);
    void paintIfFileLoaded(juce::Graphics&, const juce::Rectangle<int>&);
    void thumbnailChanged();
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
private: 
    DrumSamplerAudioProcessor& Processor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(waveFormEditor)
};




class DragAndDropButton : public juce::TextButton, public juce::FileDragAndDropTarget
{
public:
    DragAndDropButton::DragAndDropButton(DrumSamplerAudioProcessor&);
    ~DragAndDropButton() override;
    void paint(juce::Graphics&) override;

    bool FileDragAndDropTarget::isInterestedInFileDrag(const juce::StringArray& );
   
    void FileDragAndDropTarget::filesDropped(const juce::StringArray& , int , int );


private:
    DrumSamplerAudioProcessor& Processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DragAndDropButton)
};



//main component//

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
    DragAndDropButton myButton { audioProcessor };
    waveFormEditor waveForm{ audioProcessor };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumSamplerAudioProcessorEditor)
};



