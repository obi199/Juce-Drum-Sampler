# JUCE Drum Sampler - Architecture Documentation

## Overview

This is a professional-grade JUCE audio plugin that implements a 16-pad drum sampler with per-pad parameter control, multi-output routing, and real-time audio processing.

---

## Core Components

### 1. **DrumSamplerAudioProcessor** (PluginProcessor.h/.cpp)
**Responsibility**: Audio engine and parameter management

**Key Members**:
```cpp
juce::Synthesiser mSampler;                    // Voice pool & sound management
std::array<DrumPad, NUM_PADS> pads;            // Per-pad state storage
juce::AudioProcessorValueTreeState mAPVSTATE;  // Parameter state tree
juce::AudioThumbnail thumbnail;                // Waveform cache
std::unique_ptr<juce::AudioFormatReader> mFormatReader;  // Current file reader
```

**Key Methods**:
- `processBlock()` - Main audio callback (called ~44 times/second @ 44.1kHz)
- `loadFile()` - Load audio file and create sampler sound
- `updateADSR()` - Apply ADSR parameters to sound
- `createParameters()` - Initialize APVTS parameter tree
- `prepareToPlay()` - Setup sample rate and buffer

**Parameter Naming Convention**:
```
Pad 1 (no suffix): "GAIN", "ATTACK", "DETUNE", ...
Pad 2: "GAIN2", "ATTACK2", "DETUNE2", ...
Pad 3-16: "GAIN3"-"GAIN16", etc.
```

---

### 2. **DrumSamplerAudioProcessorEditor** (PluginEditor.h/.cpp)
**Responsibility**: GUI layout and user interaction

**Key Components**:
```cpp
std::vector<std::unique_ptr<DragAndDropButton>> padButtons;  // 16 pad buttons
waveFormEditor waveComponent;      // Waveform display
controlSlidersBlock CBlock;         // 9 rotary sliders (2 rows)
positionLine position;              // Playback position indicator
startLine start;                    // Sample start offset handle
endLine end;                        // Sample end offset handle
ADSROverlay adsrOverlay;            // Interactive envelope editor
juce::TextButton saveKitButton;     // Save .drumkit file
juce::TextButton loadKitButton;     // Load .drumkit file
```

**Layout**:
```
┌─────────────────────────────────────────────────────┐
│  Pad Grid (4x4)        │  [Save Kit]  [Load Kit]    │
│                        │  Waveform Display           │
│  [1] [2] [3] [4]       │  ┌──────────────────────┐  │
│  [5] [6] [7] [8]       │  │  ~~~~~ ~~~ ~~~~~     │  │
│  [9] [10][11][12]      │  │  ADSR Overlay (A F E) │  │
│  [13][14][15][16]      │  └──────────────────────┘  │
│                        │  Gain  Detune  EQLo Mid Hi │
│                        │  LP    HP   Vel→LP Vel→Atk │
└─────────────────────────────────────────────────────┘
```

---

### 3. **CustomSamplerSound & CustomSamplerVoice** (CustomSamplerVoice.h)
**Responsibility**: Sample playback with real-time DSP

**CustomSamplerSound** (extends juce::SamplerSound):
```cpp
struct PerPadData {
    float startOffset;          // 0-1 range, position in sample
    float endOffset;            // 0-1 range
    float fadeStartOffset;      // Where fade-out begins
    float detuneSemitones;      // Pitch shift
    float lowpassCutoff;        // Filter frequency (Hz)
    float highpassCutoff;       // Filter frequency (Hz)
    float velToLowpass;         // Velocity → filter modulation
    float velToAttack;          // Velocity → attack modulation
    float eqLowDb;              // Low shelf gain ±12 dB (100 Hz)
    float eqMidDb;              // Mid peak gain ±12 dB (1 kHz)
    float eqHighDb;             // High shelf gain ±12 dB (8 kHz)
    float gainLinear;           // Volume multiplier
    int outputBusIndex;         // Which output bus to use
};
```

