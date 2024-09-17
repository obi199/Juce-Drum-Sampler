/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class DrumSamplerAudioProcessor  : public juce::AudioProcessor, public juce::ValueTree::Listener
   
{
public:
    //==============================================================================
    DrumSamplerAudioProcessor();
    ~DrumSamplerAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;

    void setStateInformation (const void* data, int sizeInBytes) override;

    void loadFile (const juce::String& path);
    //void loadFile(const juce::File& file);
    int getNumSamplerSounds() { return mSampler.getNumSounds(); }
    juce::AudioFormatManager mFormatManager;
   
    juce::AudioThumbnailCache thumbnailCache;                  
    juce::AudioThumbnail thumbnail;
    juce::AudioFormatReader* mFormatReader{ nullptr };
    void playFile(int noteNumber);
    float gain{ 0.5 };
 /*   float attack{ 0.5 };
    float decay{ 0.5 };*/
    void getValue();
    void updateADSR();
    juce::ADSR::Parameters& getADSRparams() { return mADSRparams; }
    juce::AudioProcessorValueTreeState& getAPVTS() { return mAPVSTATE; }
    std::atomic<bool>& isNotePlayed() { return mIsNotePlayed; }
    std::atomic<int>& getSampleCount() { return mSampleCount; }
    float getPosInSec() { return currentPositionInSeconds; }

     
private:
    //==============================================================================
    juce::Synthesiser mSampler;
    const int numVoices{ 3 };
    juce::AudioBuffer<float> mWaveForm;
    juce::ADSR::Parameters mADSRparams;
    juce::AudioProcessorValueTreeState mAPVSTATE;
    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property);
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    std::atomic<bool> mShouldUpdate{ false };
    std::atomic<bool> mIsNotePlayed{ false };
    std::atomic<int> mSampleCount{ false };
    int timeLinePosInSamples;
    float currentPositionInSeconds=0;
    double mSamplerate = 441000.0;
    float duration;
    float getCurrentSamplePosition();
   
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumSamplerAudioProcessor)
};

