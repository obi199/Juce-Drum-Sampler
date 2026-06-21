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

        void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
            const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override
        {
            auto outline = slider.findColour(juce::Slider::rotarySliderOutlineColourId);
            auto fill = slider.findColour(juce::Slider::rotarySliderFillColourId);

            auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(6);

            auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
            auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
            auto lineW = juce::jmin(4.0f, radius * 0.2f);
            auto arcRadius = radius - lineW * 0.5f;

            // Draw background arc
            juce::Path backgroundArc;
            backgroundArc.addCentredArc(bounds.getCentreX(),
                bounds.getCentreY(),
                arcRadius,
                arcRadius,
                0.0f,
                rotaryStartAngle,
                rotaryEndAngle,
                true);

            g.setColour(outline.withAlpha(0.3f));
            g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            if (slider.isEnabled())
            {
                juce::Path valueArc;
                valueArc.addCentredArc(bounds.getCentreX(),
                    bounds.getCentreY(),
                    arcRadius,
                    arcRadius,
                    0.0f,
                    rotaryStartAngle,
                    toAngle,
                    true);

                g.setColour(fill);
                g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }

            // --- Knob Body ---
            auto knobRadius = radius - lineW * 2.0f;
            auto knobBounds = juce::Rectangle<float>(bounds.getCentreX() - knobRadius, bounds.getCentreY() - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);

            // Shadow
            g.setColour(juce::Colours::black.withAlpha(0.5f));
            g.fillEllipse(knobBounds.translated(0, 2));

            // Main body
            juce::ColourGradient knobGrad(juce::Colours::lightgrey, knobBounds.getTopLeft(),
                juce::Colours::darkgrey.darker(0.8f), knobBounds.getBottomRight(), false);
            g.setGradientFill(knobGrad);
            g.fillEllipse(knobBounds);

            // Top shine
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.fillEllipse(knobBounds.reduced(2).translated(0, -1));

            // Outline
            g.setColour(juce::Colours::black.withAlpha(0.8f));
            g.drawEllipse(knobBounds, 1.0f);

            // --- Pointer ---
            juce::Path p;
            auto pointerLength = knobRadius * 0.45f;
            auto pointerThickness = 3.0f;
            p.addRoundedRectangle(-pointerThickness * 0.5f, -knobRadius + 2.0f, pointerThickness, pointerLength, 1.0f);
            p.applyTransform(juce::AffineTransform::rotation(toAngle).translated(bounds.getCentreX(), bounds.getCentreY()));

            g.setColour(juce::Colours::white);
            g.fillPath(p);
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
    sliderController DetuneSlider{ "Detune" };
    sliderController EqLowSlider{ "EQ Lo" };
    sliderController EqMidSlider{ "EQ Mid" };
    sliderController EqHighSlider{ "EQ Hi" };
    sliderController LowpassSlider{ "Lowpass" };
    sliderController HighpassSlider{ "Highpass" };
    sliderController VelToLowpassSlider{ "Vel>LP" };
    sliderController VelToAttackSlider{ "Vel>Atk" };
    sliderController DistortionSlider{ "Dist" };
    sliderController ReverbSlider{ "Reverb" };
    sliderController ReverbDecaySlider{ "Decay" };

    std::unique_ptr<SliderAttachment> mGainAttachment;
    std::unique_ptr<SliderAttachment> mDetuneAttachment;
    std::unique_ptr<SliderAttachment> mEqLowAttachment;
    std::unique_ptr<SliderAttachment> mEqMidAttachment;
    std::unique_ptr<SliderAttachment> mEqHighAttachment;
    std::unique_ptr<SliderAttachment> mLowpassAttachment;
    std::unique_ptr<SliderAttachment> mHighpassAttachment;
    std::unique_ptr<SliderAttachment> mVelToLowpassAttachment;
    std::unique_ptr<SliderAttachment> mVelToAttackAttachment;
    std::unique_ptr<SliderAttachment> mDistortionAttachment;
    std::unique_ptr<SliderAttachment> mReverbAttachment;
    std::unique_ptr<SliderAttachment> mReverbDecayAttachment;

    juce::Label labelGain{ {}, "Gain" };
    juce::Label labelDetune{ {}, "Detune" };
    juce::Label labelEqLow{ {}, "EQ Lo" };
    juce::Label labelEqMid{ {}, "EQ Mid" };
    juce::Label labelEqHigh{ {}, "EQ Hi" };
    juce::Label labelLowpass{ {}, "Lowpass" };
    juce::Label labelHighpass{ {}, "Highpass" };
    juce::Label labelVelToLP{ {}, "Vel>LP" };
    juce::Label labelVelToAtk{ {}, "Vel>Atk" };
    juce::Label labelDistortion{ {}, "Dist" };
    juce::Label labelReverb{ {}, "Reverb" };
    juce::Label labelReverbDecay{ {}, "Decay" };

    DrumSamplerAudioProcessor& audioProcessor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(controlSlidersBlock)
};
