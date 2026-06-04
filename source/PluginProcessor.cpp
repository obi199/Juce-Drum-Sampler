/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DrumSamplerAudioProcessor::DrumSamplerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Main", juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Pad 1", juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Pad 2", juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Pad 3", juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Pad 4", juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Pad 5", juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Pad 6", juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Pad 7", juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Pad 8", juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Pad 9", juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Pad 10", juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Pad 11", juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Pad 12", juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Pad 13", juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Pad 14", juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Pad 15", juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Pad 16", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),   thumbnailCache(5),                         
                            thumbnail(128, mFormatManager, thumbnailCache), 
    mAPVSTATE(*this, nullptr, "PARAMETERS", createParameters())
#endif
{
    mFormatManager.registerBasicFormats();
    mAPVSTATE.state.addListener(this);
    
    // Initialize pads
    for (size_t i = 0; i < (size_t)NUM_PADS; ++i)
    {
        pads[i].midiNote = MIDI_NOTES[i];
        pads[i].gain = 1.0f; // Default to unity gain (0.0 dB)
        
        auto suffix = (i == 0) ? juce::String("") : juce::String(i + 1);
        if (auto* p = mAPVSTATE.getRawParameterValue("VEL_TO_ATTACK" + suffix))
            lastVelToAttack[i] = p->load();
        else
            lastVelToAttack[i] = 0.0f;
    }

    for (int i = 0; i < MAX_VOICES; i++) {
        mSampler.addVoice(new CustomSamplerVoice());
    }
}

DrumSamplerAudioProcessor::~DrumSamplerAudioProcessor()
{
    mAPVSTATE.state.removeListener(this);
    mFormatReader = nullptr;
}

//==============================================================================
const juce::String DrumSamplerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DrumSamplerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DrumSamplerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DrumSamplerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DrumSamplerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DrumSamplerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DrumSamplerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DrumSamplerAudioProcessor::setCurrentProgram (int /*index*/)
{
}

const juce::String DrumSamplerAudioProcessor::getProgramName (int /*index*/)
{
    return {};
}

void DrumSamplerAudioProcessor::changeProgramName (int /*index*/, const juce::String& /*newName*/)
{
}

//==============================================================================
void DrumSamplerAudioProcessor::prepareToPlay (double newSampleRate, int samplesPerBlock)
{
    mSampler.setCurrentPlaybackSampleRate(newSampleRate);
    mSamplerate = newSampleRate;
    multiOutBuffer.setSize(32, samplesPerBlock);
    updateADSR(0);
}

void DrumSamplerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    //transportSource.releaseResources();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DrumSamplerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    for (auto& set : layouts.outputBuses)
    {
        if (set != juce::AudioChannelSet::stereo() && set != juce::AudioChannelSet::disabled())
            return false;
    }
    return true;
  #endif
}
#endif

void DrumSamplerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto numSamples = buffer.getNumSamples();

    // Clear all output buses
    for (int i = 0; i < getBusCount(false); ++i)
    {
        auto busBuffer = getBus(false, i)->getBusBuffer(buffer);
        busBuffer.clear();
    }

    // Detect incoming MIDI note-on events and update the active pad accordingly
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            int noteNumber = msg.getNoteNumber();
            int padIdx = getPadIndexFromMidiNote(noteNumber);
            if (padIdx != -1)
            {
                samplePlayed(noteNumber);
                mPadSwitchedFromMidi = true;
            }
        }
    }

    // Update gain and ADSR for the currently active pad before rendering
    if (mUpdateCount > 0)
    {
        updateADSR(sampleIndex);
        mUpdateCount--;
        if (mUpdateCount == 0)
            mShouldUpdate = false;
    }
    
    if (mSampler.getNumVoices() > 0)
    {
        bool anyActive = false;
        int latestSamplePos = 0;
        
        for (int i = 0; i < mSampler.getNumVoices(); ++i)
        {
            if (auto* v = dynamic_cast<CustomSamplerVoice*>(mSampler.getVoice(i)))
            {
                if (v->isVoiceActive())
                {
                    anyActive = true;
                    // We take the position from the first active voice we find, 
                    // or we could track which one was triggered last if needed.
                    latestSamplePos = v->getNextSamplePos();
                }
            }
        }
        
        if (anyActive)
        {
            mSampleCount = latestSamplePos;
            mIsNotePlayed = true;
        }
        else
        {
            mSampleCount = 0;
            mIsNotePlayed = false;
        }
    }
    else {
        mSampleCount = 0;
        mIsNotePlayed = false;
    }

    currentPositionInSeconds = static_cast<float>(mSampleCount) / static_cast<float>(mSamplerate);
    
    // Render into temporary buffer
    multiOutBuffer.clear();
    mSampler.renderNextBlock(multiOutBuffer, midiMessages, 0, numSamples);

    // Distribute from multiOutBuffer to output buses
    
    // 1. Main Mix (Bus 0)
    if (auto* mainBus = getBus(false, 0))
    {
        auto mainBuffer = mainBus->getBusBuffer(buffer);
        if (mainBuffer.getNumChannels() >= 2)
        {
            for (int i = 0; i < NUM_PADS; ++i)
            {
                mainBuffer.addFrom(0, 0, multiOutBuffer, i * 2, 0, numSamples);
                mainBuffer.addFrom(1, 0, multiOutBuffer, i * 2 + 1, 0, numSamples);
            }
        }
    }

    // 2. Individual Pads (Buses 1 to NUM_PADS)
    for (int i = 0; i < NUM_PADS; ++i)
    {
        if (auto* bus = getBus(false, i + 1))
        {
            auto busBuffer = bus->getBusBuffer(buffer);
            if (busBuffer.getNumChannels() >= 2)
            {
                busBuffer.addFrom(0, 0, multiOutBuffer, i * 2, 0, numSamples);
                busBuffer.addFrom(1, 0, multiOutBuffer, i * 2 + 1, 0, numSamples);
            }
        }
    }

    // Safety limiter/clamping (applying to all active channels in the main buffer)
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
            data[i] = juce::jlimit(-1.0f, 1.0f, data[i]);
    }
}

//==============================================================================
bool DrumSamplerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DrumSamplerAudioProcessor::createEditor()
{
    return new DrumSamplerAudioProcessorEditor (*this);
}

//==============================================================================
void DrumSamplerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = mAPVSTATE.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void DrumSamplerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(mAPVSTATE.state.getType()))
            mAPVSTATE.replaceState(juce::ValueTree::fromXml(*xmlState));
}



//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DrumSamplerAudioProcessor();
}

void DrumSamplerAudioProcessor::resetPadParametersToDefault(int padIndex)
{
    auto suffix = (padIndex == 0) ? juce::String("") : juce::String(padIndex + 1);

    auto setDefault = [&](const juce::String& id, float value)
    {
        if (auto* param = mAPVSTATE.getParameter(id + suffix))
            param->setValueNotifyingHost(param->convertTo0to1(value));
    };

    setDefault("GAIN",         0.0f);
    setDefault("ATTACK",       0.0f);
    setDefault("DECAY",        0.5f);
    setDefault("SUSTAIN",      1.0f);
    setDefault("RELEASE",      0.2f);
    setDefault("START_OFFSET", 0.0f);
    setDefault("END_OFFSET",   1.0f);
    setDefault("FADE_START",   0.8f);
    setDefault("FADE_END",     1.0f);
    setDefault("VEL_TO_ATTACK",0.0f);
    setDefault("DETUNE",       0.0f);
    setDefault("LOWPASS",      20000.0f);
    setDefault("HIGHPASS",     20.0f);
}

void DrumSamplerAudioProcessor::clearPad(int midiNoteNumber)
{
    int padIndex = getPadIndexFromMidiNote(midiNoteNumber);
    if (padIndex == -1) return;

    // Remove the sampler sound for this note
    for (int i = mSampler.getNumSounds() - 1; i >= 0; --i)
    {
        if (auto* sound = dynamic_cast<CustomSamplerSound*>(mSampler.getSound(i).get()))
        {
            if (sound->appliesToNote(midiNoteNumber))
            {
                mSampler.removeSound(i);
                break;
            }
        }
    }

    // Clear the pad file reference and reset parameters
    pads[static_cast<size_t>(padIndex)].sampleFile = juce::File{};
    pads[static_cast<size_t>(padIndex)].startOffset = 0.0f;
    resetPadParametersToDefault(padIndex);
}

