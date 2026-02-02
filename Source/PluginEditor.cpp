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

    setSize (800, 400);
    myButton.setButtonText("myButton1");
    addAndMakeVisible(&myButton);
    myButton2.setButtonText("myButton2");
    addAndMakeVisible(&myButton2);
    myButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
    //myButton.setMidinote(60);
    myButton.onClick = [this] {ButtonClicked(&myButton, 60); };

    //myButton2.setMidinote(61);
    myButton2.onClick = [this] {ButtonClicked(&myButton2, 61); };
   
    addAndMakeVisible(&CBlock);
    addAndMakeVisible(&waveComponent);
    addAndMakeVisible(&position);
    addAndMakeVisible(&start);

    //position.toBack(); //in back not visible

    
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

    myButton.setBounds(10, getHeight()-80, 70, 70);
    myButton2.setBounds(90, getHeight() - 80, 70, 70);
    CBlock.setBounds((getWidth() / 2), (getHeight() / 2), (getWidth() / 2)-50, 150);

    //wave thumbnail
    juce::Rectangle<int> thumbnailBounds ((getWidth() / 2), 10, getWidth()/2 - 50, getHeight()/2 - 20);
    waveComponent.setBounds(thumbnailBounds);
    position.setBounds(thumbnailBounds);
    //start.setBounds((getWidth() / 2)-50, 10, getWidth() / 2 - 50, getHeight() / 2 - 20);
    start.setBounds(thumbnailBounds);

   
}

void DrumSamplerAudioProcessorEditor::ButtonClicked(juce::Button* button, int noteNumber)

{
    if (button == &myButton)
    {   
        audioProcessor.playFile(noteNumber);
        
        // Safely get sample file for pad 0
        if (audioProcessor.hasSampleLoaded(0)) 
        {
            auto sampleFile = audioProcessor.getSampleFile(0);
            audioProcessor.thumbnail.setSource(new juce::FileInputSource(sampleFile));
        }
        
        // Update the slider attachment to the new parameter
        CBlock.changeSliderParameter("GAIN", "Gain");
        CBlock.changeSliderParameter("ATTACK", "Attack"); 
        CBlock.changeSliderParameter("DECAY", "Decay");
        CBlock.changeSliderParameter("RELEASE", "Release");
        CBlock.changeSliderParameter("SUSTAIN", "Sustain");
        audioProcessor.updateADSR(0);

    }

    else if (button == &myButton2)
    {
        audioProcessor.playFile(noteNumber);
        
        // Safely get sample file for pad 1
        if (audioProcessor.hasSampleLoaded(1)) 
        {
            auto sampleFile = audioProcessor.getSampleFile(1);
            audioProcessor.thumbnail.setSource(new juce::FileInputSource(sampleFile));
        }
        
        // Update the slider attachment to the new parameter
        CBlock.changeSliderParameter("GAIN2", "Gain"); 
        CBlock.changeSliderParameter("ATTACK2", "Attack"); 
        CBlock.changeSliderParameter("DECAY2", "Decay");
        CBlock.changeSliderParameter("RELEASE2", "Release");
        CBlock.changeSliderParameter("SUSTAIN2", "Sustain");
        
        audioProcessor.updateADSR(1);

    }
}











