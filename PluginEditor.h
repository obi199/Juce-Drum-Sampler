/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "WaveFormEditor.h"

//==============================================================================
/**Drum Sampler Plugin
*/



class sliderController : public juce::Slider
{
public:
    sliderController::sliderController(juce::String);
    void attachLabel(Component*, bool);

    //void paint(juce::Graphics&) override;

private:
    juce::Label nameLabel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(sliderController)
};




class controlSlidersBlock : public juce::Component
{
public:
    controlSlidersBlock::controlSlidersBlock(DrumSamplerAudioProcessor&);
    
    //void sliderValueChanged(juce::Slider*);
    void resized() override;
    void paint(juce::Graphics&) override;

private:

    sliderController VolSlider{ "Volume" };
    sliderController AttackSlider{ "Attack" };
    sliderController DecaySlider{ "Decay" };
    sliderController SustainSlider{ "Sustain" };
    sliderController ReleaseSlider{ "Release" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mAttackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mDecayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mSustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mReleaseAttachment;

    DrumSamplerAudioProcessor& audioProcessor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(controlSlidersBlock)
};








class DragAndDropButton : public juce::TextButton, public juce::FileDragAndDropTarget
{
public:
    DragAndDropButton::DragAndDropButton(DrumSamplerAudioProcessor&);
    ~DragAndDropButton() override;
    void paint(juce::Graphics&) override;

    bool FileDragAndDropTarget::isInterestedInFileDrag(const juce::StringArray& );
   
    void FileDragAndDropTarget::filesDropped(const juce::StringArray& , int , int );


private:
    DrumSamplerAudioProcessor& Processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DragAndDropButton)
};



//main component//

class DrumSamplerAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    DrumSamplerAudioProcessorEditor (DrumSamplerAudioProcessor&);
    ~DrumSamplerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void ButtonClicked(juce::Button* button, int noteNumber);    
    juce::File audioFile;
   
private:

    DrumSamplerAudioProcessor& audioProcessor;
    DragAndDropButton myButton { audioProcessor };
    waveFormEditor waveForm{ audioProcessor };
    controlSlidersBlock CBlock{ audioProcessor };



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumSamplerAudioProcessorEditor)
};



