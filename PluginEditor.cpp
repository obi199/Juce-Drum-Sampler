/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DrumSamplerAudioProcessorEditor::DrumSamplerAudioProcessorEditor (DrumSamplerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor(p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);

    myButton.setButtonText("myButton");
    addAndMakeVisible(&myButton);
    myButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
    myButton.onClick = [this] {ButtonClicked(&myButton, 60); };



}

DrumSamplerAudioProcessorEditor::~DrumSamplerAudioProcessorEditor()
{
  
}

//==============================================================================
void DrumSamplerAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    //g.fillAll(juce::Colours::black);

    g.setColour(juce::Colours::red);
    g.setFont(15.0f);
    //g.drawEllipse(50, 10, 60, 60, 3);


    //if (audioProcessor.getNumSamplerSounds() > 0)
    //{
    //    g.fillAll(juce::Colours::whitesmoke);

    //    g.drawText("Sound Loaded", getWidth() / 2 - 50, getHeight() / 2 - 10, 100, 20, juce::Justification::centred);
    //}
    //else
    //{
    //    g.drawText("Load", getWidth() / 2 - 50, getHeight() / 2 - 10, 100, 20, juce::Justification::centred);
    //}
    
}

void DrumSamplerAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
   
    myButton.setBounds(40, 100, 70, 70);
    
}




void DrumSamplerAudioProcessorEditor::ButtonClicked(juce::Button* button, int noteNumber)
{
    if (button == &myButton)
    {   
        audioProcessor.playFile(noteNumber);
    }

}



DragAndDropButton::DragAndDropButton(DrumSamplerAudioProcessor& p): Processor(p) {

 }

DragAndDropButton::~DragAndDropButton(){}


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
    for (auto file : files)
    {

        if (isInterestedInFileDrag(files)) {
            DBG("File dropped: " << file);

            Processor.loadFile(file);

        }

    }
    //repaint();

}