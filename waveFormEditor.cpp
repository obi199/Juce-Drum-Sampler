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
/*waveFormEditor::waveFormEditor()
{
    // In your constructor, you should add any child components, and
    // initialise any special settings that your component needs.

}*/



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


/*
void waveFormEditor::paint (juce::Graphics& g)
{
   
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));   // clear the background

    g.setColour (juce::Colours::grey);
    g.drawRect (getLocalBounds(), 1);   // draw an outline around the component

    g.setColour (juce::Colours::white);
    g.setFont (14.0f);
    g.drawText ("waveFormEditor", getLocalBounds(),
                juce::Justification::centred, true);   // draw some placeholder text
}*/

