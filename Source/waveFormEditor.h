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
    //float lengthLineX = 10.0f;

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


class startLine : public juce::Component

{
public:
    startLine(DrumSamplerAudioProcessor&);
    ~startLine() override;

    void paint(juce::Graphics&) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    float getStartPosInSec() { return newPositionInSeconds; }


private:
    float lengthLineX = 0.0f;
    float newPositionInSeconds = 0;
    DrumSamplerAudioProcessor& Processor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(startLine)
};
