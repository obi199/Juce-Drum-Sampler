# Quick Start Guide for Future Development

## Project Overview at a Glance

**What it is**: A 16-pad drum sampler VST3/AU/Standalone plugin in JUCE  
**What you can do**: Load samples per-pad, control ADSR/filters/pitch, multi-output routing  
**Why it matters**: Professional-grade audio plugin with real-time DSP  

---

## Directory Structure

```
Juce-Drum-Sampler/
├── README.md                        ← Start here for overview
├── CODEBASE_ARCHITECTURE.md         ← For technical deep-dive
├── DEVELOPMENT_GUIDELINES.md        ← For code style & best practices
├── IMPLEMENTATION_SUMMARY.md        ← What was fixed (you are here)
├── CMakeLists.txt                   ← Build configuration
├── JUCE/                            ← JUCE framework (git submodule)
├── source/
│   ├── PluginProcessor.h/.cpp       ← Audio engine + parameters
│   ├── PluginEditor.h/.cpp          ← GUI layout + user interaction
│   ├── CustomSamplerVoice.h          ← DSP: voice rendering
│   ├── DragAndDropButton.h/.cpp      ← Drag-drop component
│   ├── sliderController.h/.cpp       ← Custom slider
│   └── waveFormEditor.h/.cpp         ← Waveform + envelope overlay
├── build/                           ← CMake build output (Linux)
└── cmake-build-debug/               ← CLion build output

```

---

## Quick Build

### Linux/macOS
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Windows (Visual Studio)
```cmd
mkdir build & cd build
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release
```

### CLion (IDE)
- Open project folder
- CMake will auto-configure
- Build → Build (Ctrl+F9)
- Run → Run (Shift+F10)

---

## Key Files to Know

### 1. PluginProcessor (Audio Engine)
**File**: `source/PluginProcessor.h` / `.cpp`

**What it does**:
- Manages 16 drum pads with independent MIDI mapping
- Handles sample loading and synthesis
- Manages ADSR, filters, and parameters
- Routes audio to multi-output buses

**Key methods**:
- `processBlock()` - Main audio callback
- `loadFile()` - Load sample into pad
- `updateADSR()` - Apply parameters
- `createParameters()` - Define APVTS tree

**Important members**:
```cpp
juce::Synthesiser mSampler;                    // Voice pool
std::array<DrumPad, NUM_PADS> pads;            // Pad state
juce::AudioProcessorValueTreeState mAPVSTATE;  // Parameters
```

---

### 2. PluginEditor (GUI)
**File**: `source/PluginEditor.h` / `.cpp`

**What it does**:
- Creates and lays out UI components
- Handles user interactions (clicks, drags)
- Updates parameters when user changes controls

**Key components**:
```cpp
std::vector<DragAndDropButton> padButtons;     // 16 pad buttons
controlSlidersBlock CBlock;                    // Parameter sliders
waveFormEditor waveComponent;                  // Sample display
ADSROverlay adsrOverlay;                       // Interactive envelope
```

---

### 3. CustomSamplerVoice (DSP)
**File**: `source/CustomSamplerVoice.h`

**What it does**:
- Per-voice audio synthesis
- Real-time filtering (lowpass/highpass)
- ADSR envelope
- Sample interpolation for pitch shifting

**Processing chain**:
```
MIDI Note → startNote() → renderNextBlock()
  → Apply filters
  → Apply ADSR
  → Interpolate samples
  → Output audio
```

---

## Common Tasks

### Add a New Parameter

1. **Add to createParameters()** in PluginProcessor.cpp:
```cpp
parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID("MYPARAM" + suffix, 1),
    "My Parameter",
    0.0f, 100.0f,      // min, max
    50.0f,             // default
    "%"                // unit
));
```

2. **Get value in processBlock()**:
```cpp
if (auto* p = mAPVSTATE.getRawParameterValue("MYPARAM" + suffix))
    float value = p->load();
```

3. **Create UI slider** in PluginEditor.cpp:
```cpp
sliderController mySlider{ "My Param" };
SliderAttachment attachment{
    audioProcessor.getAPVTS(),
    "MYPARAM",
    mySlider
};
```

---

### Load & Play a Sample

**Already implemented**, but here's how it works:

1. User drops file on pad
2. `DragAndDropButton::filesDropped()` called
3. Calls `Processor.loadFile(path, midiNote)`
4. `loadFile()`:
   - Reads file with AudioFormatManager
   - Creates CustomSamplerSound
   - Adds to Synthesiser
   - Updates ADSR

5. On MIDI note-on:
   - Synthesiser routes to CustomSamplerVoice
   - Voice plays sample with current parameters

---

### Adjust a Parameter in Real-Time

1. User moves slider on GUI
2. SliderAttachment updates APVTS
3. valueTreePropertyChanged() called
4. `updateADSR()` fetches all params
5. Next audio block uses new values

**No audio thread blocking!** All safe.

---

## Architecture at 30,000 Feet

```
User Interface                Audio Engine
├─ Pad Buttons                ├─ Synthesiser (16 voices)
├─ Parameter Sliders          ├─ CustomSamplerSound (per sample)
├─ Waveform Display           ├─ CustomSamplerVoice (per voice)
├─ Envelope Editor            └─ IIR Filters
└─ Drag-and-Drop              
        ↓                             ↓
    APVTS (AudioProcessorValueTreeState)
    ↓
Parameter Values (thread-safe atomic access)
```

---

## Threading Model

### Audio Thread (Real-Time, ~500µs/block)
```cpp
void processBlock(AudioBuffer& buffer, MidiBuffer& midi) {
    // ✓ Safe here:
    param = mAPVSTATE.getRawParameterValue("GAIN")->load();
    mSampler.renderNextBlock(buffer, midi, 0, numSamples);
    
    // ✗ NEVER do here:
    File file = File::chooseFileToOpen();     // UI dialog!
    mFormatReader.reset(new AudioFormatReader()); // Allocation!
    juce::Logger::writeToLog("Processing");   // I/O!
}
```

