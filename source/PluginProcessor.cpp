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
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
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
void DrumSamplerAudioProcessor::prepareToPlay (double newSampleRate, int /*samplesPerBlock*/)
{
    mSampler.setCurrentPlaybackSampleRate(newSampleRate);
    mSamplerate = newSampleRate;
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

    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void DrumSamplerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

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
    
    mSampler.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // Safety limiter/clamping to avoid harsh digital distortion
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
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

void DrumSamplerAudioProcessor::loadFile(const juce::String& path, int noteNumber, juce::String /*buttonName*/)
{   
    auto file = juce::File(path);
    int padIndex = getPadIndexFromMidiNote(noteNumber);
    if (padIndex == -1) return;

    mFormatReader = mFormatManager.createReaderFor(file);
    if (mFormatReader == nullptr) return;
    
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
    
    // Apply existing offset if any (from APVTS if available)
    float offset = getStartOffsetForNote(noteNumber);
    sound->setStartOffset(offset);
    
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
    
    // Get normalized 0-1 values from APVTS with safe fallbacks
    float attackNorm = 0.02f;
    float decayNorm = 0.5f;
    float sustainNorm = 1.0f;
    float releaseNorm = 0.2f;
    
    if (auto* p = mAPVSTATE.getRawParameterValue("ATTACK" + suffix)) 
        attackNorm = juce::jlimit(0.0f, 1.0f, p->load());
    if (auto* p = mAPVSTATE.getRawParameterValue("DECAY" + suffix)) 
        decayNorm = juce::jlimit(0.0f, 1.0f, p->load());
    if (auto* p = mAPVSTATE.getRawParameterValue("SUSTAIN" + suffix)) 
        sustainNorm = juce::jlimit(0.0f, 1.0f, p->load());
    if (auto* p = mAPVSTATE.getRawParameterValue("RELEASE" + suffix)) 
        releaseNorm = juce::jlimit(0.0f, 1.0f, p->load());

    // Maximum absolute times in seconds — independent of sample length so that
    // even very short drum hits can have a long, audible fade-in / fade-out.
    constexpr float maxAttackSecs  = 0.5f;   // 500ms max — practical for short drum hits
    constexpr float maxDecaySecs   = 0.25f;  // very short range for punchy percussive sounds
    constexpr float maxReleaseSecs = 2.0f;

    // Map the 0-1 slider values to absolute time ranges
    float attackSecs  = juce::jlimit(0.001f, maxAttackSecs,  attackNorm  * maxAttackSecs);
    float decaySecs   = juce::jlimit(0.001f, maxDecaySecs,   decayNorm   * maxDecaySecs);
    float releaseSecs = juce::jlimit(0.001f, maxReleaseSecs,  releaseNorm * maxReleaseSecs);

    DBG("ADSR Update Pad " << padIndex << ": attack=" << attackSecs << "s (norm=" << attackNorm 
        << "), decay=" << decaySecs << "s, sustain=" << sustainNorm << ", release=" << releaseSecs << "s");

    // Set ADSR parameters with actual time values
    pads[(size_t)padIndex].adsr.attack = attackSecs;
    pads[(size_t)padIndex].adsr.decay = decaySecs;
    pads[(size_t)padIndex].adsr.sustain = sustainNorm;  // Sustain is level (0-1), not time
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
void DrumSamplerAudioProcessor::valueTreePropertyChanged(juce::ValueTree& /*treeWhosePropertyHasChanged*/, const juce::Identifier& /*property*/)
{
    mShouldUpdate = true;
    mUpdateCount = 2;
}