/*
  ==============================================================================

    sliderController.cpp
    Created: 19 Aug 2024 10:09:32pm
    Author:  obi

  ==============================================================================
*/

#include <JuceHeader.h>
#include "sliderController.h"

//==============================================================================


sliderController::sliderController(juce::String name) {

    addAndMakeVisible(&nameLabel);
    setTextBoxStyle(TextBoxBelow, true, getTextBoxWidth() - 20, getTextBoxHeight());
    nameLabel.setFont(15.0);
    nameLabel.setText(name, juce::dontSendNotification);
    nameLabel.attachToComponent(this, true);
    nameLabel.setJustificationType(juce::Justification::centredTop);

}

sliderController::~sliderController()
{
}

void sliderController::attachLabel(Component* owner, bool onLeft)
{
    nameLabel.attachToComponent(owner, onLeft);
}


//Block of Sliders//
controlSlidersBlock::controlSlidersBlock(DrumSamplerAudioProcessor& p) : audioProcessor(p)
{
    addAndMakeVisible(&GainSlider);
    GainSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mGainAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "GAIN", GainSlider);
    GainSlider.attachLabel(&GainSlider, false);

    addAndMakeVisible(&AttackSlider);
    AttackSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mAttackAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "ATTACK", AttackSlider);
    AttackSlider.attachLabel(&AttackSlider, false);

    addAndMakeVisible(&DecaySlider);
    DecaySlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mDecayAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "DECAY", DecaySlider);
    DecaySlider.attachLabel(&DecaySlider, false);

    addAndMakeVisible(&ReleaseSlider);
    ReleaseSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mReleaseAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "RELEASE", ReleaseSlider);
    ReleaseSlider.attachLabel(&ReleaseSlider, false);

    addAndMakeVisible(&SustainSlider);
    SustainSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mSustainAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "SUSTAIN", SustainSlider);
    SustainSlider.attachLabel(&SustainSlider, false);

}

void controlSlidersBlock::resized() {

    const auto x = 0;
    const auto y = (getHeight() / 2);
    const auto sWidth = 70;
    const auto sHeight = 70;

    GainSlider.setBounds(x, y, sWidth, sHeight);
    AttackSlider.setBounds(x + sWidth, (getHeight() / 2), sWidth, sHeight);
    DecaySlider.setBounds(x + sWidth * 2, (getHeight() / 2), sWidth, sHeight);
    SustainSlider.setBounds(x + sWidth * 3, (getHeight() / 2), sWidth, sHeight);
    ReleaseSlider.setBounds(x + sWidth * 4, (getHeight() / 2), sWidth, sHeight);
}

void controlSlidersBlock::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::grey);
}

void controlSlidersBlock::changeSliderParameter(const juce::String& parameterID, juce::String sliderName) {
    // Update the attachment to the new parameter
    // Detach the current parameter
    
    //mAttackAttachment.reset();
    if (sliderName == "Gain") {
        mGainAttachment.reset();
        mGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), parameterID, GainSlider);
    }
    if (sliderName == "Attack") {
        mAttackAttachment.reset();
        mAttackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), parameterID, AttackSlider);
    }
    if (sliderName == "Decay") {
        mDecayAttachment.reset();
        mDecayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), parameterID, DecaySlider);
    }
    if (sliderName == "Sustain") {
        mSustainAttachment.reset();
        mSustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), parameterID, SustainSlider);
    }
    if (sliderName == "Release") {
        mReleaseAttachment.reset();
        mReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), parameterID, ReleaseSlider);
    }
}
