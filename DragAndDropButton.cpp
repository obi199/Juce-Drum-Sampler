/*
  ==============================================================================

    DragAndDropButton.cpp
    Created: 19 Aug 2024 10:09:51pm
    Author:  obi

  ==============================================================================
*/

#include <JuceHeader.h>
#include "DragAndDropButton.h"

//==============================================================================

//this is your drag and drop button//
DragAndDropButton::DragAndDropButton(DrumSamplerAudioProcessor& p, int m, juce::String name) : Processor(p) {
    midiNote = m;
    buttonName = name;
}

DragAndDropButton::~DragAndDropButton() {}

bool DragAndDropButton::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto file : files)
    {
        if (file.contains(".wav") || file.contains(".mp3")) return true;
    }
    return false;
}

void DragAndDropButton::filesDropped(const juce::StringArray& files, int x, int y)
{
    for (juce::File file : files)
    {
        if (isInterestedInFileDrag(files)) {
            filename = file.getFileName();
            DBG("File dropped: " << filename);
            DBG("Midinote: " << midiNote);
            Processor.loadFile(file.getFullPathName(), midiNote, buttonName);
        }
    }
    repaint();
}

void DragAndDropButton::paint(juce::Graphics& g)
{

    g.fillAll(juce::Colours::grey);

    if (Processor.getNumSamplerSounds() > 0)
    {
        g.fillAll(juce::Colours::red);

        g.drawText(filename, getWidth() / 2 - 50, getHeight() / 2 - 10, 100, 20, juce::Justification::centred);
    }
    else
    {
        g.drawText("Drop here", getWidth() / 2 - 50, getHeight() / 2 - 10, 100, 20, juce::Justification::centred);
    }

}

//void DragAndDropButton::setMidinote(int m) {
//    midiNote = m;
//}