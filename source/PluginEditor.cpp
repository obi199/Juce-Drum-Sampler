/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================
DrumSamplerAudioProcessorEditor::DrumSamplerAudioProcessorEditor (DrumSamplerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor(p), waveComponent(p) {

    addAndMakeVisible(&CBlock);
    addAndMakeVisible(&waveComponent);
    addAndMakeVisible(&position);
    addAndMakeVisible(&start);
    addAndMakeVisible(&end);
    addAndMakeVisible(&adsrOverlay);

    // Save / Load Kit buttons
    saveKitButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a4a2a));
    saveKitButton.setColour(juce::TextButton::textColourOffId, juce::Colours::lightgreen);
    addAndMakeVisible(saveKitButton);
    saveKitButton.onClick = [this]()
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Save Drum Set", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.drumkit");
        fileChooser->launchAsync(juce::FileBrowserComponent::saveMode |
                                 juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc)
            {
                auto file = fc.getResult();
                if (file != juce::File{})
                {
                    auto target = file.withFileExtension("drumkit");
                    audioProcessor.saveDrumSet(target);
                }
            });
    };

    loadKitButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a4a));
    loadKitButton.setColour(juce::TextButton::textColourOffId, juce::Colours::lightyellow);
    addAndMakeVisible(loadKitButton);
    loadKitButton.onClick = [this]()
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Load Drum Set", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.drumkit");
        fileChooser->launchAsync(juce::FileBrowserComponent::openMode |
                                 juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc)
            {
                auto file = fc.getResult();
                if (file.existsAsFile())
                {
                    audioProcessor.loadDrumSet(file, [this]()
                    {
                        juce::MessageManager::callAsync([this]() { refreshAllPads(); });
                    });
                }
            });
    };

    // Create 16 pads dynamically
    padButtons.reserve(NUM_PADS);
    for (int i = 0; i < NUM_PADS; ++i)
    {
        auto name = juce::String("Pad ") + juce::String(i + 1);
        auto midiNote = MIDI_NOTES[i];
        auto btn = std::make_unique<DragAndDropButton>(audioProcessor, midiNote, name);
        btn->setButtonText(name);

        auto* btnPtr = btn.get();
        btn->onClick = [this, btnPtr, midiNote]() { ButtonClicked(btnPtr, midiNote); };
        btn->onFileDropped = [this, i]() { switchTopad(i); };

        addAndMakeVisible(btnPtr);
        padButtons.push_back(std::move(btn));
        
        DBG("Created and added pad " << i << " with midiNote " << midiNote);
    }
   
    // Compute editor height to closely match the drum grid height
    const int initialWidth = 900;
    const int leftMarginCalc = 10;
    const int topMarginCalc = 10;
    const int colsCalc = 4;
    const int padGapCalc = 6;
    const int leftWidthCalc = initialWidth / 2;
    const int padSizeCalc = (leftWidthCalc - leftMarginCalc - (padGapCalc * colsCalc)) / colsCalc;
    const int rowsCalc = 4;
    const int gridHeight = topMarginCalc + rowsCalc * padSizeCalc + (rowsCalc - 1) * padGapCalc + topMarginCalc; // add bottom margin equal to top
    const int extra = 8; // a little bigger so the grid fits nicely
    setSize (initialWidth, gridHeight + extra);
    startTimer(50); // Poll for MIDI pad switches at ~20Hz
}

DrumSamplerAudioProcessorEditor::~DrumSamplerAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void DrumSamplerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void DrumSamplerAudioProcessorEditor::resized()
{
    // Layout: 4x4 grid of pads on the left half; controls + waveform on right
    int leftMargin = 10;
    int topMargin = 10;
    int cols = 4;
    int padGap = 6;

    int leftWidth = getWidth() / 2;
    int padSize = (leftWidth - leftMargin - (padGap * cols)) / cols;

    int rows = 4;
    for (size_t i = 0; i < padButtons.size(); ++i)
    {
        int row = static_cast<int>(i) / cols;
        int col = static_cast<int>(i) % cols;
        // Flip rows so pad 0 is at the bottom-left (MPC-style layout)
        int flippedRow = (rows - 1) - row;
        int x = leftMargin + col * (padSize + padGap);
        int y = topMargin + flippedRow * (padSize + padGap);
        padButtons[i]->setBounds(x, y, padSize, padSize);
    }

    // Right side layout
    int rightX = leftWidth;
    int rightWidth = getWidth() - rightX - 10; // 10px right margin
    int totalHeight = getHeight();

    // Save / Load buttons at the very top of the right panel
    const int btnH = 24;
    const int btnW = (rightWidth - 10) / 2;
    saveKitButton.setBounds(rightX,             5, btnW, btnH);
    loadKitButton.setBounds(rightX + btnW + 10, 5, btnW, btnH);

    // Waveform below the buttons
    int waveTop = 5 + btnH + 6;
    int waveHeight = (totalHeight / 2) - waveTop - 5;
    juce::Rectangle<int> thumbnailBounds(rightX, waveTop, rightWidth, waveHeight);
    waveComponent.setBounds(thumbnailBounds);
    position.setBounds(thumbnailBounds);
    start.setBounds(thumbnailBounds);
    end.setBounds(thumbnailBounds);
    adsrOverlay.setBounds(thumbnailBounds);

    // Controls below the waveform
    int controlsY = thumbnailBounds.getBottom() + 20;
    int controlsHeight = totalHeight - controlsY - 10;
    CBlock.setBounds(rightX, controlsY, rightWidth, controlsHeight);
}

