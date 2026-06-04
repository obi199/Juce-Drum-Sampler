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

    void setEndOffset(float newOffset) { endOffset = juce::jlimit(0.0f, 1.0f, newOffset); }
    float getEndOffset() const { return endOffset; }

    void setFadeStartOffset(float newOffset) { fadeStartOffset = juce::jlimit(0.0f, 1.0f, newOffset); }
    float getFadeStartOffset() const { return fadeStartOffset; }

    void setEnvelopeParameters(const juce::ADSR::Parameters& newParams) { adsrParams = newParams; }
    const juce::ADSR::Parameters& getEnvelopeParameters() const { return adsrParams; }

    void setVelToAttack(float amount) { velToAttack = juce::jlimit(0.0f, 1.0f, amount); }
    float getVelToAttack() const { return velToAttack; }

    void setDetuneSemitones(float st) { detuneSemitones = juce::jlimit(-24.0f, 24.0f, st); }
    float getDetuneSemitones() const { return detuneSemitones; }

    void setLowpassCutoff(float hz) { lowpassCutoff = juce::jlimit(200.0f, 20000.0f, hz); }
    float getLowpassCutoff() const { return lowpassCutoff; }

    void setHighpassCutoff(float hz) { highpassCutoff = juce::jlimit(20.0f, 18000.0f, hz); }
    float getHighpassCutoff() const { return highpassCutoff; }

    void setGainLinear(float g) { gainLinear = juce::jlimit(0.0f, 4.0f, g); }
    float getGainLinear() const { return gainLinear; }

    void setOutputBusIndex(int index) { outputBusIndex = index; }
    int getOutputBusIndex() const { return outputBusIndex; }

    double getSourceSampleRate() const { return sourceSampleRate; }

private:
    float startOffset = 0.0f;
    float endOffset = 1.0f;
    float fadeStartOffset = 1.0f;  // position (0-1) where fade-out begins; 1.0 = no fade
    float velToAttack = 0.0f;
    float detuneSemitones = 0.0f;
    float lowpassCutoff = 20000.0f;
    float highpassCutoff = 20.0f;
    float gainLinear = 1.0f;
    int outputBusIndex = 0;
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
        fadeTriggered = false;
        // Apply a velocity curve: velocity^0.5 gives a more natural, louder response
        // at high velocities compared to linear mapping
        currentGain = std::pow(velocity, 0.5f);

        if (auto* sound = dynamic_cast<CustomSamplerSound*>(s))
        {
            // Multiply pad gain so each voice is independent — no global buffer gain needed
            currentGain *= sound->getGainLinear();
            // Compute pitch ratio so files at any sample rate play at correct speed
            double hostRate = getSampleRate();
            double srcRate = sound->getSourceSampleRate();
            double detune = std::pow(2.0, sound->getDetuneSemitones() / 12.0);
            pitchRatio = (hostRate > 0.0) ? (srcRate / hostRate * detune) : 1.0;

            // Set up lowpass filter coefficients
            float cutoff = sound->getLowpassCutoff();
            auto coeffs = juce::IIRCoefficients::makeLowPass(hostRate, (double)cutoff);
            for (auto& f : lowpassFilters)
            {
                f.setCoefficients(coeffs);
                f.reset();
            }

            // Set up highpass filter coefficients
            float hpCutoff = sound->getHighpassCutoff();
            auto hpCoeffs = juce::IIRCoefficients::makeHighPass(hostRate, (double)hpCutoff);
            for (auto& f : highpassFilters)
            {
                f.setCoefficients(hpCoeffs);
                f.reset();
            }

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
                // Add extra attack for lower velocities. 
                // At velocity 1.0, extra is 0. 
                // At velocity 0.0, we add up to 0.5s of attack time.
                float extraAttack = (1.0f - velocity) * velAmount * 0.5f;
                params.attack += extraAttack;
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
            fadeTriggered = false;
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
            int endSamplePos   = static_cast<int>(playingSound->getEndOffset()       * static_cast<float>(sampleLength));
            int fadeSamplePos  = static_cast<int>(playingSound->getFadeStartOffset() * static_cast<float>(sampleLength));

            // Update filter coefficients every block so knob changes take effect in real-time
            double hostRate = getSampleRate();
            auto lpCoeffs = juce::IIRCoefficients::makeLowPass(hostRate, (double)playingSound->getLowpassCutoff());
            auto hpCoeffs = juce::IIRCoefficients::makeHighPass(hostRate, (double)playingSound->getHighpassCutoff());
            for (int ch = 0; ch < 2; ++ch)
            {
                lowpassFilters[ch].setCoefficients(lpCoeffs);
                highpassFilters[ch].setCoefficients(hpCoeffs);
            }

            for (int i = 0; i < numSamples; ++i)
            {
                int pos0 = static_cast<int>(currentSamplePos);
                if (pos0 >= sampleLength - 1 || pos0 >= endSamplePos)
                {
                    clearCurrentNote();
                    adsr.reset();
                    break;
                }

                // Trigger fade-out (release) when we reach the fade-start position
                if (!fadeTriggered && pos0 >= fadeSamplePos)
                {
                    adsr.noteOff();
                    fadeTriggered = true;
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

                int busOffset = playingSound->getOutputBusIndex() * 2;
                
                for (int channel = 0; channel < 2; ++channel)
                {
                    int targetCh = busOffset + channel;
                    if (targetCh >= numChannels) break;

                    int srcCh = channel % data->getNumChannels();
                    float s0 = data->getSample(srcCh, pos0);
                    float s1 = data->getSample(srcCh, pos1);
                    float sample = s0 + frac * (s1 - s0);
                    int filterIdx = juce::jmin(channel, 1);
                    sample = lowpassFilters[filterIdx].processSingleSampleRaw(sample);
                    sample = highpassFilters[filterIdx].processSingleSampleRaw(sample);
                    outputBuffer.addSample(targetCh, startSample + i, sample * envelopeValue);
                }
                currentSamplePos += pitchRatio;
            }
        }
    }

    void renderNextBlock(juce::AudioBuffer<double>&, int, int) override {}

    int getNextSamplePos() const { return static_cast<int>(currentSamplePos); }

private:
    double currentSamplePos = 0.0;
    bool fadeTriggered = false;
    double pitchRatio = 1.0;
    float currentGain = 1.0f;
    juce::ADSR adsr;
    juce::IIRFilter lowpassFilters[2];
    juce::IIRFilter highpassFilters[2];
};
