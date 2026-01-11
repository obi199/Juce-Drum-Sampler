/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
//#include "CustomSamplerVoice.h"#
#define MAX_VOICES 16



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
                            thumbnail(512, mFormatManager, thumbnailCache), 
    mAPVSTATE(*this, nullptr, "PARAMETERS", createParameters())
#endif
{
    mFormatManager.registerBasicFormats();
    mAPVSTATE.state.addListener(this);

    for (int i = 0; i < MAX_VOICES; i++) {
        //mSampler.addVoice(new juce::SamplerVoice());
        mSampler.addVoice(new CustomSamplerVoice());


    }
}

DrumSamplerAudioProcessor::~DrumSamplerAudioProcessor()
{
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
      
    if (mShouldUpdate) updateADSR(sampleIndex);
    
    if (mSampler.getNumVoices() > 0 && mSampler.getVoice(0)->isVoiceActive()) {
        mSampleCount += buffer.getNumSamples();
    }
    else {
        mSampleCount = 0;  // Reset when sample playback stops
    }

    currentPositionInSeconds = mSampleCount / mSamplerate;
    
    mSampler.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    if (mShouldUpdate) {
        gain = updateGain(sampleIndex);
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
    int i = 0;

    mFormatReader = mFormatManager.createReaderFor(file);
    sampleRate = mFormatReader->sampleRate;
    totalLength = mFormatReader->lengthInSamples;
    
    if (mFormatReader != nullptr) {
        thumbnail.setSource(new juce::FileInputSource(file));
        sampleFiles.push_back(file);

        if (buttonName == "myButton1") {
            sampleFiles[0] = file;
            i = 0;
        }
        if (buttonName == "myButton2") {
            sampleFiles[1] = file;
            i = 1;
        }

        juce::BigInteger range;
        range.setRange(noteNumber, 1, true);
        
        // Erstelle einen CustomSamplerSound
        auto* sound = new CustomSamplerSound(fileName, *mFormatReader, range, noteNumber, 0.01, 0.1, 10.0);
        
        // Setze den Offset für diese MIDI-Note
        if (newPositionSec > 0) {
            float normalizedPosition = newPositionSec / (totalLength / sampleRate);
            sampleOffsets[noteNumber] = normalizedPosition;
            sound->setStartOffset(normalizedPosition);
        }
        
        mSampler.addSound(sound);
        updateADSR(i);
    }

   
}

void DrumSamplerAudioProcessor::playFile(int midiNoteNumber)
{
    DBG("playFile called for note: " << midiNoteNumber);
    DBG("newPositionSec: " << newPositionSec);
    DBG("totalLength: " << totalLength);
    DBG("sampleRate: " << sampleRate);
    
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

    // Now trigger the note
    DBG("Calling noteOn for note: " << midiNoteNumber);
    mSampler.noteOn(1, midiNoteNumber, (juce::uint8)7);
    samplePlayed(midiNoteNumber);

}

void DrumSamplerAudioProcessor::getValue() 
{
    //DBG("Volume: " << gain);
}

int DrumSamplerAudioProcessor::samplePlayed(int midiNote) {
    switch (midiNote) {
    case 60:
       sampleIndex = 0;
       break;
    case 61:
       sampleIndex = 1;
       break;
    default:
        break;
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

float DrumSamplerAudioProcessor::updateGain(int i) {
    if (i == 0) {
        gain = mAPVSTATE.getRawParameterValue("GAIN")->load();
    }
    if (i == 1) {
        gain = mAPVSTATE.getRawParameterValue("GAIN2")->load();
    }
    return gain;
  
}


// new update ADSR per each sound
void DrumSamplerAudioProcessor::updateADSR(int i) {
    DBG("Updating ADSR for sound " << i);
    if(i == 0){
        mADSRparams.attack = mAPVSTATE.getRawParameterValue("ATTACK")->load();
        mADSRparams.decay = mAPVSTATE.getRawParameterValue("DECAY")->load();
        mADSRparams.sustain = mAPVSTATE.getRawParameterValue("SUSTAIN")->load();
        mADSRparams.release = mAPVSTATE.getRawParameterValue("RELEASE")->load();
    }

    if (i == 1) {
        mADSRparams.attack = mAPVSTATE.getRawParameterValue("ATTACK2")->load();
        mADSRparams.decay = mAPVSTATE.getRawParameterValue("DECAY2")->load();
        mADSRparams.sustain = mAPVSTATE.getRawParameterValue("SUSTAIN2")->load();
        mADSRparams.release = mAPVSTATE.getRawParameterValue("RELEASE2")->load();
    }

    // Use CustomSamplerSound instead of juce::SamplerSound
    if (auto sound = dynamic_cast<CustomSamplerSound*>(mSampler.getSound(i).get()))
    {
        DBG("Found sound at index " << i);
        sound->setEnvelopeParameters(mADSRparams);
    }
    else
    {
        DBG("No sound found at index " << i);
    }
}

void DrumSamplerAudioProcessor::valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property)
{
    DBG("property changed: " << property);
    mShouldUpdate = true;
}