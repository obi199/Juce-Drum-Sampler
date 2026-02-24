/*
  ==============================================================================

    DragAndDropButton.h
    Created: 19 Aug 2024 10:09:51pm
    Author:  obi

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
///*
//*/

class DragAndDropButton : public juce::TextButton, public juce::FileDragAndDropTarget
{
public:
    DragAndDropButton(DrumSamplerAudioProcessor&, int m, juce::String name); //insert midi notes for 2nd pad
    ~DragAndDropButton() override;
    void paint(juce::Graphics&) override;

    bool isInterestedInFileDrag(const juce::StringArray&) override;

    void filesDropped(const juce::StringArray&, int, int) override;

    //void DragAndDropButton::setMidinote(int m);


private:
    DrumSamplerAudioProcessor& Processor;
    juce::String filename;
    int midiNote;
    juce::String buttonName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DragAndDropButton)
};