void DrumSamplerAudioProcessorEditor::switchTopad(int padIndex)
{
    if (padIndex < 0 || padIndex >= NUM_PADS)
        return;

    lastDisplayedPadIndex = padIndex;
    audioProcessor.setUIPadIndex(padIndex);

    // If the pad has a sample, point the thumbnail to it; otherwise clear the waveform
    if (audioProcessor.hasSampleLoaded(padIndex))
    {
        auto sampleFile = audioProcessor.getSampleFile(padIndex);
        audioProcessor.thumbnail.setSource(new juce::FileInputSource(sampleFile));
    }
    else
    {
        audioProcessor.thumbnail.setSource(nullptr);
    }

    // Build parameter suffix for this pad ("" for pad 0, "2".."16" for others)
    auto suffix = (padIndex == 0) ? juce::String("") : juce::String(padIndex + 1);

    // Update the slider attachment to the new pad's parameters
    CBlock.changeSliderParameter("GAIN" + suffix, "Gain");
    CBlock.changeSliderParameter("VEL_TO_ATTACK" + suffix, "VelToAttack");
    CBlock.changeSliderParameter("DETUNE" + suffix, "Detune");
    CBlock.changeSliderParameter("EQ_LOW" + suffix, "EqLow");
    CBlock.changeSliderParameter("EQ_MID" + suffix, "EqMid");
    CBlock.changeSliderParameter("EQ_HIGH" + suffix, "EqHigh");
    CBlock.changeSliderParameter("LOWPASS" + suffix, "Lowpass");
    CBlock.changeSliderParameter("HIGHPASS" + suffix, "Highpass");
    CBlock.changeSliderParameter("VEL_TO_LOWPASS" + suffix, "VelToLowpass");
    CBlock.changeSliderParameter("DISTORTION" + suffix, "Distortion");

    // Apply the new pad's current parameter values to its sound. SliderAttachment
    // creation syncs the slider FROM the parameter (not the reverse), so
    // valueTreePropertyChanged is never triggered -- schedule the update for the audio thread.
    audioProcessor.schedulePadUpdate(padIndex);

    // Reset the draggable start line to the pad's stored offset
    int noteNumber = MIDI_NOTES[padIndex];
    float offset = audioProcessor.getStartOffsetForNote(noteNumber);
    start.setNormalizedOffset(offset);

    float endOffset = audioProcessor.getEndOffsetForNote(noteNumber);
    end.setNormalizedOffset(endOffset);
}

void DrumSamplerAudioProcessorEditor::ButtonClicked(juce::Button* /*button*/, int noteNumber)
{
    int padIndex = audioProcessor.getPadIndexFromMidiNote(noteNumber);
    if (padIndex < 0)
        return;

    switchTopad(padIndex);
    audioProcessor.playFile(noteNumber);
}

void DrumSamplerAudioProcessorEditor::refreshAllPads()
{
    for (int i = 0; i < NUM_PADS; ++i)
    {
        if (i >= (int)padButtons.size()) break;
        auto& btn = *padButtons[static_cast<size_t>(i)];
        if (audioProcessor.hasSampleLoaded(i))
            btn.setFilename(audioProcessor.getSampleFile(i).getFileName());
        else
            btn.clearSample();
    }
    switchTopad(0);
}

void DrumSamplerAudioProcessorEditor::timerCallback()
{
    // Consume the flag so it doesn't accumulate, but do not switch
    // the waveform display on MIDI input — only button presses do that.
    audioProcessor.checkAndClearPadSwitchedFromMidi();
}










