/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DrumSamplerAudioProcessorEditor::DrumSamplerAudioProcessorEditor (DrumSamplerAudioProcessor& p)
    : AudioProcessorEditor (&p), waveComponent(p), audioProcessor(p){

    setSize (800, 400);
    myButton.setButtonText("myButton");
    addAndMakeVisible(&myButton);
    myButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
    myButton.onClick = [this] {ButtonClicked(&myButton, 60); };
    addAndMakeVisible(&CBlock);
    addAndMakeVisible(&waveComponent);
    addAndMakeVisible(&position);

    
}

DrumSamplerAudioProcessorEditor::~DrumSamplerAudioProcessorEditor()
{

}

//==============================================================================
void DrumSamplerAudioProcessorEditor::paint (juce::Graphics& g)
{

}

void DrumSamplerAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    myButton.setBounds(10, getHeight()-80, 70, 70);
    CBlock.setBounds((getWidth() / 2), (getHeight() / 2), (getWidth() / 2)-50, 150);

    //wave thumbnail
    //juce::Rectangle<int> thumbnailBounds(100, 100, getWidth() - 100, getHeight() - 150);
    juce::Rectangle<int> thumbnailBounds ((getWidth() / 2), 10, getWidth()/2 - 50, getHeight()/2 - 20);
    //DBG(getHeight() / 2 - 2);
    waveComponent.setBounds(thumbnailBounds);
    position.setBounds(thumbnailBounds);
   
    //GainSlider.setBounds(getWidth() / 2 + 150, getHeight() / 2 - 50, 80, 150);
}

void DrumSamplerAudioProcessorEditor::ButtonClicked(juce::Button* button, int noteNumber)
{
    if (button == &myButton)
    {   
        audioProcessor.playFile(noteNumber);
    }
}