void DrumSamplerAudioProcessor::loadFile(const juce::String& path, int noteNumber, juce::String /*buttonName*/)
{   
    auto file = juce::File(path);
    int padIndex = getPadIndexFromMidiNote(noteNumber);
    if (padIndex == -1) return;

    mFormatReader = mFormatManager.createReaderFor(file);
    if (mFormatReader == nullptr) return;

    // If this pad had no sample before, reset all its parameters to defaults
    // so leftover values from a previous session don't bleed in.
    bool wasEmpty = !pads[static_cast<size_t>(padIndex)].sampleFile.existsAsFile();
    if (wasEmpty)
        resetPadParametersToDefault(padIndex);
    
    mSampleRateInt = static_cast<int>(mFormatReader->sampleRate);
    totalLength = static_cast<int>(mFormatReader->lengthInSamples);
    
    thumbnail.setSource(new juce::FileInputSource(file));
    
    // Store in pad structure
    pads[static_cast<size_t>(padIndex)].sampleFile = file;

    juce::BigInteger range;
    range.setRange(noteNumber, 1, true);
    
    // Remove any existing sound for this MIDI note before adding the new one.
    // Without this, addSound stacks duplicates and updateADSR updates the stale
    // old sound first — leaving the new sound with no ADSR/filter settings.
    for (int i = mSampler.getNumSounds() - 1; i >= 0; --i)
    {
        if (auto* existing = dynamic_cast<CustomSamplerSound*>(mSampler.getSound(i).get()))
        {
            if (existing->appliesToNote(noteNumber))
            {
                mSampler.removeSound(i);
                break;
            }
        }
    }

    auto* sound = new CustomSamplerSound(file.getFileName(), *mFormatReader, range, noteNumber, 0.01, 0.1, 10.0);
    sound->setOutputBusIndex(padIndex);
    
    // Apply existing offset if any (from APVTS if available)
    float offset = getStartOffsetForNote(noteNumber);
    sound->setStartOffset(offset);
    float endOff = getEndOffsetForNote(noteNumber);
    sound->setEndOffset(endOff);
    {
        int padIndex2 = getPadIndexFromMidiNote(noteNumber);
        auto suffix2 = (padIndex2 == 0) ? juce::String("") : juce::String(padIndex2 + 1);
        float fadeStart = 0.8f;
        if (auto* v = mAPVSTATE.getRawParameterValue("FADE_START" + suffix2))
            fadeStart = v->load();
        sound->setFadeStartOffset(fadeStart);
    }
    
    mSampler.addSound(sound);
    updateADSR(padIndex);
}

void DrumSamplerAudioProcessor::playFile(int midiNoteNumber)
{
    int padIndex = getPadIndexFromMidiNote(midiNoteNumber);
    if (padIndex == -1) return;
    
    // Use a fixed velocity for GUI-triggered playback (0.0–1.0 range).
    // The gain slider is already applied to the output buffer in processBlock,
    // so deriving velocity from gain would double-apply it.
    float velocity = 100.0f / 127.0f;
    
    float offset = getStartOffsetForNote(midiNoteNumber);

    for (int i = 0; i < mSampler.getNumSounds(); ++i)
    {
        if (auto* sound = dynamic_cast<CustomSamplerSound*>(mSampler.getSound(i).get()))
        {
            if (sound->appliesToNote(midiNoteNumber))
            {
                sound->setStartOffset(offset);
                sound->setEndOffset(getEndOffsetForNote(midiNoteNumber));
                {
                    auto suffix2 = (padIndex == 0) ? juce::String("") : juce::String(padIndex + 1);
                    float fadeStart = 0.8f;
                    if (auto* v = mAPVSTATE.getRawParameterValue("FADE_START" + suffix2))
                        fadeStart = v->load();
                    sound->setFadeStartOffset(fadeStart);
                }
                break;
            }
        }
    }

    mSampler.noteOn(1, midiNoteNumber, velocity);
    samplePlayed(midiNoteNumber);
}

