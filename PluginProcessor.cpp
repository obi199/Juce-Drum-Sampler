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
                            thumbnail(512, mFormatManager, thumbnailCache), 
    mAPVSTATE(*this, nullptr, "PARAMETERS", createParameters())
#endif
{
    
    mFormatManager.registerBasicFormats();
    mAPVSTATE.state.addListener(this);

    for (int i = 0; i < numVoices; i++) {
        mSampler.addVoice(new juce::SamplerVoice());
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
    updateADSR();
    mSamplerate = sampleRate;
    totalSamplesProcessed = 0;
    
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
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
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

    if (mShouldUpdate) updateADSR();
    

    juce::MidiMessage m;
    juce::MidiBuffer::Iterator it{midiMessages};
    int sample;

    while (it.getNextEvent(m, sample))
    {
        if (m.isNoteOn())
            mIsNotePlayed = true;
        else if(m.isNoteOff())
            mIsNotePlayed = false;
    }

    mSampleCount = mIsNotePlayed ? mSampleCount += buffer.getNumSamples():0;
    //mSampleCount += buffer.getNumSamples();
    currentPositionInSeconds = mSampleCount / mSamplerate;

    mSampler.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

   /* for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            channelData[sample] = channelData[sample] * juce::Decibels::decibelsToGain(gain);
        }
        
    }*/

    //buffer.applyGain(juce::Decibels::decibelsToGain(gain));

   
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
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void DrumSamplerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}



//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DrumSamplerAudioProcessor();
}

void DrumSamplerAudioProcessor::loadFile(const juce::String& path)
{   
    mSampler.clearSounds();

    auto file = juce::File(path);

    mFormatReader = mFormatManager.createReaderFor(file);
    
    //auto sampleLength = static_cast<int>(mFormatReader->lengthInSamples);
    //mWaveForm.setSize(1, sampleLength);
    //mFormatReader->read(&mWaveForm,0, sampleLength, 0, true, false );
    //auto buffer = mWaveForm.getReadPointer(0);
    //for (int sample = 0; sample < mWaveForm.getNumSamples(); ++sample)
    //{
    //    DBG(buffer[sample]);
    //}
    if (mFormatReader != nullptr) {
        thumbnail.setSource(new juce::FileInputSource(file));
        juce::BigInteger range;
        range.setRange(60, 1, true);
        mSampler.addSound(new juce::SamplerSound("Sample", *mFormatReader, range, 60, 0.01, 0.1, 10.0));
    }

    updateADSR();

}

void DrumSamplerAudioProcessor::playFile(int noteNumber)
{
     mSampler.noteOn(1, noteNumber, (juce::uint8)7);
}

void DrumSamplerAudioProcessor::getValue() 
{
    //DBG("Volume: " << gain);
}

void DrumSamplerAudioProcessor::updateADSR() {

    mADSRparams.attack = mAPVSTATE.getRawParameterValue("ATTACK")->load();
    mADSRparams.decay = mAPVSTATE.getRawParameterValue("DECAY")->load();
    mADSRparams.sustain = mAPVSTATE.getRawParameterValue("SUSTAIN")->load();
    mADSRparams.release = mAPVSTATE.getRawParameterValue("RELEASE")->load();


    for (int i = 0; i < mSampler.getNumSounds(); ++i)
    {
        if (auto sound = dynamic_cast<juce::SamplerSound*>(mSampler.getSound(i).get()))
        {
            sound->setEnvelopeParameters(mADSRparams);
        }
    }

}


void DrumSamplerAudioProcessor::valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property)
{
    mShouldUpdate = true;
}


juce::AudioProcessorValueTreeState::ParameterLayout DrumSamplerAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;
    
    parameters.push_back(std::make_unique<juce::AudioParameterFloat> ("ATTACK", "Attack", 0.0f, 1.0f, 0.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>("DECAY", "Decay", 0.0f, 1.0f, 0.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>("RELEASE", "Release", 0.0f, 1.0f, 0.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>("SUSTAIN", "Sustain", 0.0f, 1.0f, 1.0f));

    return{ parameters.begin(), parameters.end()};
}

    