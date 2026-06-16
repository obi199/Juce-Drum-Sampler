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
    setupLabel(labelEqLow);
    setupLabel(labelEqMid);
    setupLabel(labelEqHigh);
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

    addAndMakeVisible(&EqLowSlider);
    EqLowSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mEqLowAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "EQ_LOW", EqLowSlider);

    addAndMakeVisible(&EqMidSlider);
    EqMidSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mEqMidAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "EQ_MID", EqMidSlider);

    addAndMakeVisible(&EqHighSlider);
    EqHighSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    mEqHighAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "EQ_HIGH", EqHighSlider);

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

    const auto labelH  = 16;
    const auto sWidth  = 75;
    const auto sHeight = 75;
    const auto rowH    = labelH + sHeight;  // 91px per row
    const auto row1Y   = 5;
    const auto row2Y   = row1Y + rowH + 8;

    // Row 1: 5 knobs — Gain, Detune, EQ Lo, EQ Mid, EQ Hi
    const auto gap1 = (getWidth() - (sWidth * 5)) / 6;
    auto placeSlider = [&](sliderController& s, juce::Label& lbl, int x, int y)
    {
        lbl.setBounds(x, y, sWidth, labelH);
        s.setBounds(x, y + labelH, sWidth, sHeight);
    };

    placeSlider(GainSlider,    labelGain,    gap1,                row1Y);
    placeSlider(DetuneSlider,  labelDetune,  gap1 * 2 + sWidth,   row1Y);
    placeSlider(EqLowSlider,   labelEqLow,   gap1 * 3 + sWidth*2, row1Y);
    placeSlider(EqMidSlider,   labelEqMid,   gap1 * 4 + sWidth*3, row1Y);
    placeSlider(EqHighSlider,  labelEqHigh,  gap1 * 5 + sWidth*4, row1Y);

    // Row 2: 4 knobs — Lowpass, Highpass, Vel>LP, Vel>Atk
    const auto gap2 = (getWidth() - (sWidth * 4)) / 5;
    placeSlider(LowpassSlider,      labelLowpass,  gap2,                row2Y);
    placeSlider(HighpassSlider,     labelHighpass, gap2 * 2 + sWidth,   row2Y);
    placeSlider(VelToLowpassSlider, labelVelToLP,  gap2 * 3 + sWidth*2, row2Y);
    placeSlider(VelToAttackSlider,  labelVelToAtk, gap2 * 4 + sWidth*3, row2Y);
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
    if (sliderName == "EqLow") {
        mEqLowAttachment.reset();
        mEqLowAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), parameterID, EqLowSlider);
    }
    if (sliderName == "EqMid") {
        mEqMidAttachment.reset();
        mEqMidAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), parameterID, EqMidSlider);
    }
    if (sliderName == "EqHigh") {
        mEqHighAttachment.reset();
        mEqHighAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), parameterID, EqHighSlider);
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