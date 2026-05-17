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
    waveFormEditor(DrumSamplerAudioProcessor&);
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


class startLine : public juce::Component,
    private juce::Timer
{
public:
    startLine(DrumSamplerAudioProcessor&);
    ~startLine() override;

    void paint(juce::Graphics&) override;
    bool hitTest(int x, int y) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void timerCallback() override;  // Poll for parameter changes
    float getStartPosInSec() { return newPositionInSeconds; }
    void setPosition(float positionInSeconds);
    void setNormalizedOffset(float offsetRatio01);

private:
    float lengthLineX = 0.0f;
    float newPositionInSeconds = 0;
    float lastKnobValue = -1.0f;  // Track last seen knob value to detect changes
    DrumSamplerAudioProcessor& Processor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(startLine)
};


class endLine : public juce::Component,
    private juce::Timer
{
public:
    endLine(DrumSamplerAudioProcessor&);
    ~endLine() override;

    void paint(juce::Graphics&) override;
    bool hitTest(int x, int y) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void timerCallback() override;
    void setNormalizedOffset(float offsetRatio01);

private:
    float lengthLineX = 0.0f;
    float lastKnobValue = -1.0f;
    DrumSamplerAudioProcessor& Processor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(endLine)
};


// ADSR envelope overlay — drawn on top of the waveform with draggable handles.
// Each handle writes to the APVTS so the corresponding knob updates automatically.
class ADSROverlay : public juce::Component, private juce::Timer
{
public:
    ADSROverlay(DrumSamplerAudioProcessor&);
    ~ADSROverlay() override;

    void paint(juce::Graphics&) override;
    bool hitTest(int x, int y) override;   // passes events through when not near a handle
    void mouseMove(const juce::MouseEvent&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void timerCallback() override;

private:
    DrumSamplerAudioProcessor& Processor;

    enum class DragHandle { None, Attack, Decay, FadeEnd };
    DragHandle activeDrag = DragHandle::None;

    // APVTS helpers
    float getParam(const juce::String& name);
    void  setParam(const juce::String& name, float value);

    // Pixel positions computed from current APVTS values
    float startX();
    float attackX();
    float decayX();
    float fadeEndX();   // x where the fade finishes (FADE_END param)
    float endX();       // x where playback hard-stops (END_OFFSET param)

    DragHandle hitTestForHandle(float x, float y);

    static constexpr float kHandleRadius = 6.0f;
    static constexpr float kHitTolerance = 10.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ADSROverlay)
};
