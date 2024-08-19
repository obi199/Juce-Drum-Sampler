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
    DragAndDropButton::DragAndDropButton(DrumSamplerAudioProcessor&);
    ~DragAndDropButton() override;
    void paint(juce::Graphics&) override;

    bool FileDragAndDropTarget::isInterestedInFileDrag(const juce::StringArray&);

    void FileDragAndDropTarget::filesDropped(const juce::StringArray&, int, int);


private:
    DrumSamplerAudioProcessor& Processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DragAndDropButton)
};


