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
    nameLabel.setFont(juce::FontOptions(15.0f));
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

    addAndMakeVisible(&StartOffsetSlider);
    StartOffsetSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mStartOffsetAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "START_OFFSET", StartOffsetSlider);
    StartOffsetSlider.attachLabel(&StartOffsetSlider, false);

    addAndMakeVisible(&VelToAttackSlider);
    VelToAttackSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mVelToAttackAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "VEL_TO_ATTACK", VelToAttackSlider);
    VelToAttackSlider.attachLabel(&VelToAttackSlider, false);

}

void controlSlidersBlock::resized() {

    const auto sWidth = 70;
    const auto sHeight = 70;
    const auto y = 25; // Increased from 15 to give more room for labels
    const auto gap = (getWidth() - (sWidth * 7)) / 8;

    GainSlider.setBounds(gap, y, sWidth, sHeight);
    AttackSlider.setBounds(gap * 2 + sWidth, y, sWidth, sHeight);
    DecaySlider.setBounds(gap * 3 + sWidth * 2, y, sWidth, sHeight);
    SustainSlider.setBounds(gap * 4 + sWidth * 3, y, sWidth, sHeight);
    ReleaseSlider.setBounds(gap * 5 + sWidth * 4, y, sWidth, sHeight);
    StartOffsetSlider.setBounds(gap * 6 + sWidth * 5, y, sWidth, sHeight);
    VelToAttackSlider.setBounds(gap * 7 + sWidth * 6, y, sWidth, sHeight);
}

void controlSlidersBlock::paint(juce::Graphics& /*g*/)
{
    // No fillAll here, it might cover other components if the parent doesn't set bounds correctly
    // or if this component is larger than expected.
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
    if (sliderName == "StartOffset") {
        mStartOffsetAttachment.reset();
        mStartOffsetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), parameterID, StartOffsetSlider);
    }
    if (sliderName == "VelToAttack") {
        mVelToAttackAttachment.reset();
        mVelToAttackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), parameterID, VelToAttackSlider);
    }
}
