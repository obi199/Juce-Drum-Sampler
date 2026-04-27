#pragma once

#include <JuceHeader.h>

/**
    CustomSamplerSound stores the audio data and metadata for a single drum hit.
*/
class CustomSamplerSound : public juce::SamplerSound
{
public:
    CustomSamplerSound(const juce::String& soundName,
                       juce::AudioFormatReader& source,
                       const juce::BigInteger& midiNoteRange,
                       int midiNoteForNormalPitch,
                       double attackTimeSecs,
                       double releaseTimeSecs,
                       double maxSampleLengthSeconds)
        : SamplerSound(soundName, source, midiNoteRange, midiNoteForNormalPitch,
                       attackTimeSecs, releaseTimeSecs, maxSampleLengthSeconds)
    {
        adsrParams.attack = static_cast<float>(attackTimeSecs);
        adsrParams.release = static_cast<float>(releaseTimeSecs);
    }

    void setStartOffset(float newOffset) { startOffset = juce::jlimit(0.0f, 1.0f, newOffset); }
    float getStartOffset() const { return startOffset; }

    void setEnvelopeParameters(const juce::ADSR::Parameters& newParams) { adsrParams = newParams; }
    const juce::ADSR::Parameters& getEnvelopeParameters() const { return adsrParams; }

private:
    float startOffset = 0.0f;
    juce::ADSR::Parameters adsrParams;
};

/**
    CustomSamplerVoice renders the audio for a CustomSamplerSound.
    It handles the start offset and the ADSR envelope.
*/
class CustomSamplerVoice : public juce::SamplerVoice
{
public:
    void startNote(int midiNoteNumber, float velocity,
                   juce::SynthesiserSound* s, int currentPitchWheelPosition) override
    {
        juce::SamplerVoice::startNote(midiNoteNumber, velocity, s, currentPitchWheelPosition);
        currentSamplePos = 0;
        currentGain = velocity;

        if (auto* sound = dynamic_cast<CustomSamplerSound*>(s))
        {
            float offset = sound->getStartOffset();
            if (auto* data = sound->getAudioData())
            {
                auto numSamples = data->getNumSamples();
                currentSamplePos = static_cast<int>(offset * static_cast<float>(numSamples));
            }

            adsr.setSampleRate(getSampleRate());
            adsr.setParameters(sound->getEnvelopeParameters());
            adsr.reset();   // ensure envelopeVal starts from 0, not a leftover sustain level
            adsr.noteOn();
        }
    }

    void stopNote(float /*velocity*/, bool allowTailOff) override
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
                    adsr.reset();
                    break;
                }

                float envelopeValue = adsr.getNextSample() * currentGain;
                for (int channel = 0; channel < numChannels; ++channel)
                {
                    float sample = data->getSample(channel % data->getNumChannels(), currentSamplePos);
                    outputBuffer.addSample(channel, startSample + i, sample * envelopeValue);
                }
                ++currentSamplePos;
            }
        }
    }

    void renderNextBlock(juce::AudioBuffer<double>&, int, int) override {}

    int getNextSamplePos() const { return currentSamplePos; }

private:
    int currentSamplePos = 0;
    float currentGain = 1.0f;
    juce::ADSR adsr;
};