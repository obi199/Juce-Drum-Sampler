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


///class thumbnail waveform-----------
waveFormEditor::waveFormEditor(DrumSamplerAudioProcessor& p): Processor(p)
{
    Processor.thumbnail.addChangeListener(this);
}

waveFormEditor::~waveFormEditor()
{
}

void waveFormEditor::paint(juce::Graphics& g) {

    /*juce::Rectangle<int> thumbnailBounds(10, 100, getWidth() - 20, getHeight() - 120);*/

    if (Processor.thumbnail.getNumChannels() == 0)
        paintIfNoFileLoaded(g);
    else
        paintIfFileLoaded(g);

}

void waveFormEditor::paintIfNoFileLoaded(juce::Graphics& g)
{
    g.setColour(juce::Colours::darkgrey);
    g.fillAll(juce::Colours::white);
    g.setColour(juce::Colours::white);
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




//Position line class

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
    auto audioLength = (float)Processor.thumbnail.getTotalLength();
    auto audioPosition = (float)Processor.getPosInSec();
    DBG("audioPosition = " << audioPosition << " getPos= "<< Processor.getPosInSec());

    if (audioLength > 0.0) {
        auto drawPosition = (audioPosition / audioLength) * (float)getWidth();
        g.setColour(juce::Colours::red);
        g.drawLine(drawPosition, 0.0f, drawPosition,
            (float)getHeight(), 2.0f);
    }
    //###position line fill
   /* g.setColour(juce::Colours::black.withAlpha(0.2f));
    g.fillRect(1.0, (float)thumbnailBounds.getY(), drawPosition, (float)thumbnailBounds.getHeight());*/

}

void positionLine::timerCallback()
{
    repaint();
}



