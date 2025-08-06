/*
  ==============================================================================

    waveFormEditor.cpp
    Created: 17 Aug 2024 3:05:24pm
    Author:  obi

  ==============================================================================
*/

#include <JuceHeader.h>
#include "waveFormEditor.h"


//==============================================================================
waveFormEditor::waveFormEditor(DrumSamplerAudioProcessor& p)
    : Processor(p)
{
    Processor.thumbnail.addChangeListener(this);
    setWantsKeyboardFocus(true);
}

waveFormEditor::~waveFormEditor()
{
}

void waveFormEditor::paint(juce::Graphics& g) {

    if (Processor.thumbnail.getNumChannels() == 0)
        paintIfNoFileLoaded(g);
    else {
        paintIfFileLoaded(g);
        g.setColour(juce::Colours::green);
        //g.drawLine(lengthLineX, 0, lengthLineX, getHeight(), 4.5f);
    }
}

void waveFormEditor::paintIfNoFileLoaded(juce::Graphics& g)
{
    g.setColour(juce::Colours::darkgrey);
    g.fillAll(juce::Colours::white);
    g.setColour(juce::Colours::red);
    g.drawFittedText("No File Loaded", getLocalBounds(), juce::Justification::centred, 1);
}

void waveFormEditor::paintIfFileLoaded(juce::Graphics& g)
{
    g.setColour(juce::Colours::white);
    g.fillAll(juce::Colours::white);

    g.setColour(juce::Colours::red);                               
    auto audioLength = (float)Processor.thumbnail.getTotalLength();

    Processor.thumbnail.drawChannels(g,                                      
        getLocalBounds(),
        0.0,                                    // start time
        audioLength,             // end time
        1.0f);                                  // vertical zoom
}

void waveFormEditor::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &Processor.thumbnail)  thumbnailChanged();
}

void waveFormEditor::thumbnailChanged()
{
    repaint();
}

//
//Position line Class==============================================================================

positionLine::positionLine(DrumSamplerAudioProcessor& p) : Processor(p)
{
    startTimer(30);
}

positionLine::~positionLine()
{
    stopTimer();
}

void positionLine::paint(juce::Graphics& g)
{
    //g.fillAll(juce::Colours::red);
    auto audioLength = (float)Processor.thumbnail.getTotalLength();
    auto audioPosition = (float)Processor.getPosInSec();
    if (audioLength > 0.0) {
        //toFront(true);
        auto drawPosition = (audioPosition / audioLength) * (float)getWidth();
        g.setColour(juce::Colours::red);
        g.drawLine(drawPosition, 0.0f, drawPosition,
            (float)getHeight(), 1.0f);
       
    }
}

void positionLine::timerCallback()
{
    repaint();
}




//Start Line Class==============================================================================
startLine::startLine(DrumSamplerAudioProcessor& p) : Processor(p)
{

}

startLine::~startLine()
{

}

void startLine::paint(juce::Graphics& g)
{
    
    g.setColour(juce::Colours::black);
    g.drawLine(lengthLineX, 0.0f, lengthLineX,(float)getHeight(), 3.0f);

}


void startLine::mouseDrag(const juce::MouseEvent& event)
{
    //Update the position of the length line based on mouse drag
    if (isMouseOver()) DBG("mouseOver");
    //lengthLineX = event.x;
    lengthLineX = juce::jlimit(0.0f, (float)getWidth(), (float)event.x);
    DBG("lengthLineX=" << lengthLineX);
    auto audioLength = (float)Processor.thumbnail.getTotalLength();
    DBG("audioLength=" << audioLength);
    newPositionInSeconds = (lengthLineX / (float)getWidth()) * audioLength;
    DBG("newPositionInSeconds=" << newPositionInSeconds);
    Processor.newPositionSec = newPositionInSeconds;
    //Optionally, update the sample length based on the new position
   //updateSampleLength();
    repaint();
}

