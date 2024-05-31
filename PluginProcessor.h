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
class DrumSamplerAudioProcessor  : public juce::AudioProcessor
   
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
    void getValue();
  

     
private:
    //==============================================================================
    juce::Synthesiser mSampler;
    const int numVoices{ 3 };
    juce::AudioBuffer<float> mWaveForm;

   
  

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumSamplerAudioProcessor)
};

