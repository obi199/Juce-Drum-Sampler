#pragma once

#include <JuceHeader.h>

class CustomSamplerSound : public juce::SamplerSound
{
public:
    CustomSamplerSound(const juce::String& name,
        juce::AudioFormatReader& source,
        const juce::BigInteger& midiNotes,
        int midiNoteForNormalPitch,
        double attackTimeSecs,
        double releaseTimeSecs,
        double maxSampleLengthSeconds)
        : SamplerSound(name, source, midiNotes, midiNoteForNormalPitch,
            attackTimeSecs, releaseTimeSecs, maxSampleLengthSeconds),
        startOffset(0.0f)
    {
        adsrParams.attack = attackTimeSecs;
        adsrParams.release = releaseTimeSecs;
    }

    void setStartOffset(float newOffset) {
        startOffset = juce::jlimit(0.0f, 1.0f, newOffset);
    }

    float getStartOffset() const { return startOffset; }

    void setEnvelopeParameters(const juce::ADSR::Parameters& params) {
        adsrParams = params;
    }
    const juce::ADSR::Parameters& getEnvelopeParameters() const {
        return adsrParams;
    }

private:
    float startOffset;
    juce::ADSR::Parameters adsrParams;
};

class CustomSamplerVoice : public juce::SamplerVoice
{
public:
    int currentSamplePos = 0;
    juce::ADSR adsr;

    void startNote(int midiNoteNumber, float velocity,
        juce::SynthesiserSound* s, int currentPitchWheelPosition) override
    {
        // Call base class first to set up the sound
        juce::SamplerVoice::startNote(midiNoteNumber, velocity, s, currentPitchWheelPosition);
        
        // Reset and set positions AFTER base class initialization
        currentSamplePos = 0;
        
        if (auto* sound = dynamic_cast<CustomSamplerSound*>(s))
        {
            // Set the starting position based on offset
            float offset = sound->getStartOffset();
            if (auto* data = sound->getAudioData())
            {
                auto numSamples = data->getNumSamples();
                currentSamplePos = static_cast<int>(offset * numSamples);
            }
            
            adsr.setSampleRate(getSampleRate());
            adsr.setParameters(sound->getEnvelopeParameters());
            adsr.noteOn();
        }
    }

    void stopNote(float velocity, bool allowTailOff) override
    {
        if (allowTailOff)
            adsr.noteOff();
        else
        {
            clearCurrentNote();
            adsr.reset();
        }
        currentSamplePos = 0;
    }

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (auto* playingSound = dynamic_cast<CustomSamplerSound*>(getCurrentlyPlayingSound().get()))
        {
            auto* data = playingSound->getAudioData();
            if (data == nullptr) return;

            int numChannels = outputBuffer.getNumChannels();
            int sampleLength = data->getNumSamples();

            for (int i = 0; i < numSamples; ++i)
            {
                if (currentSamplePos >= sampleLength)
                {
                    clearCurrentNote();
                    break;
                }

                float envelopeValue = adsr.getNextSample();
                for (int channel = 0; channel < numChannels; ++channel)
                {
                    float sample = data->getSample(channel % data->getNumChannels(), currentSamplePos);
                    outputBuffer.addSample(channel, startSample + i, sample * envelopeValue);
                }
                ++currentSamplePos;
            }
        }
    }
};