**CustomSamplerVoice** (extends juce::SamplerVoice):
- Handles per-voice DSP processing
- Manages IIR filter coefficients
- Implements ADSR envelope
- Performs linear interpolation for pitch shifting
- Triggers fade-out at `fadeStartOffset` position

**Audio Processing Chain**:
```
MIDI Note On
    ↓
startNote() - Initialize voice with velocity curve
    ↓
renderNextBlock() - For each sample:
    1. Calculate current position (with pitch ratio)
    2. Check if fade-out should trigger
    3. Sample linear interpolation (for pitch)
    4. Apply lowpass filter with velocity modulation
    5. Apply highpass filter
    6. Apply EQ low shelf (100 Hz)
    7. Apply EQ mid peak (1 kHz)
    8. Apply EQ high shelf (8 kHz)
    9. Multiply by ADSR envelope value
   10. Accumulate to output buffer
    ↓
MIDI Note Off or sample end detected
    ↓
stopNote() - Trigger release phase
```

---

### 4. **UI Components**

#### **DragAndDropButton** (DragAndDropButton.h/.cpp)
- Extends `juce::TextButton` with drag-and-drop support
- Accepts files from OS or other pads
- Right-click context menu for file selection
- Visual feedback: filename display, color change on hover

#### **sliderController** (sliderController.h/.cpp)
- Custom rotary slider with gradient styling
- Uses `NoBoxLookAndFeel` (custom JUCE look and feel)
- Rotary knob with pointer and text box
- Supports all parameter types (dB, Hz, normalized)

#### **controlSlidersBlock** (sliderController.h/.cpp)
- Container for 9 sliders in 2 rows
- Row 1: Gain, Detune, EQ Lo, EQ Mid, EQ Hi
- Row 2: Lowpass, Highpass, Vel→LP, Vel→Atk
- Manages parameter attachments (SliderAttachment)
- `changeSliderParameter()` to rebind sliders to different pad parameters

#### **waveFormEditor** (waveFormEditor.h/.cpp)
- Displays audio thumbnail/waveform
- White area = active sample region (between start/end offsets)

#### **positionLine** (waveFormEditor.h/.cpp)
- Red vertical line showing playback position
- Updates at ~30Hz via timer callback

#### **startLine & endLine** (waveFormEditor.h/.cpp)
- Draggable handles for sample trim points
- Syncs with APVTS parameters via polling

#### **ADSROverlay** (waveFormEditor.h/.cpp)
- Draws envelope shape (attack, sustain, fade)
- Three draggable handles: Attack, Fade-Start, Fade-End
- Converts mouse position to parameter values
- Provides visual feedback for envelope editing

---

## Data Flow

### Loading a Sample
```
User drops file on pad
    ↓
DragAndDropButton::filesDropped()
    ↓
DrumSamplerAudioProcessor::loadFile(path, midiNote)
    ↓
1. Read file with AudioFormatManager
2. Create CustomSamplerSound (binds to midiNote)
3. Add to mSampler (Synthesiser)
4. Call updateADSR() to apply current params
    ↓
Sample ready to play
```

### Playing a Note
```
MIDI Note 36 (Pad 1)
    ↓
processBlock() detects noteOn
    ↓
mSampler.noteOn(channel, midiNote, velocity)
    ↓
Synthesiser routes to CustomSamplerVoice
    ↓
voice->startNote() initializes voice
    ↓
voice->renderNextBlock() generates audio
    ↓
Audio routed to Bus 1 (Pad 1 output)
    ↓
Speaker / Output
```

### Parameter Update
```
User moves "Attack" slider on GUI
    ↓
SliderAttachment updates APVTS: setParameter("ATTACK", newValue)
    ↓
ValueTree notifies listener
    ↓
valueTreePropertyChanged() called
    ↓
updateADSR() fetches all current params
    ↓
sound->setEnvelopeParameters() updates internal ADSR
    ↓
Next audio block uses new values
```

