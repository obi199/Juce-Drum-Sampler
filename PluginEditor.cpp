/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DrumSamplerAudioProcessorEditor::DrumSamplerAudioProcessorEditor (DrumSamplerAudioProcessor& p)
    : AudioProcessorEditor (&p), waveForm(p), audioProcessor(p){ 
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (800, 400);
    myButton.setButtonText("myButton");
    addAndMakeVisible(&myButton);
    myButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
    myButton.onClick = [this] {ButtonClicked(&myButton, 60); };
    addAndMakeVisible(&waveForm);
    addAndMakeVisible(&CBlock);
   
   

    
}

DrumSamplerAudioProcessorEditor::~DrumSamplerAudioProcessorEditor()
{

}

//==============================================================================
void DrumSamplerAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    //g.fillAll(juce::Colours::black);

    //g.setColour(juce::Colours::red);
    //g.setFont(15.0f);
    //
}

void DrumSamplerAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    myButton.setBounds(10, getHeight()-80, 70, 70);
    waveForm.setBounds((getWidth() / 2), (getHeight() / 2) - 270, 250, 250);
    CBlock.setBounds((getWidth() / 2), (getHeight() / 2), (getWidth() / 2)-50, 150);
   
    //GainSlider.setBounds(getWidth() / 2 + 150, getHeight() / 2 - 50, 80, 150);
}

void DrumSamplerAudioProcessorEditor::ButtonClicked(juce::Button* button, int noteNumber)
{
    if (button == &myButton)
    {   
        audioProcessor.playFile(noteNumber);
    }
}









