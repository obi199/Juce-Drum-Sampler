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

class DragAndDropButton : public juce::TextButton,
                          public juce::FileDragAndDropTarget,
                          public juce::DragAndDropTarget
{
public:
    DragAndDropButton(DrumSamplerAudioProcessor&, int m, juce::String name); //insert midi notes for 2nd pad
    ~DragAndDropButton() override;
    void paint(juce::Graphics&) override;

    // File drag-and-drop (from OS)
    bool isInterestedInFileDrag(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray&, int, int) override;

    // Pad-to-pad drag-and-drop
    bool isInterestedInDragSource(const SourceDetails& details) override;
    void itemDropped(const SourceDetails& details) override;
    void itemDragEnter(const SourceDetails&) override;
    void itemDragExit(const SourceDetails&) override;

    // Initiate drag on mouse drag
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    std::function<void()> onFileDropped;

    int getMidiNote() const { return midiNote; }
    void clearSample() { filename.clear(); repaint(); }

private:
    DrumSamplerAudioProcessor& Processor;
    juce::String filename;
    int midiNote;
    juce::String buttonName;
    bool dragHighlight = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DragAndDropButton)
};