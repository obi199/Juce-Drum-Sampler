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


sliderController::sliderController(juce::String /*name*/) {
    setLookAndFeel(&noBoxLAF);
    setTextBoxStyle(TextBoxBelow, true, 60, 16);
}

sliderController::~sliderController()
{
    setLookAndFeel(nullptr);
}


//Block of Sliders//
controlSlidersBlock::controlSlidersBlock(DrumSamplerAudioProcessor& p) : audioProcessor(p)
{
    auto setupLabel = [&](juce::Label& lbl)
    {
        lbl.setFont(juce::FontOptions(15.0f));
        lbl.setJustificationType(juce::Justification::centred);
        lbl.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(lbl);
    };

    setupLabel(labelGain);
    setupLabel(labelAttack);
    setupLabel(labelDecay);
    setupLabel(labelSustain);
    setupLabel(labelRelease);
    setupLabel(labelStart);
    setupLabel(labelDetune);
    setupLabel(labelLowpass);
    setupLabel(labelHighpass);
    setupLabel(labelVelToAtk);

    addAndMakeVisible(&GainSlider);
    GainSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mGainAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "GAIN", GainSlider);

    addAndMakeVisible(&AttackSlider);
    AttackSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mAttackAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "ATTACK", AttackSlider);

    addAndMakeVisible(&DecaySlider);
    DecaySlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mDecayAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "DECAY", DecaySlider);

    addAndMakeVisible(&ReleaseSlider);
    ReleaseSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mReleaseAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "RELEASE", ReleaseSlider);

    addAndMakeVisible(&SustainSlider);
    SustainSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mSustainAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "SUSTAIN", SustainSlider);

    addAndMakeVisible(&StartOffsetSlider);
    StartOffsetSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mStartOffsetAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "START_OFFSET", StartOffsetSlider);

    addAndMakeVisible(&VelToAttackSlider);
    VelToAttackSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mVelToAttackAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "VEL_TO_ATTACK", VelToAttackSlider);

    addAndMakeVisible(&DetuneSlider);
    DetuneSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mDetuneAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "DETUNE", DetuneSlider);

    addAndMakeVisible(&LowpassSlider);
    LowpassSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mLowpassAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "LOWPASS", LowpassSlider);

    addAndMakeVisible(&HighpassSlider);
    HighpassSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mHighpassAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "HIGHPASS", HighpassSlider);
}

void controlSlidersBlock::resized() {

    const auto labelH = 16;
    const auto sWidth = 70;
    const auto sHeight = 65;
    const auto row1Y = 10;
    const auto row2Y = row1Y + labelH + sHeight + 8;
    const auto gap = (getWidth() - (sWidth * 7)) / 8;

    auto placeSlider = [&](sliderController& s, juce::Label& lbl, int x, int y)
    {
        lbl.setBounds(x, y, sWidth, labelH);
        s.setBounds(x, y + labelH, sWidth, sHeight);
    };

    // Row 1: Gain, Attack, Decay, Sustain, Release, Start
    placeSlider(GainSlider,        labelGain,     gap,                row1Y);
    placeSlider(AttackSlider,      labelAttack,   gap * 2 + sWidth,   row1Y);
    placeSlider(DecaySlider,       labelDecay,    gap * 3 + sWidth*2, row1Y);
    placeSlider(SustainSlider,     labelSustain,  gap * 4 + sWidth*3, row1Y);
    placeSlider(ReleaseSlider,     labelRelease,  gap * 5 + sWidth*4, row1Y);
    placeSlider(StartOffsetSlider, labelStart,    gap * 6 + sWidth*5, row1Y);

    // Row 2: Detune, Lowpass, Highpass, Vel>Atk
    placeSlider(DetuneSlider,      labelDetune,   gap,                row2Y);
    placeSlider(LowpassSlider,     labelLowpass,  gap * 2 + sWidth,   row2Y);
    placeSlider(HighpassSlider,    labelHighpass, gap * 3 + sWidth*2, row2Y);
    placeSlider(VelToAttackSlider, labelVelToAtk, gap * 4 + sWidth*3, row2Y);
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
    if (sliderName == "Detune") {
        mDetuneAttachment.reset();
        mDetuneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), parameterID, DetuneSlider);
    }
    if (sliderName == "Lowpass") {
        mLowpassAttachment.reset();
        mLowpassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), parameterID, LowpassSlider);
    }
    if (sliderName == "Highpass") {
        mHighpassAttachment.reset();
        mHighpassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), parameterID, HighpassSlider);
    }
}
