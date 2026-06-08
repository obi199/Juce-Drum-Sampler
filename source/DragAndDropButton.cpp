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

    // Fetch existing filename if a sample is already loaded for this pad
    int padIdx = Processor.getPadIndexFromMidiNote(midiNote);
    if (padIdx >= 0)
    {
        auto file = Processor.getSampleFile(padIdx);
        if (file.existsAsFile())
            filename = file.getFileName();
    }
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
            if (onFileDropped) onFileDropped();
        }
    }
    repaint();
}

void DragAndDropButton::mouseDrag(const juce::MouseEvent& e)
{
    // Only start a pad drag if this pad has a sample loaded
    if (filename.isEmpty()) return;

    // Must have moved enough to be intentional
    if (e.getDistanceFromDragStart() < 8) return;

    auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this);
    if (container && !container->isDragAndDropActive())
    {
        // Pass the file path as the drag description so the target can load it
        juce::String filePath = Processor.getSampleFile(
            Processor.getPadIndexFromMidiNote(midiNote)).getFullPathName();
        container->startDragging(filePath, this);
    }
}

bool DragAndDropButton::isInterestedInDragSource(const SourceDetails& details)
{
    // Accept drags from other pads (description is a file path string)
    return details.description.isString() &&
           details.sourceComponent.get() != this;
}

void DragAndDropButton::itemDropped(const SourceDetails& details)
{
    dragHighlight = false;
    juce::String filePath = details.description.toString();
    if (filePath.isNotEmpty())
    {
        filename = juce::File(filePath).getFileName();
        Processor.loadFile(filePath, midiNote, buttonName);
        if (onFileDropped) onFileDropped();

        // Clear the source pad (move, not copy)
        if (auto* source = dynamic_cast<DragAndDropButton*>(details.sourceComponent.get()))
        {
            Processor.clearPad(source->getMidiNote());
            source->clearSample();
        }

        repaint();
    }
}

void DragAndDropButton::itemDragEnter(const SourceDetails&)
{
    dragHighlight = true;
    repaint();
}

void DragAndDropButton::itemDragExit(const SourceDetails&)
{
    dragHighlight = false;
    repaint();
}

void DragAndDropButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    float cornerSize = 6.0f;

    bool hasSound = !filename.isEmpty();

    // Outer shadow
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillRoundedRectangle(bounds.translated(1.5f, 1.5f), cornerSize);

    // Main pad body — highlight when a pad is being dragged over this one
    juce::Colour padColour = dragHighlight ? juce::Colour(0xff555500)
                           : hasSound      ? juce::Colour(0xff3a3a3a)
                                           : juce::Colours::lightgrey;
    g.setColour(padColour);
    g.fillRoundedRectangle(bounds, cornerSize);

    // Top-left highlight bevel
    juce::ColourGradient highlight(juce::Colours::white.withAlpha(0.15f), bounds.getX(), bounds.getY(),
                                   juce::Colours::transparentBlack, bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(highlight);
    g.fillRoundedRectangle(bounds, cornerSize);

    // Subtle inner border — bright yellow when dragging over
    g.setColour(dragHighlight ? juce::Colours::yellow.withAlpha(0.8f)
                              : juce::Colours::white.withAlpha(0.08f));
    g.drawRoundedRectangle(bounds.reduced(1.0f), cornerSize, dragHighlight ? 2.0f : 1.0f);

    // Active indicator strip at the bottom when a sample is loaded
    if (hasSound)
    {
        auto strip = bounds.removeFromBottom(4.0f).reduced(10.0f, 0.0f);
        g.setColour(juce::Colour(0xffff4444));
        g.fillRoundedRectangle(strip, 2.0f);
    }

    // Text
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(juce::FontOptions(12.0f));

    if (hasSound)
        g.drawText(filename, getLocalBounds().reduced(6), juce::Justification::centred, true);
    else
    {
        g.setColour(juce::Colours::darkgrey);
        g.drawText("DROP\nSAMPLE", getLocalBounds().reduced(6), juce::Justification::centred, true);
    }
}

void DragAndDropButton::mouseUp(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown())
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Load Sample...");
        menu.addItem(2, "Clear Pad", !filename.isEmpty());

        menu.showMenuAsync(juce::PopupMenu::Options{}.withTargetComponent(this),
            [this](int result)
            {
                if (result == 1)
                {
                    // Load Sample
                    chooser = std::make_unique<juce::FileChooser>("Select a sample to load...",
                        juce::File{},
                        "*.wav;*.mp3");

                    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

                    chooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
                        {
                            auto file = fc.getResult();
                            if (file.existsAsFile())
                            {
                                filename = file.getFileName();
                                Processor.loadFile(file.getFullPathName(), midiNote, buttonName);
                                if (onFileDropped) onFileDropped();
                                repaint();
                            }
                        });
                }
                else if (result == 2)
                {
                    Processor.clearPad(midiNote);
                    clearSample();
                }
            });
    }
    else
    {
        // Forward left-clicks to TextButton so onClick fires correctly
        juce::TextButton::mouseUp(e);
    }
}

//void DragAndDropButton::setMidinote(int m) {
//    midiNote = m;
//}