juce::AudioProcessorValueTreeState::ParameterLayout DrumSamplerAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    for (int i = 0; i < NUM_PADS; ++i)
    {
        auto suffix = (i == 0) ? juce::String("") : juce::String(i + 1);
        parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID("GAIN" + suffix, 1), 
            "Gain", 
            juce::NormalisableRange<float>(-42.0f, 24.0f, 0.1f, 1.5f), 
            0.0f,
            "dB"
        ));
        parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("ATTACK" + suffix, 1), "Attack", 0.0f, 1.0f, 0.02f));  // 0.02 = 20ms with 1s max
        parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("DECAY" + suffix, 1), "Decay", 0.0f, 1.0f, 0.5f));   // 0.5 = 500ms with 1s max
        parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("RELEASE" + suffix, 1), "Release", 0.0f, 1.0f, 0.2f)); // 0.2 = 200ms with 1s max
        parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("SUSTAIN" + suffix, 1), "Sustain", 0.0f, 1.0f, 1.0f));
        parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("START_OFFSET" + suffix, 1), "Start Offset", 0.0f, 1.0f, 0.0f));
        parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("END_OFFSET" + suffix, 1), "End Offset", 0.0f, 1.0f, 1.0f));
        parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("FADE_START" + suffix, 1), "Fade Start", 0.0f, 1.0f, 0.8f));
        parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("FADE_END" + suffix, 1), "Fade End", 0.0f, 1.0f, 1.0f));
        parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("VEL_TO_ATTACK" + suffix, 1), "Vel>Atk", 0.0f, 1.0f, 0.0f));
        parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID("DETUNE" + suffix, 1),
            "Detune",
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f),
            0.0f,
            "st"
        ));
        parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID("LOWPASS" + suffix, 1),
            "Lowpass",
            juce::NormalisableRange<float>(200.0f, 20000.0f, 1.0f, 0.3f),
            20000.0f,
            "Hz"
        ));
        parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID("HIGHPASS" + suffix, 1),
            "Highpass",
            juce::NormalisableRange<float>(20.0f, 18000.0f, 1.0f, 0.3f),
            20.0f,
            "Hz"
        ));
    }

    return { parameters.begin(), parameters.end() };
}

float DrumSamplerAudioProcessor::updateGain(int padIndex) 
{
    if (padIndex < 0 || padIndex >= NUM_PADS)
        return currentGain;
    
    auto suffix = (padIndex == 0) ? juce::String("") : juce::String(padIndex + 1);
    juce::String paramID = juce::String("GAIN") + suffix;
    
    if (auto* param = mAPVSTATE.getRawParameterValue(paramID))
    {
        float dbValue = param->load();
        currentGain = juce::Decibels::decibelsToGain(dbValue, -60.0f);
        pads[static_cast<size_t>(padIndex)].gain = currentGain;
    }
    
    return currentGain;
}

