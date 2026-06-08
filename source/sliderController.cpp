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
    setTextBoxStyle(TextBoxBelow, true, 75, 16);
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
    setupLabel(labelDetune);
    setupLabel(labelLowpass);
    setupLabel(labelHighpass);
    setupLabel(labelVelToLP);
    setupLabel(labelVelToAtk);

    addAndMakeVisible(&GainSlider);
    GainSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mGainAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "GAIN", GainSlider);

    addAndMakeVisible(&DetuneSlider);
    DetuneSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mDetuneAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "DETUNE", DetuneSlider);

    addAndMakeVisible(&LowpassSlider);
    LowpassSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mLowpassAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "LOWPASS", LowpassSlider);

    addAndMakeVisible(&HighpassSlider);
    HighpassSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mHighpassAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "HIGHPASS", HighpassSlider);

    addAndMakeVisible(&VelToLowpassSlider);
    VelToLowpassSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mVelToLowpassAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "VEL_TO_LOWPASS", VelToLowpassSlider);

    addAndMakeVisible(&VelToAttackSlider);
    VelToAttackSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mVelToAttackAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "VEL_TO_ATTACK", VelToAttackSlider);
}

void controlSlidersBlock::resized() {

    const auto labelH = 16;
    const auto sWidth = 75;
    const auto sHeight = 75;
    const auto row1Y = 10;
    const auto gap = (getWidth() - (sWidth * 6)) / 7;

    auto placeSlider = [&](sliderController& s, juce::Label& lbl, int x, int y)
    {
        lbl.setBounds(x, y, sWidth, labelH);
        s.setBounds(x, y + labelH, sWidth, sHeight);
    };

    // Row 1: Gain, Detune, Lowpass, Highpass, Vel>LP, Vel>Atk
    placeSlider(GainSlider,          labelGain,     gap,                row1Y);
    placeSlider(DetuneSlider,        labelDetune,   gap * 2 + sWidth,   row1Y);
    placeSlider(LowpassSlider,       labelLowpass,  gap * 3 + sWidth*2, row1Y);
    placeSlider(HighpassSlider,      labelHighpass, gap * 4 + sWidth*3, row1Y);
    placeSlider(VelToLowpassSlider,  labelVelToLP,  gap * 5 + sWidth*4, row1Y);
    placeSlider(VelToAttackSlider,   labelVelToAtk, gap * 6 + sWidth*5, row1Y);
}

void controlSlidersBlock::paint(juce::Graphics& /*g*/)
{
}

void controlSlidersBlock::changeSliderParameter(const juce::String& parameterID, juce::String sliderName) {
    if (sliderName == "Gain") {
        mGainAttachment.reset();
        mGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), parameterID, GainSlider);
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
    if (sliderName == "VelToLowpass") {
        mVelToLowpassAttachment.reset();
        mVelToLowpassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), parameterID, VelToLowpassSlider);
    }
    if (sliderName == "VelToAttack") {
        mVelToAttackAttachment.reset();
        mVelToAttackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), parameterID, VelToAttackSlider);
    }
}