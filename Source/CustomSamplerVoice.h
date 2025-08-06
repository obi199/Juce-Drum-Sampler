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
    }

    void setStartOffset(float newOffset) {
        startOffset = juce::jlimit(0.0f, 1.0f, newOffset);
    }

    float getStartOffset() const { return startOffset; }

private:
    float startOffset;
};

class CustomSamplerVoice : public juce::SamplerVoice
{
public:
    void setStartOffset(float newOffset) {
        startOffset = juce::jlimit(0.0f, 1.0f, newOffset);
    }

    void startNote(int midiNoteNumber, float velocity,
        juce::SynthesiserSound* s, int currentPitchWheelPosition) override
    {
        if (auto* sound = dynamic_cast<juce::SamplerSound*>(s))
        {
            if (auto* data = sound->getAudioData())
            {
                auto numSamples = data->getNumSamples();
                startSamplePos = static_cast<int>(startOffset * numSamples);
            }
        }
        currentSamplePos = startSamplePos;
        juce::SamplerVoice::startNote(midiNoteNumber, velocity, s, currentPitchWheelPosition);
    }

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (auto* playingSound = dynamic_cast<juce::SamplerSound*>(getCurrentlyPlayingSound().get()))
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

                for (int channel = 0; channel < numChannels; ++channel)
                {
                    float sample = data->getSample(channel % data->getNumChannels(), currentSamplePos);
                    outputBuffer.addSample(channel, startSample + i, sample);
                }
                ++currentSamplePos;
            }
        }
    }

private:
    float startOffset = 0.0f;
    int startSamplePos = 0;
    int currentSamplePos = 0;
};
