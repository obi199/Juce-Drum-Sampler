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
    SButton.setButtonText("Sample");
    SButton.onClick = [this] {SButtonClicked(&SButton); };

    addAndMakeVisible(&SButton);
    SButton.setEnabled(true);
    
   

}

DrumSamplerAudioProcessorEditor::~DrumSamplerAudioProcessorEditor()
{
  
}

//==============================================================================
void DrumSamplerAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(juce::Colours::black);

    g.setColour(juce::Colours::red);
    g.setFont(15.0f);

    if (audioProcessor.getNumSamplerSounds() > 0)
    {
        g.fillAll(juce::Colours::whitesmoke);

        g.drawText("Sound Loaded", getWidth() / 2 - 50, getHeight() / 2 - 10, 100, 20, juce::Justification::centred);
    }
    else
    {
        g.drawText("Load", getWidth() / 2 - 50, getHeight() / 2 - 10, 100, 20, juce::Justification::centred);
    }
    
}

void DrumSamplerAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    SButton.setBounds(getLocalBounds().reduced(30));
    //SButton.setBounds(50, 50, 150, 150);
}

bool DrumSamplerAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto file : files)
    {
        if (file.contains(".wav") || file.contains(".mp3")) return true;
    }
    return false;

}
void DrumSamplerAudioProcessorEditor::filesDropped(const juce::StringArray& files, int x, int y)
{
    for (auto file : files)
    {
        if (isInterestedInFileDrag(files)){

            audioFile = file;
            audioProcessor.loadFile(file);
        }
      
    }
    repaint();
       
 }



bool DrumSamplerAudioProcessorEditor::isMouseOver(juce::TextButton& button){
    if (button.isMouseOverOrDragging() == true) {
        return true;
        }
    return false;
    }



void DrumSamplerAudioProcessorEditor::SButtonClicked(juce::Button* button)
{
    if (button == &SButton)
    {
        audioProcessor.playFile();
    }

}

