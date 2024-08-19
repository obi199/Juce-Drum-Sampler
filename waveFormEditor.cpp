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



waveFormEditor::waveFormEditor(DrumSamplerAudioProcessor& p): Processor(p)
{
    Processor.thumbnail.addChangeListener(this);
}


waveFormEditor::~waveFormEditor()
{
}


void waveFormEditor::paint(juce::Graphics& g) {

    juce::Rectangle<int> thumbnailBounds(10, 100, getWidth() - 20, getHeight() - 120);

    if (Processor.thumbnail.getNumChannels() == 0)
        paintIfNoFileLoaded(g, thumbnailBounds);
    else
        paintIfFileLoaded(g, thumbnailBounds);

}

void waveFormEditor::paintIfNoFileLoaded(juce::Graphics& g, const juce::Rectangle<int>& thumbnailBounds)
{
    g.setColour(juce::Colours::darkgrey);
    g.fillRect(thumbnailBounds);
    g.setColour(juce::Colours::white);
    g.drawFittedText("No File Loaded", thumbnailBounds, juce::Justification::centred, 1);
}

void waveFormEditor::paintIfFileLoaded(juce::Graphics& g, const juce::Rectangle<int>& thumbnailBounds)
{
    g.setColour(juce::Colours::white);
    g.fillRect(thumbnailBounds);

    g.setColour(juce::Colours::red);                               

    Processor.thumbnail.drawChannels(g,                                      
        thumbnailBounds,
        0.0,                                    // start time
        Processor.thumbnail.getTotalLength(),             // end time
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


void waveFormEditor::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..

}




