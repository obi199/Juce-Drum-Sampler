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
    
    // Initialize sample files vector with empty files for all pads
    sampleFiles.resize(NUM_PADS);

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

void DrumSamplerAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String DrumSamplerAudioProcessor::getProgramName (int index)
{
    return {};
}

void DrumSamplerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void DrumSamplerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mSampler.setCurrentPlaybackSampleRate(sampleRate);
    DBG(mSamplerate);
    updateADSR(1);
    mSamplerate = sampleRate;
    
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
    
    // Update gain BEFORE rendering (important!)
    if (mUpdateCount > 0)
    {
        updateADSR(sampleIndex);
        gain = updateGain(sampleIndex);
        mUpdateCount--;
        if (mUpdateCount == 0)
            mShouldUpdate = false;
    }
    
    if (mSampler.getNumVoices() > 0 && mSampler.getVoice(0)->isVoiceActive()) {
        mSampleCount += buffer.getNumSamples();
    }
    else {
        mSampleCount = 0;  // Reset when sample playback stops
    }

    currentPositionInSeconds = mSampleCount / mSamplerate;
    
    // Render sampler
    mSampler.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // Apply gain AFTER rendering
    if (gain > 0.0f && gain != 1.0f) {
        buffer.applyGain(gain);
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

void DrumSamplerAudioProcessor::loadFile(const juce::String& path, int noteNumber, juce::String buttonName)
{   
    auto file = juce::File(path);
    auto fileName = file.getFileName();
    
    // Get pad index from MIDI note
    int padIndex = getPadIndexFromMidiNote(noteNumber);
    if (padIndex == -1)
    {
        DBG("Invalid MIDI note: " << noteNumber);
        return;
    }

    mFormatReader = mFormatManager.createReaderFor(file);
    if (mFormatReader == nullptr)
    {
        DBG("Failed to create format reader for: " << fileName);
        return;
    }
    
    sampleRate = mFormatReader->sampleRate;
    totalLength = mFormatReader->lengthInSamples;
    
    thumbnail.setSource(new juce::FileInputSource(file));
    
    // Safely store the file
    if (padIndex < static_cast<int>(sampleFiles.size()))
    {
        sampleFiles[padIndex] = file;
    }
    else
    {
        sampleFiles.push_back(file);
    }

    juce::BigInteger range;
    range.setRange(noteNumber, 1, true);
    
    // Create CustomSamplerSound
    auto* sound = new CustomSamplerSound(fileName, *mFormatReader, range, noteNumber, 0.01, 0.1, 10.0);
    
    // Set the offset for this MIDI note
    if (newPositionSec > 0) 
    {
        float normalizedPosition = newPositionSec / (totalLength / sampleRate);
        sampleOffsets[noteNumber] = normalizedPosition;
        sound->setStartOffset(normalizedPosition);
    }
    
    mSampler.addSound(sound);
    updateADSR(padIndex);
}

void DrumSamplerAudioProcessor::playFile(int midiNoteNumber)
{
    DBG("playFile called for note: " << midiNoteNumber);
    DBG("newPositionSec: " << newPositionSec);
    DBG("totalLength: " << totalLength);
    DBG("sampleRate: " << sampleRate);
    
    // Get the current gain and use it as velocity (0-127 range)
    int padIndex = getPadIndexFromMidiNote(midiNoteNumber);
    if (padIndex == -1) return;
    
    float currentGain = updateGain(padIndex);
    juce::uint8 velocity = static_cast<juce::uint8>(juce::jlimit(1.0f, 127.0f, currentGain * 127.0f));
    
    DBG("Current gain for pad " << padIndex << ": " << currentGain);
    DBG("Velocity: " << static_cast<int>(velocity));
    
    // Update the offset before playing if needed
    if (newPositionSec > 0)
    {
        DBG("Updating offset...");
        float normalizedPosition = newPositionSec / (totalLength / (float)sampleRate);
        // Clamp to ensure we don't start past the last sample
        normalizedPosition = juce::jlimit(0.0f, 0.9999f, normalizedPosition);
        DBG("normalizedPosition: " << normalizedPosition);
        
        // Find and update the sound for this MIDI note
        for (int i = 0; i < mSampler.getNumSounds(); ++i)
        {
            if (auto* sound = dynamic_cast<CustomSamplerSound*>(mSampler.getSound(i).get()))
            {
                // Check if this sound is mapped to our MIDI note
                if (sound->appliesToNote(midiNoteNumber))
                {
                    sampleOffsets[midiNoteNumber] = normalizedPosition;
                    sound->setStartOffset(normalizedPosition);
                    DBG("Set offset for note " << midiNoteNumber << " to " << normalizedPosition);
                    break;
                }
            }
        }
    }
    else
    {
        DBG("newPositionSec is " << newPositionSec << " - resetting offset to 0");
        // Reset offset to 0 when no position is set
        for (int i = 0; i < mSampler.getNumSounds(); ++i)
        {
            if (auto* sound = dynamic_cast<CustomSamplerSound*>(mSampler.getSound(i).get()))
            {
                if (sound->appliesToNote(midiNoteNumber))
                {
                    sound->setStartOffset(0.0f);
                    DBG("Reset offset for note " << midiNoteNumber << " to 0");
                    break;
                }
            }
        }
    }

    // Now trigger the note with velocity based on gain
    DBG("Calling noteOn for note: " << midiNoteNumber << " with velocity: " << static_cast<int>(velocity));
    mSampler.noteOn(1, midiNoteNumber, velocity);
    samplePlayed(midiNoteNumber);
}

void DrumSamplerAudioProcessor::getValue() 
{
    //DBG("Volume: " << gain);
}

int DrumSamplerAudioProcessor::samplePlayed(int midiNote) 
{
    int padIndex = getPadIndexFromMidiNote(midiNote);
    if (padIndex != -1)
    {
        sampleIndex = padIndex;
    }
    return sampleIndex;
}

juce::AudioProcessorValueTreeState::ParameterLayout DrumSamplerAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    parameters.push_back(std::make_unique<juce::AudioParameterFloat>("GAIN", "Gain", juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>("ATTACK", "Attack", 0.0f, 1.0f, 0.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>("DECAY", "Decay", 0.0f, 1.0f, 1.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>("RELEASE", "Release", 0.0f, 1.0f, 0.5f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>("SUSTAIN", "Sustain", 0.0f, 1.0f, 1.0f));
    //second pad
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>("GAIN2", "Gain", juce::NormalisableRange<float>(0.0f, 1.0f), 0.1f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>("ATTACK2", "Attack", 0.0f, 1.0f, 0.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>("DECAY2", "Decay", 0.0f, 1.0f, 1.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>("RELEASE2", "Release", 0.0f, 1.0f, 0.5f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>("SUSTAIN2", "Sustain", 0.0f, 1.0f, 1.0f));

    return{ parameters.begin(), parameters.end() };
}

float DrumSamplerAudioProcessor::updateGain(int padIndex) 
{
    // Ensure we have a valid index
    if (padIndex < 0 || padIndex >= NUM_PADS)
    {
        DBG("Invalid pad index for gain update: " << padIndex);
        return gain;  // Return current gain if invalid index
    }
    
    juce::String paramID = padIndex == 0 ? "GAIN" : "GAIN2";
    
    if (auto* param = mAPVSTATE.getRawParameterValue(paramID))
    {
        float newGain = param->load();
        gain = newGain;
        DBG("Updated gain for pad " << padIndex << ": " << gain);
    }
    else
    {
        DBG("Failed to get parameter: " << paramID);
    }
    
    return gain;
}


// Update ADSR per pad with improved error handling
void DrumSamplerAudioProcessor::updateADSR(int padIndex) 
{
    DBG("Updating ADSR for pad " << padIndex);
    
    if (padIndex < 0 || padIndex >= NUM_PADS)
    {
        DBG("Invalid pad index: " << padIndex);
        return;
    }
    
    // Build parameter IDs based on pad index
    juce::String attackID = padIndex == 0 ? "ATTACK" : "ATTACK2";
    juce::String decayID = padIndex == 0 ? "DECAY" : "DECAY2";
    juce::String sustainID = padIndex == 0 ? "SUSTAIN" : "SUSTAIN2";
    juce::String releaseID = padIndex == 0 ? "RELEASE" : "RELEASE2";
    
    // Load parameters safely
    if (auto* attackParam = mAPVSTATE.getRawParameterValue(attackID))
        mADSRparams.attack = attackParam->load();
        
    if (auto* decayParam = mAPVSTATE.getRawParameterValue(decayID))
        mADSRparams.decay = decayParam->load();
        
    if (auto* sustainParam = mAPVSTATE.getRawParameterValue(sustainID))
        mADSRparams.sustain = sustainParam->load();
        
    if (auto* releaseParam = mAPVSTATE.getRawParameterValue(releaseID))
        mADSRparams.release = releaseParam->load();

    // Apply to CustomSamplerSound
    if (padIndex < mSampler.getNumSounds())
    {
        if (auto* sound = dynamic_cast<CustomSamplerSound*>(mSampler.getSound(padIndex).get()))
        {
            DBG("Found sound at index " << padIndex);
            sound->setEnvelopeParameters(mADSRparams);
        }
        else
        {
            DBG("No sound found at index " << padIndex);
        }
    }
}

void DrumSamplerAudioProcessor::valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property)
{
    DBG("property changed: " << property);
    mShouldUpdate = true;
    mUpdateCount = 2;  // Update for 2 frames to ensure changes are applied
}

//==============================================================================
// Safe sample file accessors
//==============================================================================

juce::File DrumSamplerAudioProcessor::getSampleFile(int padIndex) const
{
    if (padIndex >= 0 && padIndex < static_cast<int>(sampleFiles.size()))
    {
        return sampleFiles[padIndex];
    }
    return juce::File();
}

bool DrumSamplerAudioProcessor::hasSampleLoaded(int padIndex) const
{
    auto file = getSampleFile(padIndex);
    return file.existsAsFile() && file.hasFileExtension("wav;mp3;aiff;flac");
}

int DrumSamplerAudioProcessor::getPadIndexFromMidiNote(int midiNote) const
{
    for (int i = 0; i < NUM_PADS; ++i)
    {
        if (MIDI_NOTES[i] == midiNote)
            return i;
    }
    return -1;  // Invalid MIDI note
}