void DrumSamplerAudioProcessor::updateADSR(int padIndex) 
{
    if (padIndex < 0 || padIndex >= NUM_PADS) return;
    
    auto suffix = (padIndex == 0) ? juce::String("") : juce::String(padIndex + 1);
    
    // Get attack from APVTS
    float attackNorm = 0.02f;
    if (auto* p = mAPVSTATE.getRawParameterValue("ATTACK" + suffix))
        attackNorm = juce::jlimit(0.0f, 1.0f, p->load());

    constexpr float maxAttackSecs = 0.5f;
    float attackSecs = juce::jlimit(0.001f, maxAttackSecs, attackNorm * maxAttackSecs);

    // Compute release time from the distance between FADE_START and END_OFFSET positions
    float fadeStart = 0.8f;
    float fadeEnd   = 1.0f;
    if (auto* p = mAPVSTATE.getRawParameterValue("FADE_START" + suffix))
        fadeStart = p->load();
    if (auto* p = mAPVSTATE.getRawParameterValue("FADE_END" + suffix))
        fadeEnd = p->load();

    // Get sample duration in seconds from the loaded sound (if available)
    float sampleDurationSecs = 1.0f;
    for (int i = 0; i < mSampler.getNumSounds(); ++i)
    {
        if (auto* sound = dynamic_cast<CustomSamplerSound*>(mSampler.getSound(i).get()))
        {
            if (sound->appliesToNote(pads[(size_t)padIndex].midiNote))
            {
                if (auto* data = sound->getAudioData())
                    sampleDurationSecs = static_cast<float>(data->getNumSamples()) / static_cast<float>(mSamplerate);
                break;
            }
        }
    }

    float fadeSpan    = juce::jmax(0.0f, fadeEnd - fadeStart);
    float releaseSecs = juce::jmax(0.001f, fadeSpan * sampleDurationSecs);

    DBG("ADSR Pad " << padIndex << ": attack=" << attackSecs << "s, release=" << releaseSecs << "s");

    // sustain=1.0 so volume holds at full after attack.
    // noteOff is triggered inside the voice when position reaches FADE_START.
    // release = time from FADE_START to END_OFFSET.
    pads[(size_t)padIndex].adsr.attack  = attackSecs;
    pads[(size_t)padIndex].adsr.decay   = 0.001f;
    pads[(size_t)padIndex].adsr.sustain = 1.0f;
    pads[(size_t)padIndex].adsr.release = releaseSecs;

    // Apply to the relevant sound
    for (int i = 0; i < mSampler.getNumSounds(); ++i)
    {
        if (auto* sound = dynamic_cast<CustomSamplerSound*>(mSampler.getSound(i).get()))
        {
            if (sound->appliesToNote(pads[(size_t)padIndex].midiNote))
            {
                sound->setEnvelopeParameters(pads[(size_t)padIndex].adsr);

                // Bake the pad's dB gain into the sound so each voice uses its own gain
                float gainDb = 0.0f;
                if (auto* v = mAPVSTATE.getRawParameterValue("GAIN" + suffix))
                    gainDb = v->load();
                sound->setGainLinear(juce::Decibels::decibelsToGain(gainDb, -60.0f));

                float velToAtk = 0.0f;
                if (auto* v = mAPVSTATE.getRawParameterValue("VEL_TO_ATTACK" + suffix))
                    velToAtk = v->load();
                sound->setVelToAttack(velToAtk);

                float detuneSt = 0.0f;
                if (auto* v = mAPVSTATE.getRawParameterValue("DETUNE" + suffix))
                    detuneSt = v->load();
                sound->setDetuneSemitones(detuneSt);

                float lowpassHz = 20000.0f;
                if (auto* v = mAPVSTATE.getRawParameterValue("LOWPASS" + suffix))
                    lowpassHz = v->load();
                sound->setLowpassCutoff(lowpassHz);

                float highpassHz = 20.0f;
                if (auto* v = mAPVSTATE.getRawParameterValue("HIGHPASS" + suffix))
                    highpassHz = v->load();
                sound->setHighpassCutoff(highpassHz);

                float startOff = 0.0f;
                if (auto* v = mAPVSTATE.getRawParameterValue("START_OFFSET" + suffix))
                    startOff = v->load();
                sound->setStartOffset(startOff);

                float endOff = 1.0f;
                if (auto* v = mAPVSTATE.getRawParameterValue("END_OFFSET" + suffix))
                    endOff = v->load();
                sound->setEndOffset(endOff);

                float fadeStart = 0.8f;
                if (auto* v = mAPVSTATE.getRawParameterValue("FADE_START" + suffix))
                    fadeStart = v->load();
                sound->setFadeStartOffset(fadeStart);

                break;
            }
        }
    }
}

int DrumSamplerAudioProcessor::getPadIndexFromMidiNote(int midiNote) const
{
    for (int i = 0; i < NUM_PADS; ++i)
    {
        if (MIDI_NOTES[i] == midiNote)
            return i;
    }
    return -1;
}

void DrumSamplerAudioProcessor::setStartOffsetForNote(int midiNoteNumber, float offset)
{
    int padIndex = getPadIndexFromMidiNote(midiNoteNumber);
    if (padIndex != -1)
    {
        pads[static_cast<size_t>(padIndex)].startOffset = offset;
        
        auto suffix = (padIndex == 0) ? juce::String("") : juce::String(padIndex + 1);
        if (auto* param = mAPVSTATE.getParameter("START_OFFSET" + suffix))
        {
            param->setValueNotifyingHost(offset);
        }
    }
}

float DrumSamplerAudioProcessor::getStartOffsetForNote(int midiNoteNumber) const
{
    int padIndex = getPadIndexFromMidiNote(midiNoteNumber);
    if (padIndex != -1)
    {
        auto suffix = (padIndex == 0) ? juce::String("") : juce::String(padIndex + 1);
        if (auto* param = mAPVSTATE.getRawParameterValue("START_OFFSET" + suffix))
        {
            return param->load();
        }
        return pads[static_cast<size_t>(padIndex)].startOffset;
    }
    return 0.0f;
}

