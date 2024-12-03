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
    //setSize(200, 100);
    addAndMakeVisible(&GainSlider);
    GainSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    //VolSlider.setRange(-50.0f, 0.0f, 0.01f);
    //VolSlider.setValue(-12.0f);
    mGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), "GAIN", GainSlider);
    //VolSlider.onValueChange = [this] { sliderValueChanged(&VolSlider); };
    GainSlider.attachLabel(&GainSlider, false);

    addAndMakeVisible(&AttackSlider);
    AttackSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    //AttackSlider.setRange(0.0f, 1.0f, 0.01f);
    mAttackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), "ATTACK", AttackSlider);
    //AttackSlider.setValue(0.0f);
    //AttackSlider.onValueChange = [this] { sliderValueChanged(&AttackSlider); };
    AttackSlider.attachLabel(&AttackSlider, false);

    addAndMakeVisible(&DecaySlider);
    DecaySlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    //DecaySlider.setRange(0.0f, 1.0f, 0.01f);
    //DecaySlider.setValue(1.0f);
    mDecayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), "DECAY", DecaySlider);
    //DecaySlider.onValueChange = [this] { sliderValueChanged(&DecaySlider); };
    DecaySlider.attachLabel(&DecaySlider, false);

    addAndMakeVisible(&ReleaseSlider);
    ReleaseSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    //ReleaseSlider.setRange(0.0f, 1.0f, 0.01f);
    //ReleaseSlider.setValue(1.0f);
    mReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), "RELEASE", ReleaseSlider);
    //ReleaseSlider.onValueChange = [this] { sliderValueChanged(&ReleaseSlider); };
    ReleaseSlider.attachLabel(&ReleaseSlider, false);

    addAndMakeVisible(&SustainSlider);
    SustainSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    //SustainSlider.setRange(0.0f, 1.0f, 0.01f);
   /* SustainSlider.setValue(1.0f);*/
    mSustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), "SUSTAIN", SustainSlider);
    //SustainSlider.onValueChange = [this] { sliderValueChanged(&SustainSlider); };
    SustainSlider.attachLabel(&SustainSlider, false);

}

//void controlSlidersBlock::sliderValueChanged(juce::Slider* slider) {
//    if (slider == &VolSlider)
//    {
//        audioProcessor.gain = VolSlider.getValue();
//        DBG(VolSlider.getValue());
//    }
//    else if (slider == &AttackSlider)
//    {
//        audioProcessor.getADSRparams().attack = AttackSlider.getValue();
//    }
//    else if (slider == &DecaySlider)
//    {
//        audioProcessor.getADSRparams().decay = DecaySlider.getValue();
//    }
//    else if (slider == &SustainSlider)
//    {
//        audioProcessor.getADSRparams().sustain = SustainSlider.getValue();
//    }
//    else if (slider == &ReleaseSlider)
//    {
//        audioProcessor.getADSRparams().release = ReleaseSlider.getValue();
//    }
//
//    audioProcessor.updateADSR();
//
//}

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
    //juce::Rectangle<int> area(0, 0, getWidth(), getHeight());
    //g.fillRect(area);

}