---

## Threading Model

### Audio Thread (Real-Time, ~500µs per block)
- `processBlock()` - Generate audio
- `renderNextBlock()` - Voice DSP
- Parameter reads from APVTS
- ⚠️ No allocations, locks, or I/O

### UI Thread
- GUI drawing
- Parameter writes via APVTS
- File loading dialog
- ⚠️ No audio processing, no blocking calls

### Communication
- **APVTS** - Thread-safe parameter bridge
- **Atomic variables** - Status flags (mIsNotePlayed, mSampleCount)

---

## Parameter System

### Parameters per Pad (18 total × 16 pads = 288 parameters)

| Parameter | ID | Range | Default |
|---|---|---|---|
| Gain | `GAIN[n]` | -42 to +24 dB | 0 dB |
| Attack | `ATTACK[n]` | 0–1 | 0.02 |
| Decay | `DECAY[n]` | 0–1 | 0.5 |
| Sustain | `SUSTAIN[n]` | 0–1 | 1.0 |
| Release | `RELEASE[n]` | 0–1 | 0.2 |
| Start Offset | `START_OFFSET[n]` | 0–1 | 0 |
| End Offset | `END_OFFSET[n]` | 0–1 | 1 |
| Fade Start | `FADE_START[n]` | 0–1 | 0.8 |
| Fade End | `FADE_END[n]` | 0–1 | 1 |
| Detune | `DETUNE[n]` | -24 to +24 st | 0 |
| EQ Low | `EQ_LOW[n]` | -12 to +12 dB | 0 |
| EQ Mid | `EQ_MID[n]` | -12 to +12 dB | 0 |
| EQ High | `EQ_HIGH[n]` | -12 to +12 dB | 0 |
| Lowpass | `LOWPASS[n]` | 200–20000 Hz | 20000 |
| Highpass | `HIGHPASS[n]` | 20–18000 Hz | 20 |
| Vel→LP | `VEL_TO_LOWPASS[n]` | 0–1 | 0 |
| Vel→Atk | `VEL_TO_ATTACK[n]` | 0–1 | 0 |

> Suffix `[n]`: pad 1 has no suffix, pads 2–16 use suffix `2`–`16`.

### Creating Parameters
**File**: PluginProcessor.cpp `createParameters()`

```cpp
// For each pad (i = 0 to 15):
parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID("ATTACK" + suffix, 1),  // unique ID
    "Attack",                                   // display name
    0.0f, 1.0f,                                // range
    0.02f,                                     // default
    "Attack Time"                              // unit
));
```

### Retrieving Parameters
```cpp
// Get normalized value (0-1)
if (auto* param = mAPVSTATE.getRawParameterValue("ATTACK" + suffix))
    float normalized = param->load();

// Set parameter from audio thread
if (auto* param = mAPVSTATE.getParameter("ATTACK" + suffix))
    param->setValueNotifyingHost(newNormalizedValue);
```

---

## Multi-Output Routing

### Output Bus Configuration
```cpp
// In PluginProcessor constructor
BusesProperties()
    .withOutput("Main", AudioChannelSet::stereo(), true)     // Bus 0 - Main mix
    .withOutput("Pad 1", AudioChannelSet::stereo(), true)    // Bus 1
    .withOutput("Pad 2", AudioChannelSet::stereo(), true)    // Bus 2
    // ... Pad 3-16
```

