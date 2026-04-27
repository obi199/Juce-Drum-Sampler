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
    for (int i = 0; i < NUM_PADS; ++i)
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
    
    // Update gain and ADSR for the currently active pad before rendering
    if (mUpdateCount > 0)
    {
        updateADSR(sampleIndex);
        currentGain = updateGain(sampleIndex);
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

    // Apply gain from the active pad
    if (currentGain > 0.0f && std::abs(currentGain - 1.0f) > 1e-6f) {
        buffer.applyGain(currentGain);
    }

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
    
    float padGain = updateGain(padIndex);
    juce::uint8 velocity = static_cast<juce::uint8>(juce::jlimit(1.0f, 127.0f, padGain * 100.0f));
    
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
        // Shift by -18 dB to keep the same internal volume (-18 dB displayed as 0 dB)
        currentGain = juce::Decibels::decibelsToGain(dbValue - 18.0f, -78.0f);
        pads[static_cast<size_t>(padIndex)].gain = currentGain;
    }
    
    return currentGain;
}

void DrumSamplerAudioProcessor::updateADSR(int padIndex) 
{
    if (padIndex < 0 || padIndex >= NUM_PADS) return;
    
    auto suffix = (padIndex == 0) ? juce::String("") : juce::String(padIndex + 1);
    
    // Get normalized 0-1 values from APVTS with safe fallbacks
    float attackNorm = 0.02f;   // Default attack 0.02 = 2% of audio length
    float decayNorm = 0.5f;     // Default decay 0.5 = 50% of audio length
    float sustainNorm = 1.0f;   // Default sustain 1.0 = full level
    float releaseNorm = 0.2f;   // Default release 0.2 = 20% of audio length
    
    if (auto* p = mAPVSTATE.getRawParameterValue("ATTACK" + suffix)) 
        attackNorm = juce::jlimit(0.0f, 1.0f, p->load());
    if (auto* p = mAPVSTATE.getRawParameterValue("DECAY" + suffix)) 
        decayNorm = juce::jlimit(0.0f, 1.0f, p->load());
    if (auto* p = mAPVSTATE.getRawParameterValue("SUSTAIN" + suffix)) 
        sustainNorm = juce::jlimit(0.0f, 1.0f, p->load());
    if (auto* p = mAPVSTATE.getRawParameterValue("RELEASE" + suffix)) 
        releaseNorm = juce::jlimit(0.0f, 1.0f, p->load());

    // Get audio length for this pad to scale ADSR times appropriately
    float audioLengthSecs = 1.0f;  // Default fallback to 1 second
    for (int i = 0; i < mSampler.getNumSounds(); ++i)
    {
        if (auto* sound = dynamic_cast<CustomSamplerSound*>(mSampler.getSound(i).get()))
        {
            if (sound->appliesToNote(pads[padIndex].midiNote))
            {
                if (auto* data = sound->getAudioData())
                {
                    audioLengthSecs = (float)data->getNumSamples() / (float)mSamplerate;
                }
                break;
            }
        }
    }

    // ADSR params are fractions of total audio length — independent of start offset.
    // The start offset only shifts where playback begins, not how long each ADSR phase is.
    float rawAttackSecs  = attackNorm  * audioLengthSecs;
    float rawDecaySecs   = decayNorm   * audioLengthSecs;
    float rawReleaseSecs = releaseNorm * audioLengthSecs;
    
    // Total envelope time (attack + decay + release, not including sustain duration)
    float totalEnvelopeTime = rawAttackSecs + rawDecaySecs + rawReleaseSecs;
    
    // If total time exceeds available audio length, scale all phases proportionally
    float scaleFactor = 1.0f;
    if (totalEnvelopeTime > audioLengthSecs * 0.95f)  // Leave 5% margin
    {
        scaleFactor = (audioLengthSecs * 0.95f) / totalEnvelopeTime;
    }
    
    // Apply scaling and ensure minimum values
    float attackSecs  = juce::jlimit(0.001f, audioLengthSecs, rawAttackSecs  * scaleFactor);
    float decaySecs   = juce::jlimit(0.001f, audioLengthSecs, rawDecaySecs   * scaleFactor);
    float releaseSecs = juce::jlimit(0.001f, audioLengthSecs, rawReleaseSecs * scaleFactor);

    DBG("ADSR Update Pad " << padIndex << ": attack=" << attackSecs << "s (norm=" << attackNorm 
        << "), decay=" << decaySecs << "s, sustain=" << sustainNorm << ", release=" << releaseSecs 
        << "s | Audio length=" << audioLengthSecs << "s, Scale=" << scaleFactor 
        << ", Total=" << (attackSecs + decaySecs + releaseSecs) << "s");

    // Set ADSR parameters with actual time values
    pads[padIndex].adsr.attack = attackSecs;
    pads[padIndex].adsr.decay = decaySecs;
    pads[padIndex].adsr.sustain = sustainNorm;  // Sustain is level (0-1), not time
    pads[padIndex].adsr.release = releaseSecs;

    // Apply to the relevant sound
    for (int i = 0; i < mSampler.getNumSounds(); ++i)
    {
        if (auto* sound = dynamic_cast<CustomSamplerSound*>(mSampler.getSound(i).get()))
        {
            if (sound->appliesToNote(pads[padIndex].midiNote))
            {
                sound->setEnvelopeParameters(pads[padIndex].adsr);
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
void DrumSamplerAudioProcessor::valueTreePropertyChanged(juce::ValueTree& /*treeWhosePropertyHasChanged*/, const juce::Identifier& property)
{
    mShouldUpdate = true;
    mUpdateCount = 2;
}