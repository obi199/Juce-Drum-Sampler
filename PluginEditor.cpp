/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DrumSamplerAudioProcessorEditor::DrumSamplerAudioProcessorEditor (DrumSamplerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);
    SButton.setButtonText("Sample");
  
    
    addAndMakeVisible(&SButton);

}

DrumSamplerAudioProcessorEditor::~DrumSamplerAudioProcessorEditor()
{
  
}

//==============================================================================
void DrumSamplerAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    g.setColour(juce::Colours::transparentBlack);

    if (audioProcessor.getNumSamplerSounds() > 0)
    {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
        g.setColour(juce::Colours::red);
        g.setFont(15.0f);
        g.drawFittedText("Sample loaded!", getLocalBounds(), juce::Justification::centred, 1);
    }
    else
    {
        g.drawFittedText("Load Sample!", getLocalBounds(), juce::Justification::centred, 1);
    }

    
}

void DrumSamplerAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    SButton.setBounds(50, 50, 150, 150);
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
        if (isInterestedInFileDrag(files))
        {
            //load file
            audioProcessor.loadFile(file);

        }
      
    }
    repaint();
       
 }