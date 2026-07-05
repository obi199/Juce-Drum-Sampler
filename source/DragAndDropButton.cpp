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

void DragAndDropButton::resized()
{
    juce::TextButton::resized();
}

void DragAndDropButton::paint(juce::Graphics& g)
{
    auto fullBounds = getLocalBounds().toFloat();
    auto bounds = fullBounds.reduced(3.0f); // Leave room for shadow
    float cornerSize = 4.0f;

    bool hasSound = !filename.isEmpty();
    bool isPressed = isMouseButtonDown();
    bool isOver = isMouseOver();

    // 1. Draw 3D Shadow
    if (!isPressed)
    {
        juce::Path shadowPath;
        shadowPath.addRoundedRectangle(bounds.translated(2.0f, 2.0f), cornerSize);
        juce::DropShadow shadow(juce::Colours::black.withAlpha(0.6f), 5, { 2, 2 });
        shadow.drawForPath(g, shadowPath);
    }
    else
    {
        // Slightly shift the pad content down-right to simulate being pressed
        bounds = bounds.translated(1.0f, 1.0f);
        
        juce::Path shadowPath;
        shadowPath.addRoundedRectangle(bounds.translated(0.5f, 0.5f), cornerSize);
        juce::DropShadow shadow(juce::Colours::black.withAlpha(0.4f), 2, { 1, 1 });
        shadow.drawForPath(g, shadowPath);
    }

    // 2. Pad Body with Gradient (MPC style)
    juce::Colour baseColour = dragHighlight ? juce::Colour(0xff777700)
                           : hasSound      ? juce::Colours::grey
                                           : juce::Colours::lightgrey;
                                           
    if (isOver && !dragHighlight)
        baseColour = baseColour.brighter(0.05f);

    juce::Colour topColor = baseColour.brighter(0.15f);
    juce::Colour bottomColor = baseColour.darker(0.15f);
    
    if (isPressed)
        std::swap(topColor, bottomColor); // Invert gradient when pressed

    juce::ColourGradient grad(topColor, bounds.getX(), bounds.getY(),
                              bottomColor, bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(bounds, cornerSize);

    // 3. Bevel and Edges
    // Inner highlight (top-left)
    g.setColour(juce::Colours::white.withAlpha(isPressed ? 0.1f : 0.3f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 1.0f);
    
    // Inner shadow (bottom-right)
    g.setColour(juce::Colours::black.withAlpha(isPressed ? 0.3f : 0.15f));
    g.drawRoundedRectangle(bounds.reduced(1.0f), cornerSize, 0.5f);

    // 4. Active indicator strip (Glowy)
    if (hasSound)
    {
        auto stripBounds = bounds.withHeight(4.0f).translated(0, bounds.getHeight() - 10.0f).reduced(bounds.getWidth() * 0.25f, 0);
        
        juce::Colour indicatorColor = juce::Colour(0xffff4444);
        if (isOver) indicatorColor = indicatorColor.brighter(0.3f);
        
        // Glow effect
        for (int i = 1; i <= 3; ++i)
        {
            g.setColour(indicatorColor.withAlpha(0.2f / i));
            g.fillRoundedRectangle(stripBounds.expanded(i * 1.0f), 2.0f);
        }
        
        g.setColour(indicatorColor);
        g.fillRoundedRectangle(stripBounds, 2.0f);
    }

    // 5. Text - Filename
    if (hasSound)
    {
        g.setColour(juce::Colours::black.withAlpha(0.8f));
        g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
        
        // Increased height and used drawFittedText to prevent the name from being "cut up"
        auto textBounds = bounds.reduced(5.0f).withHeight(35.0f).translated(0, 2.0f);
        g.drawFittedText(filename, textBounds.toNearestInt(), juce::Justification::centredTop, 2);
    }
    
    // 6. Text - Note Name (Moved here from the Label for 3D alignment)
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.setFont(juce::FontOptions(13.0f));
    auto noteName = juce::MidiMessage::getMidiNoteName(midiNote, true, true, 3).toLowerCase();
    
    // Position note name slightly lower to avoid overlap with multi-line filenames
    auto noteBounds = bounds.withTrimmedTop(hasSound ? 20.0f : 0.0f);
    g.drawText(noteName, noteBounds, juce::Justification::centred, false);
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