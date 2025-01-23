/*
  ==============================================================================

    sliderController.h
    Created: 19 Aug 2024 10:09:32pm
    Author:  obi

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
//==============================================================================
/*
*/


class sliderController : public juce::Slider
{
public:
    sliderController::sliderController(juce::String);
    ~sliderController() override;
    void attachLabel(Component*, bool);


private:
    juce::Label nameLabel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(sliderController)
};



class controlSlidersBlock : public juce::Component
{
public:
    controlSlidersBlock::controlSlidersBlock(DrumSamplerAudioProcessor&);
    //~controlSlidersBlock() override;
  
    void resized() override;
    void paint(juce::Graphics&) override;
    typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
    void changeSliderParameter(const juce::String&, juce::String);

private:

    sliderController GainSlider{ "Gain" };
    sliderController AttackSlider{ "Attack" };
    sliderController DecaySlider{ "Decay" };
    sliderController SustainSlider{ "Sustain" };
    sliderController ReleaseSlider{ "Release" };

    std::unique_ptr<SliderAttachment> mGainAttachment;
    std::unique_ptr<SliderAttachment> mAttackAttachment;
    std::unique_ptr<SliderAttachment> mDecayAttachment;
    std::unique_ptr<SliderAttachment> mSustainAttachment;
    std::unique_ptr<SliderAttachment> mReleaseAttachment;

    DrumSamplerAudioProcessor& audioProcessor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(controlSlidersBlock)
};

