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

            auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4);

            auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
            auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
            auto lineW = juce::jmin(8.0f, radius * 0.2f);
            auto arcRadius = radius - lineW * 0.5f;

            // --- Background Track ---
            juce::Path backgroundArc;
            backgroundArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
            g.setColour(outline.withAlpha(0.2f));
            g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // --- Value Track ---
            if (slider.isEnabled())
            {
                juce::Path valueArc;
                valueArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius, 0.0f, rotaryStartAngle, toAngle, true);
                g.setColour(fill);
                g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }

            // --- Knob 3D Body ---
            auto knobRadius = radius - lineW * 1.5f;
            auto knobBounds = juce::Rectangle<float>(bounds.getCentreX() - knobRadius, bounds.getCentreY() - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);

            // 1. Heavy Outer Drop Shadow
            g.setColour(juce::Colours::black.withAlpha(0.6f));
            g.fillEllipse(knobBounds.translated(0.0f, 3.5f));

            // 2. Main Outer Bevel
            juce::ColourGradient outerBevelGrad(juce::Colours::lightgrey.brighter(0.2f), knobBounds.getTopLeft(),
                                              juce::Colours::lightgrey.darker(0.4f), knobBounds.getBottomRight(), false);
            g.setGradientFill(outerBevelGrad);
            g.fillEllipse(knobBounds);

            // 3. Inner Face (Slightly smaller)
            auto faceBounds = knobBounds.reduced(2.5f);
            juce::ColourGradient faceGrad(juce::Colours::lightgrey.brighter(0.15f), faceBounds.getTopLeft(),
                                        juce::Colours::lightgrey.darker(0.15f), faceBounds.getBottomRight(), false);
            g.setGradientFill(faceGrad);
            g.fillEllipse(faceBounds);

            // 4. Subtle Radial Shine on Top
            juce::ColourGradient shineGrad(juce::Colours::white.withAlpha(0.15f), faceBounds.getCentreX(), faceBounds.getY(),
                                         juce::Colours::transparentWhite, faceBounds.getCentreX(), faceBounds.getCentreY(), true);
            g.setGradientFill(shineGrad);
            g.fillEllipse(faceBounds);

            // 5. Rim highlight
            g.setColour(juce::Colours::white.withAlpha(0.3f));
            g.drawEllipse(knobBounds.reduced(0.5f), 0.5f);
            g.setColour(juce::Colours::black.withAlpha(0.5f));
            g.drawEllipse(knobBounds, 1.0f);

            // --- 3D Pointer ---
            auto pointerLength = knobRadius * 0.7f;
            auto pointerThickness = 3.5f;
            
            juce::Path p;
            p.addRoundedRectangle(-pointerThickness * 0.5f, -knobRadius + 2.0f, pointerThickness, pointerLength, 1.0f);
            
            auto transform = juce::AffineTransform::rotation(toAngle).translated(bounds.getCentreX(), bounds.getCentreY());
            
            // Pointer Shadow
            g.setColour(juce::Colours::black.withAlpha(0.4f));
            g.fillPath(p, transform.translated(1.0f, 1.0f));
            
            // Pointer Body
            g.setColour(juce::Colours::white.darker(0.1f));
            g.fillPath(p, transform);
            
            // Pointer Top Highlight
            g.setColour(juce::Colours::white);
            g.drawLine(bounds.getCentreX(), bounds.getCentreY(), 
                       bounds.getCentreX() + std::sin(toAngle) * (knobRadius - 2.0f), 
                       bounds.getCentreY() - std::cos(toAngle) * (knobRadius - 2.0f), 0.5f);
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
    sliderController CompressionSlider{ "Comp" };
    sliderController VolumeSlider{ "Vol" };

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
    std::unique_ptr<SliderAttachment> mCompressionAttachment;
    std::unique_ptr<SliderAttachment> mVolumeAttachment;

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
    juce::Label labelCompression{ {}, "Comp" };
    juce::Label labelVolume{ {}, "Vol" };

    DrumSamplerAudioProcessor& audioProcessor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(controlSlidersBlock)
};
