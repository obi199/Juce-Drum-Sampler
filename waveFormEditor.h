/*
  ==============================================================================

    waveFormEditor.h
    Created: 17 Aug 2024 3:05:24pm
    Author:  obi

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/*
*/
/*class waveFormEditor  : public juce::Component
{
public:
    waveFormEditor();
    ~waveFormEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (waveFormEditor)
};*/


class waveFormEditor : public juce::Component, private juce::ChangeListener, 
    private juce::Timer
{
public:
    waveFormEditor::waveFormEditor(DrumSamplerAudioProcessor&);
    ~waveFormEditor() override;
    void paint(juce::Graphics&) override;
    void paintIfNoFileLoaded(juce::Graphics&, const juce::Rectangle<int>&);
    void paintIfFileLoaded(juce::Graphics&, const juce::Rectangle<int>&);
    void thumbnailChanged();
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    void resized() override;
    void timerCallback() override;
   

private: 
    DrumSamplerAudioProcessor& Processor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(waveFormEditor)
};