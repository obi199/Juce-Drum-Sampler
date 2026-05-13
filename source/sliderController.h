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
    sliderController(juce::String);
    ~sliderController() override;

private:
    struct NoBoxLookAndFeel : public juce::LookAndFeel_V4
    {
        void drawTextEditorOutline(juce::Graphics&, int, int, juce::TextEditor&) override {}

        juce::Label* createSliderTextBox(juce::Slider& slider) override
        {
            auto* label = juce::LookAndFeel_V4::createSliderTextBox(slider);
            label->setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
            label->setColour(juce::Label::outlineWhenEditingColourId, juce::Colours::transparentBlack);
            return label;
        }
    };

    NoBoxLookAndFeel noBoxLAF;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(sliderController)
};



class controlSlidersBlock : public juce::Component
{
public:
    controlSlidersBlock(DrumSamplerAudioProcessor&);
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
    sliderController StartOffsetSlider{ "Start" };
    sliderController VelToAttackSlider{ "Vel>Atk" };
    sliderController DetuneSlider{ "Detune" };
    sliderController LowpassSlider{ "Lowpass" };
    sliderController HighpassSlider{ "Highpass" };

    std::unique_ptr<SliderAttachment> mGainAttachment;
    std::unique_ptr<SliderAttachment> mAttackAttachment;
    std::unique_ptr<SliderAttachment> mDecayAttachment;
    std::unique_ptr<SliderAttachment> mSustainAttachment;
    std::unique_ptr<SliderAttachment> mReleaseAttachment;
    std::unique_ptr<SliderAttachment> mStartOffsetAttachment;
    std::unique_ptr<SliderAttachment> mVelToAttackAttachment;
    std::unique_ptr<SliderAttachment> mDetuneAttachment;
    std::unique_ptr<SliderAttachment> mLowpassAttachment;
    std::unique_ptr<SliderAttachment> mHighpassAttachment;

    // Name labels positioned above each knob by the parent component
    juce::Label labelGain{ {}, "Gain" };
    juce::Label labelAttack{ {}, "Attack" };
    juce::Label labelDecay{ {}, "Decay" };
    juce::Label labelSustain{ {}, "Sustain" };
    juce::Label labelRelease{ {}, "Release" };
    juce::Label labelStart{ {}, "Start" };
    juce::Label labelDetune{ {}, "Detune" };
    juce::Label labelLowpass{ {}, "Lowpass" };
    juce::Label labelHighpass{ {}, "Highpass" };
    juce::Label labelVelToAtk{ {}, "Vel>Atk" };

    DrumSamplerAudioProcessor& audioProcessor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(controlSlidersBlock)
};