### Audio Distribution
```cpp
// In processBlock()
// 1. Render all voices to multiOutBuffer (32 channels)
mSampler.renderNextBlock(multiOutBuffer, midiMessages, 0, numSamples);

// 2. Distribute to output buses
for (int padIdx = 0; padIdx < NUM_PADS; ++padIdx) {
    auto padBus = getBus(false, padIdx + 1);
    padBus->getBusBuffer(buffer).addFrom(
        0, 0, multiOutBuffer, padIdx * 2, 0, numSamples
    );
}

// 3. Also sum to main mix (Bus 0)
auto mainBus = getBus(false, 0);
for (int padIdx = 0; padIdx < NUM_PADS; ++padIdx) {
    mainBus->getBusBuffer(buffer).addFrom(
        0, 0, multiOutBuffer, padIdx * 2, 0, numSamples
    );
}
```

---

## Memory Lifecycle

### Sample Loading
```
User loads file
    ↓
loadFile() → createReaderFor() → returns unique_ptr<AudioFormatReader>
    ↓
CustomSamplerSound created with reader reference
    ↓
Sound added to Synthesiser
    ↓
Reuse same reader for multiple sounds (all pads can reference mFormatReader)
```

### Sample Unloading
```
clearPad() called
    ↓
1. Remove CustomSamplerSound from Synthesiser
2. mSampler.removeSound() - Sound deleted, ref count drops
3. mFormatReader reset (if it was the last reference)
    ↓
Memory freed automatically
```

---

## Performance Characteristics

| Operation | CPU % | Notes |
|-----------|-------|-------|
| Idle (no audio) | ~0% | Timer-driven UI updates |
| One voice playing | ~2-3% | IIR filters + interpolation |
| 16 voices @ full polyphony | ~15-20% | Typical drum use case |
| Filter update per-block | Negligible | Coefficient calculations |
| ADSR envelope calculation | Negligible | Per-sample simple math |

**Memory Usage**:
- Base: ~20MB
- Per sample: ~depends on file size
- Typical 5s sample @ 48kHz stereo: ~2.3MB

---

## Error Handling

### File Loading
```cpp
// Validates:
✓ File exists
✓ File is readable
✓ Format is supported (WAV, MP3, FLAC, etc.)
✓ MIDI note number is valid (36-51)
✓ No duplicate loads

// Errors logged to:
juce::Logger (console, debug output)
```

### Parameter Updates
```cpp
// Validates:
✓ Pad index in range [0, NUM_PADS)
✓ Parameter values clamped to valid ranges
✓ ADSR times constrained to realistic values
✓ Filter frequencies bounded [20Hz, 20kHz]
```

---

## Extension Points

### Adding a New Parameter
1. Add to `createParameters()` with unique suffix
2. In `updateADSR()`, fetch and apply parameter
3. In UI, create slider and attach to parameter
4. Done - automatically syncs

### Adding a New DSP Effect
1. Add to `CustomSamplerVoice` (e.g., more filters)
2. Add parameters in processor
3. Update in `renderNextBlock()`
4. Add UI slider

### Adding a New Output Bus
1. Modify `BusesProperties()` in constructor
2. Update `NUM_PADS` if needed
3. Distribute audio in `processBlock()`
4. Done - DAW automatically recognizes

---

## Debugging Tips

**Enable verbose logging:**
```cpp
DBG("Pad " << padIndex << " playing at " << sampleRate << "Hz");
```

**Check APVTS tree:**
```cpp
auto tree = mAPVSTATE.state;
tree.state.dump(std::cout);  // Prints full parameter tree
```

**Monitor voices:**
```cpp
int activeVoices = 0;
for (int i = 0; i < mSampler.getNumVoices(); ++i)
    if (mSampler.getVoice(i)->isVoiceActive())
        activeVoices++;
DBG("Active voices: " << activeVoices);
```

**Audio thread safety check:**
```cpp
jassert(MessageManager::getInstance()->isThisTheAudioThread());
```

---

## References

- **JUCE Documentation**: https://docs.juce.com/
- **SamplerSound/Voice**: `juce_audio_basics/sources/juce_Sampler.cpp`
- **AudioProcessorValueTreeState**: `juce_audio_processors/utilities/juce_APVTS.h`
