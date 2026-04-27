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

void DragAndDropButton::filesDropped(const juce::StringArray& files, int /*x*/, int /*y*/)
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
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    float cornerSize = 6.0f;

    bool hasSound = !filename.isEmpty();

    // --- MPC pad base colour ---
    juce::Colour padColour = hasSound ? juce::Colour(0xff3a3a3a) : juce::Colours::lightgrey;

    // Outer shadow (gives a sunken look)
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillRoundedRectangle(bounds.translated(1.5f, 1.5f), cornerSize);

    // Main pad body
    g.setColour(padColour);
    g.fillRoundedRectangle(bounds, cornerSize);

    // Top-left highlight bevel
    juce::ColourGradient highlight(juce::Colours::white.withAlpha(0.15f), bounds.getX(), bounds.getY(),
                                   juce::Colours::transparentBlack, bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(highlight);
    g.fillRoundedRectangle(bounds, cornerSize);

    // Subtle inner border
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawRoundedRectangle(bounds.reduced(1.0f), cornerSize, 1.0f);

    // Active indicator strip at the bottom when a sample is loaded
    if (hasSound)
    {
        auto strip = bounds.removeFromBottom(4.0f).reduced(10.0f, 0.0f);
        g.setColour(juce::Colour(0xffff4444));
        g.fillRoundedRectangle(strip, 2.0f);
    }

    // --- Text ---
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(juce::FontOptions(12.0f));

    if (hasSound)
    {
        g.drawText(filename, getLocalBounds().reduced(6), juce::Justification::centred, true);
    }
    else
    {
        g.setColour(juce::Colours::darkgrey);
        g.drawText("DROP\nSAMPLE", getLocalBounds().reduced(6), juce::Justification::centred, true);
    }
}

//void DragAndDropButton::setMidinote(int m) {
//    midiNote = m;
//}