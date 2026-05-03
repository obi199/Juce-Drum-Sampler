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
                       attackTimeSecs, releaseTimeSecs, maxSampleLengthSeconds),
          sourceSampleRate(source.sampleRate)
    {
        adsrParams.attack = static_cast<float>(attackTimeSecs);
        adsrParams.release = static_cast<float>(releaseTimeSecs);
    }

    void setStartOffset(float newOffset) { startOffset = juce::jlimit(0.0f, 1.0f, newOffset); }
    float getStartOffset() const { return startOffset; }

    void setEnvelopeParameters(const juce::ADSR::Parameters& newParams) { adsrParams = newParams; }
    const juce::ADSR::Parameters& getEnvelopeParameters() const { return adsrParams; }

    void setVelToAttack(float amount) { velToAttack = juce::jlimit(0.0f, 1.0f, amount); }
    float getVelToAttack() const { return velToAttack; }

    double getSourceSampleRate() const { return sourceSampleRate; }

private:
    float startOffset = 0.0f;
    float velToAttack = 0.0f;
    double sourceSampleRate = 44100.0;
    juce::ADSR::Parameters adsrParams;
};

/**
    CustomSamplerVoice renders the audio for a CustomSamplerSound.
    It handles the start offset, sample-rate conversion, and the ADSR envelope.
*/
class CustomSamplerVoice : public juce::SamplerVoice
{
public:
    void startNote(int midiNoteNumber, float velocity,
                   juce::SynthesiserSound* s, int currentPitchWheelPosition) override
    {
        juce::SamplerVoice::startNote(midiNoteNumber, velocity, s, currentPitchWheelPosition);
        currentSamplePos = 0.0;
        // Apply a velocity curve: velocity^0.5 gives a more natural, louder response
        // at high velocities compared to linear mapping
        currentGain = std::pow(velocity, 0.5f);

        if (auto* sound = dynamic_cast<CustomSamplerSound*>(s))
        {
            // Compute pitch ratio so files at any sample rate play at correct speed
            double hostRate = getSampleRate();
            pitchRatio = (hostRate > 0.0) ? (sound->getSourceSampleRate() / hostRate) : 1.0;

            float offset = sound->getStartOffset();
            if (auto* data = sound->getAudioData())
            {
                auto numSamples = data->getNumSamples();
                currentSamplePos = static_cast<double>(offset) * static_cast<double>(numSamples);
            }

            adsr.setSampleRate(hostRate);

            // Modulate attack time based on velocity and Vel>Atk amount
            auto params = sound->getEnvelopeParameters();
            float velAmount = sound->getVelToAttack();
            if (velAmount > 0.0f)
            {
                float velFactor = 1.0f - (velocity * velAmount);
                params.attack = juce::jmax(0.001f, params.attack * velFactor);
            }
            adsr.setParameters(params);

            adsr.reset();   // ensure envelopeVal starts from 0, not a leftover sustain level
            adsr.noteOn();
        }
    }

    void stopNote(float /*velocity*/, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            adsr.noteOff();
        }
        else
        {
            clearCurrentNote();
            adsr.reset();
            currentSamplePos = 0.0;
        }
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
                int pos0 = static_cast<int>(currentSamplePos);
                if (pos0 >= sampleLength - 1)
                {
                    clearCurrentNote();
                    adsr.reset();
                    break;
                }

                float envelopeValue = adsr.getNextSample() * currentGain;

                if (!adsr.isActive())
                {
                    clearCurrentNote();
                    adsr.reset();
                    currentSamplePos = 0.0;
                    break;
                }

                // Linear interpolation between adjacent samples
                int pos1 = pos0 + 1;
                float frac = static_cast<float>(currentSamplePos - static_cast<double>(pos0));

                for (int channel = 0; channel < numChannels; ++channel)
                {
                    int srcCh = channel % data->getNumChannels();
                    float s0 = data->getSample(srcCh, pos0);
                    float s1 = data->getSample(srcCh, pos1);
                    float sample = s0 + frac * (s1 - s0);
                    outputBuffer.addSample(channel, startSample + i, sample * envelopeValue);
                }
                currentSamplePos += pitchRatio;
            }
        }
    }

    void renderNextBlock(juce::AudioBuffer<double>&, int, int) override {}

    int getNextSamplePos() const { return static_cast<int>(currentSamplePos); }

private:
    double currentSamplePos = 0.0;
    double pitchRatio = 1.0;
    float currentGain = 1.0f;
    juce::ADSR adsr;
};
