/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================
DrumSamplerAudioProcessorEditor::DrumSamplerAudioProcessorEditor (DrumSamplerAudioProcessor& p)
    : AudioProcessorEditor (&p), waveComponent(p), audioProcessor(p) {

    addAndMakeVisible(&CBlock);
    addAndMakeVisible(&waveComponent);
    addAndMakeVisible(&position);
    addAndMakeVisible(&start);

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
}

DrumSamplerAudioProcessorEditor::~DrumSamplerAudioProcessorEditor()
{

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

    for (int i = 0; i < static_cast<int>(padButtons.size()); ++i)
    {
        int row = i / cols;
        int col = i % cols;
        int x = leftMargin + col * (padSize + padGap);
        int y = topMargin + row * (padSize + padGap);
        padButtons[i]->setBounds(x, y, padSize, padSize);
    }

    // Right side layout
    int rightX = leftWidth;
    int rightWidth = getWidth() - rightX - 10; // 10px right margin
    int totalHeight = getHeight();

    // Waveform at the top right
    int waveHeight = (totalHeight / 2) - 15;
    juce::Rectangle<int> thumbnailBounds (rightX, 10, rightWidth, waveHeight);
    waveComponent.setBounds(thumbnailBounds);
    position.setBounds(thumbnailBounds);
    start.setBounds(thumbnailBounds);

    // Controls below the waveform
    int controlsY = thumbnailBounds.getBottom() + 10;
    int controlsHeight = totalHeight - controlsY - 10;
    CBlock.setBounds(rightX, controlsY, rightWidth, controlsHeight);
}

void DrumSamplerAudioProcessorEditor::ButtonClicked(juce::Button* /*button*/, int noteNumber)
{
    // Determine pad index from MIDI note
    int padIndex = audioProcessor.getPadIndexFromMidiNote(noteNumber);
    if (padIndex < 0)
        return;

    // If the pad has a sample, point the thumbnail to it first so we know its length
    if (audioProcessor.hasSampleLoaded(padIndex))
    {
        auto sampleFile = audioProcessor.getSampleFile(padIndex);
        audioProcessor.thumbnail.setSource(new juce::FileInputSource(sampleFile));
    }

    // Reset the draggable start line and processor's newPositionSec to the pad's stored offset
    // Do this BEFORE calling playFile so playback uses the correct offset.
    float offset = audioProcessor.getStartOffsetForNote(noteNumber);
    // Set visual position immediately using normalized offset to avoid 0-length thumbnail race
    start.setNormalizedOffset(offset);

    audioProcessor.playFile(noteNumber);

    // Build parameter suffix for this pad ("" for pad 0, "2".."16" for others)
    auto suffix = (padIndex == 0) ? juce::String("") : juce::String(padIndex + 1);

    // Update the slider attachment to the new pad's parameters
    CBlock.changeSliderParameter("GAIN" + suffix, "Gain");
    CBlock.changeSliderParameter("ATTACK" + suffix, "Attack");
    CBlock.changeSliderParameter("DECAY" + suffix, "Decay");
    CBlock.changeSliderParameter("RELEASE" + suffix, "Release");
    CBlock.changeSliderParameter("SUSTAIN" + suffix, "Sustain");
    CBlock.changeSliderParameter("START_OFFSET" + suffix, "StartOffset");

    audioProcessor.updateADSR(padIndex);
}