void DrumSamplerAudioProcessor::setEndOffsetForNote(int midiNoteNumber, float offset)
{
    int padIndex = getPadIndexFromMidiNote(midiNoteNumber);
    if (padIndex != -1)
    {
        auto suffix = (padIndex == 0) ? juce::String("") : juce::String(padIndex + 1);

        float oldEnd = 1.0f;
        if (auto* p = mAPVSTATE.getRawParameterValue("END_OFFSET" + suffix))
            oldEnd = p->load();

        float delta = offset - oldEnd;

        if (auto* param = mAPVSTATE.getParameter("END_OFFSET" + suffix))
            param->setValueNotifyingHost(param->convertTo0to1(offset));

        if (std::abs(delta) > 0.0001f)
        {
            auto shiftParam = [&](const juce::String& name) {
                if (auto* pv = mAPVSTATE.getRawParameterValue(name + suffix))
                {
                    if (auto* p = mAPVSTATE.getParameter(name + suffix))
                    {
                        float newVal = juce::jlimit(0.0f, 1.0f, pv->load() + delta);
                        p->setValueNotifyingHost(p->convertTo0to1(newVal));
                    }
                }
            };
            shiftParam("FADE_START");
            shiftParam("FADE_END");
        }

        for (int i = 0; i < mSampler.getNumSounds(); ++i)
        {
            if (auto* sound = dynamic_cast<CustomSamplerSound*>(mSampler.getSound(i).get()))
            {
                if (sound->appliesToNote(midiNoteNumber))
                {
                    sound->setEndOffset(offset);
                    break;
                }
            }
        }

        // Recompute release time since it depends on END_OFFSET
        updateADSR(padIndex);
    }
}

float DrumSamplerAudioProcessor::getEndOffsetForNote(int midiNoteNumber) const
{
    int padIndex = getPadIndexFromMidiNote(midiNoteNumber);
    if (padIndex != -1)
    {
        auto suffix = (padIndex == 0) ? juce::String("") : juce::String(padIndex + 1);
        if (auto* param = mAPVSTATE.getRawParameterValue("END_OFFSET" + suffix))
            return param->load();
    }
    return 1.0f;
}

float DrumSamplerAudioProcessor::getFadeStartForNote(int midiNoteNumber) const
{
    int padIndex = getPadIndexFromMidiNote(midiNoteNumber);
    if (padIndex != -1)
    {
        auto suffix = (padIndex == 0) ? juce::String("") : juce::String(padIndex + 1);
        if (auto* param = mAPVSTATE.getRawParameterValue("FADE_START" + suffix))
            return param->load();
    }
    return 0.8f;
}

float DrumSamplerAudioProcessor::getFadeEndForNote(int midiNoteNumber) const
{
    int padIndex = getPadIndexFromMidiNote(midiNoteNumber);
    if (padIndex != -1)
    {
        auto suffix = (padIndex == 0) ? juce::String("") : juce::String(padIndex + 1);
        if (auto* param = mAPVSTATE.getRawParameterValue("FADE_END" + suffix))
            return param->load();
    }
    return 1.0f;
}

int DrumSamplerAudioProcessor::samplePlayed(int midiNote) 
{
    int padIndex = getPadIndexFromMidiNote(midiNote);
    if (padIndex != -1)
    {
        sampleIndex = padIndex;
        mSampleCount = 0; // Reset sample count when a note is played
    }
    return sampleIndex;
}

//==============================================================================
void DrumSamplerAudioProcessor::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    auto propName = property.toString();
    if (propName.startsWith("VEL_TO_ATTACK"))
    {
        auto suffix = propName.substring(13); // "VEL_TO_ATTACK" is 13 chars
        int padIdx = suffix.isEmpty() ? 0 : suffix.getIntValue() - 1;
        
        if (padIdx >= 0 && padIdx < NUM_PADS)
        {
            float newVal = tree.getProperty(property);
            float delta = newVal - lastVelToAttack[static_cast<size_t>(padIdx)];
            lastVelToAttack[static_cast<size_t>(padIdx)] = newVal;

            if (std::abs(delta) > 0.0001f)
            {
                if (auto* p = mAPVSTATE.getParameter("ATTACK" + suffix))
                {
                    float currentVal = p->getValue(); // normalized 0-1
                    float newValAtk = juce::jlimit(0.0f, 1.0f, currentVal + delta);
                    p->setValueNotifyingHost(newValAtk);
                }
            }
        }
    }

    mShouldUpdate = true;
    mUpdateCount = 2;
}