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


class waveFormEditor : public juce::Component, private juce::ChangeListener
{
public:
    waveFormEditor::waveFormEditor(DrumSamplerAudioProcessor&);
    ~waveFormEditor() override;
    void paint(juce::Graphics&) override;
    void paintIfNoFileLoaded(juce::Graphics&);
    void paintIfFileLoaded(juce::Graphics&);
    void thumbnailChanged();
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
   

private: 
    DrumSamplerAudioProcessor& Processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(waveFormEditor)
};



class positionLine : public juce::Component,
    private juce::Timer
{
public:
    positionLine(DrumSamplerAudioProcessor&);
    ~positionLine() override;

    void paint(juce::Graphics&) override;
    void timerCallback() override;


private:
    DrumSamplerAudioProcessor& Processor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(positionLine)
};


////Main Wave Component
//class mainWaveComponent : public juce::Component
//{
//public:
//    mainWaveComponent::mainWaveComponent(DrumSamplerAudioProcessor&);
//    mainWaveComponent::~mainWaveComponent();
//
//    void paint(juce::Graphics&) override;
//    void resized() override;
//
//private:
//    DrumSamplerAudioProcessor& Processor;
//    waveFormEditor wave{ Processor };
//    positionLine position{ Processor };
//    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(mainWaveComponent)
// 
//};