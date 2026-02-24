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
    
    if (mSampler.getNumVoices() > 0 && mSampler.getVoice(0)->isVoiceActive()) {
        mSampleCount += buffer.getNumSamples();
    }
    else {
        mSampleCount = 0;
    }

    currentPositionInSeconds = mSampleCount / mSamplerate;
    
    mSampler.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // Apply gain from the active pad
    if (currentGain > 0.0f && std::abs(currentGain - 1.0f) > 1e-6f) {
        buffer.applyGain(currentGain);
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
    juce::uint8 velocity = static_cast<juce::uint8>(juce::jlimit(1.0f, 127.0f, padGain * 127.0f));
    
    // Use the global newPositionSec to update the per-pad offset if it was changed by the user dragging the line
    float audioLengthSeconds = (totalLength / (float)mSampleRateInt);
    if (audioLengthSeconds > 0)
    {
        float offsetRatio = juce::jlimit(0.0f, 0.9999f, newPositionSec / audioLengthSeconds);
        // Note: We don't necessarily want to ALWAYS update the stored offset to current UI position
        // during playback if it was just triggered. 
        // But the previous implementation did it this way.
        
        // Actually, ButtonClicked already sets the UI position from stored offset.
        // If we dragging, newPositionSec is updated.
    }
    
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
        parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("GAIN" + suffix, 1), "Gain", juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));
        parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("ATTACK" + suffix, 1), "Attack", 0.0f, 1.0f, 0.0f));
        parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("DECAY" + suffix, 1), "Decay", 0.0f, 1.0f, 1.0f));
        parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("RELEASE" + suffix, 1), "Release", 0.0f, 1.0f, 0.5f));
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
        currentGain = param->load();
        pads[static_cast<size_t>(padIndex)].gain = currentGain;
    }
    
    return currentGain;
}

void DrumSamplerAudioProcessor::updateADSR(int padIndex) 
{
    if (padIndex < 0 || padIndex >= NUM_PADS) return;
    
    auto suffix = (padIndex == 0) ? juce::String("") : juce::String(padIndex + 1);
    
    if (auto* p = mAPVSTATE.getRawParameterValue("ATTACK" + suffix)) pads[padIndex].adsr.attack = p->load();
    if (auto* p = mAPVSTATE.getRawParameterValue("DECAY" + suffix)) pads[padIndex].adsr.decay = p->load();
    if (auto* p = mAPVSTATE.getRawParameterValue("SUSTAIN" + suffix)) pads[padIndex].adsr.sustain = p->load();
    if (auto* p = mAPVSTATE.getRawParameterValue("RELEASE" + suffix)) pads[padIndex].adsr.release = p->load();

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
        sampleIndex = padIndex;
    return sampleIndex;
}

//==============================================================================
void DrumSamplerAudioProcessor::valueTreePropertyChanged(juce::ValueTree& /*treeWhosePropertyHasChanged*/, const juce::Identifier& property)
{
    mShouldUpdate = true;
    mUpdateCount = 2;
}