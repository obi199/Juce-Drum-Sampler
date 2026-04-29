/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "CustomSamplerVoice.h"

//==============================================================================
// DrumPad structure to hold state for each pad
//==============================================================================
struct DrumPad
{
    juce::File sampleFile;
    int midiNote = 0;
    juce::ADSR::Parameters adsr;
    float gain = 0.2f;
    float startOffset = 0.0f;
};

//==============================================================================
// Configuration Constants
static constexpr int MAX_VOICES = 16;
static constexpr int NUM_PADS = 16;
static constexpr int MIDI_NOTES[NUM_PADS] = { 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 };

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
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override {}

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

    void loadFile (const juce::String& path, int noteNumber, juce::String buttonName);
    int getNumSamplerSounds() { return mSampler.getNumSounds(); }
    
    // Pad management
    juce::File getSampleFile(int padIndex) const { return pads[static_cast<size_t>(padIndex)].sampleFile; }
    bool hasSampleLoaded(int padIndex) const { return pads[static_cast<size_t>(padIndex)].sampleFile.existsAsFile(); }
    int getPadIndexFromMidiNote(int midiNote) const;
    
    juce::AudioFormatManager mFormatManager;
    juce::AudioThumbnailCache thumbnailCache;                  
    juce::AudioThumbnail thumbnail;
    juce::AudioFormatReader* mFormatReader{ nullptr };
    
    void playFile(int);
    void updateADSR(int);
    float updateGain(int);
    
    juce::ADSR::Parameters& getADSRparams() { return pads[static_cast<size_t>(sampleIndex)].adsr; }
    juce::AudioProcessorValueTreeState& getAPVTS() { return mAPVSTATE; }
    
    std::atomic<bool>& isNotePlayed() { return mIsNotePlayed; }
    std::atomic<int>& getSampleCount() { return mSampleCount; }
    bool checkAndClearPadSwitchedFromMidi() { return mPadSwitchedFromMidi.exchange(false); }
    float getPosInSec() { return currentPositionInSeconds; }
    int getCurrentPadIndex() const { return sampleIndex; }
    
    int samplePlayed(int midiNote);
    float newPositionSec = 0;
    int newSampleCount = 0;
    int totalLength = 0;
    int mSampleRateInt = 44100;
    int mSampleStart = 0;

    void setStartOffsetForNote(int midiNoteNumber, float offset);
    float getStartOffsetForNote(int midiNoteNumber) const;
     
private:
    //==============================================================================
    juce::Synthesiser mSampler;
    std::array<DrumPad, NUM_PADS> pads;
    
    juce::AudioProcessorValueTreeState mAPVSTATE;
    
    void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) override;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    
    std::atomic<bool> mShouldUpdate{ false };
    std::atomic<int> mUpdateCount{ 0 };
    std::atomic<bool> mIsNotePlayed{ false };
    std::atomic<int> mSampleCount{ 0 };
    std::atomic<bool> mPadSwitchedFromMidi{ false };
    
    float currentPositionInSeconds = 0;
    double mSamplerate = 48000.0;
    int sampleIndex = 0;
    float currentGain = 0.2f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumSamplerAudioProcessor)
};