### UI Thread
```cpp
void buttonClicked() {
    // ✓ Safe here:
    auto file = File::chooseFileToOpen();
    processor.loadFile(file.getFullPathName(), noteNumber);
    
    // ✗ Never do here:
    processor.mSampler.renderNextBlock(...);  // Audio processing!
}
```

---

## Debugging Tips

### See What's Happening
```cpp
// In processBlock or anywhere
DBG("Pad index: " << padIndex);
DBG("Sample rate: " << mSamplerate);
DBG("Active voices: " << mSampler.getNumVoices());
```

### Monitor Parameters
```cpp
auto tree = mAPVSTATE.state;
// tree.state.dump(std::cout);  // Uncomment to print full tree
```

### Check Audio Levels
```cpp
// In renderNextBlock after processing
if (sample > 1.0f || sample < -1.0f) {
    DBG("WARNING: Clipping detected!");
}
```

### Profile CPU
```cpp
auto t0 = juce::Time::getHighResolutionTicks();
// ... do work ...
auto elapsed = juce::Time::highResolutionTicksToSeconds(
    juce::Time::getHighResolutionTicks() - t0
);
DBG("Took " << elapsed * 1000 << " ms");
```

---

## Compilation Troubleshooting

### "Command not found: cmake"
```bash
# Install CMake
brew install cmake              # macOS
sudo apt install cmake          # Linux
choco install cmake             # Windows (with Chocolatey)
```

### "Cannot find JUCE"
```bash
# Make sure JUCE submodule is initialized
git submodule update --init --recursive
```

### Compile errors with unique_ptr
```cpp
// Make sure you have:
#include <memory>
// And using C++17 or later
```

### "undefined reference to" linker errors
- Ensure all .cpp files are listed in CMakeLists.txt
- Check that JUCE modules are linked (juce_audio_utils, etc.)
- Rebuild with: `cmake --build . --clean-first`

---

## Performance Checklist

- [ ] No allocations in audio thread
- [ ] No I/O in audio thread
- [ ] No locks/mutexes in audio thread
- [ ] Filter coefficients updated at block start, not per-sample
- [ ] ADSR calculations are simple math (no branches if possible)
- [ ] Buffers pre-allocated in prepareToPlay()
- [ ] No printf/cout in audio thread

---

## Testing Your Changes

### Basic Functionality
```cpp
// 1. Load a sample
processor.loadFile("path/to/sample.wav", 36);

// 2. Check it loaded
assert(processor.hasSampleLoaded(0));

// 3. Trigger playback
processor.playFile(36);

// 4. Check voice is active
// ... render a block ...
assert(mSampler.getVoice(0)->isVoiceActive());
```

### Parameter Updates
```cpp
// 1. Change a parameter
auto param = mAPVSTATE.getParameter("ATTACK");
param->setValueNotifyingHost(0.5f);

// 2. Verify it updated
auto readback = mAPVSTATE.getRawParameterValue("ATTACK")->load();
assert(readback == 0.5f);
```

---

## Resources

### Official JUCE
- Docs: https://docs.juce.com
- Forum: https://forum.juce.com
- API: https://docs.juce.com/master/

### Audio DSP
- DSP Theory: https://www.dsprelated.com/
- Filter Design: https://www.analog.com/media/en/training-seminars/design-handbooks/Complete-MixedSignal-Communications-IC-Design-Handbook-Chapter-8.pdf
- ADSR: https://en.wikipedia.org/wiki/Envelope_(music)

### C++ 
- Modern C++: https://www.cplusplus.com/
- Best Practices: https://github.com/isocpp/CppCoreGuidelines

---

## Documentation Map

| Question | Document |
|----------|----------|
| "What does this do?" | README.md |
| "How does this work?" | CODEBASE_ARCHITECTURE.md |
| "What style do I follow?" | DEVELOPMENT_GUIDELINES.md |
| "What was changed?" | IMPLEMENTATION_SUMMARY.md |
| "How do I get started?" | This file (QUICK_START.md) |

---

## One-Minute Overview

**Juce Drum Sampler** is a JUCE plugin that:
1. Loads 16 audio samples (one per pad)
2. Maps each to a MIDI note (36-51)
3. Provides per-pad control:
   - ADSR envelope (Attack, Decay, Sustain, Release)
   - Pitch/Detune
   - Lowpass/Highpass filters
   - Sample trim (start/end offset)
   - Velocity modulation
4. Routes audio to individual output buses
5. Supports VST3, AU, and Standalone formats

**Main Components**:
- **PluginProcessor**: Audio engine + parameters
- **PluginEditor**: GUI + user interaction
- **CustomSamplerVoice**: DSP synthesis
- **UI Components**: Sliders, buttons, waveform display

**Key Skills Needed**:
- C++17 (modern features, unique_ptr)
- JUCE framework basics
- Audio DSP fundamentals
- Thread safety (audio vs. UI thread)

---

## Next Actions

1. **Understand the codebase**
   - Read CODEBASE_ARCHITECTURE.md
   - Trace code flow in main files

2. **Try a small change**
   - Modify a parameter default value
   - Add debug logging
   - Rebuild and test

3. **Add a feature**
   - Reference DEVELOPMENT_GUIDELINES.md
   - Follow existing patterns
   - Test thoroughly

4. **Ship it**
   - Run full test suite
   - Check memory for leaks
   - Monitor CPU load
   - Document your changes

---

**Happy coding! 🎵**

Need help? Check the documentation files or review existing code patterns.
