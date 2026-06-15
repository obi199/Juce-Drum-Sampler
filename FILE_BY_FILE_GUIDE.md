# File-by-File Guide

## Core Audio Files

### PluginProcessor.h (146 lines)
**Purpose**: Declares the main audio processor class

**Key Declarations**:
- `DrumPad` struct - State for each of 16 pads
- `DrumSamplerAudioProcessor` class - Main audio engine
- Configuration constants (NUM_PADS=16, MAX_VOICES=16, etc.)
- MIDI note mapping array (36-51)

**Public Interface** (user-facing):
```cpp
void loadFile(path, noteNumber, buttonName);              // Load sample
void playFile(midiNote);                                  // Trigger playback
void updateADSR(padIndex);                                // Apply ADSR params
void clearPad(midiNote);                                  // Remove sample
void resetPadParametersToDefault(padIndex);               // Reset to defaults
```

**Important Public Members**:
```cpp
juce::AudioThumbnail thumbnail;                           // Waveform cache
juce::AudioProcessorValueTreeState& getAPVTS();           // Parameter tree
int getCurrentPadIndex() const;                           // Which pad active
```

---

### PluginProcessor.cpp (849 lines)
**Purpose**: Implements audio processing and parameter management

**Key Functions** (in order of execution):

1. **Constructor** (lines ~13-75)
   - Sets up AudioProcessor configuration
   - Creates 16 output buses (1 main + 16 pads)
   - Initializes parameter state tree
   - Adds synthesizer voices

2. **prepareToPlay()** (lines ~131-136)
   - Called before audio starts
   - Set sample rate
   - Allocate buffers
   - Initialize ADSR

3. **processBlock()** (lines ~157-240)
   - Called every ~20ms with audio data
   - Detects MIDI notes
   - Routes audio to buses
   - Updates playback position

4. **loadFile()** (lines ~369-451)
   - Load audio file into pad
   - Create sampler sound
   - Update ADSR
   - Error handling included

5. **updateADSR()** (lines ~575-660)
   - Read parameters from APVTS
   - Apply to sampler sounds
   - Update filters in real-time

6. **createParameters()** (lines ~667-710)
   - Define 240 parameters (15 per pad × 16 pads)
   - Parameter naming with suffixes
   - Default values

---

### PluginEditor.h (60 lines)
**Purpose**: Declares the GUI editor

**Key Components**:
```cpp
std::vector<DragAndDropButton> padButtons;        // 16 pad buttons
controlSlidersBlock CBlock;                       // 6 sliders
waveFormEditor waveComponent;                     // Waveform display
ADSROverlay adsrOverlay;                          // Envelope editor
startLine start, endLine end;                     // Trim handles
```

**Key Methods**:
```cpp
void resized();                         // Layout components
void paint(Graphics&);                  // Draw background
void switchTopad(padIndex);             // Update for different pad
void ButtonClicked(Button*, noteNumber);// Pad button pressed
void timerCallback();                   // Poll for MIDI changes
```

---

### PluginEditor.cpp (181 lines)
**Purpose**: Implements GUI and user interaction

**Key Sections**:

1. **Constructor** (lines ~13-52)
   - Create 16 pad buttons dynamically
   - Add sliders and waveform display
   - Set window size
   - Start timer for polling

2. **resized()** (lines ~68-100)
   - Grid layout for 16 pads (4×4)
   - Right side: waveform + controls
   - MPC-style grid (pad 0 at bottom-left)

3. **switchTopad()** (lines ~110-135)
   - Update sliders to show current pad params
   - Load waveform for selected pad
   - Update ADSR for selected pad

4. **paint()** (lines ~55-57)
   - Draw black background
   - Components handle their own drawing

---

## DSP Files

### CustomSamplerVoice.h (262 lines)
**Purpose**: Audio synthesis for each voice

**Classes**:

1. **CustomSamplerSound** (lines ~8-70)
   - Extends `juce::SamplerSound`
   - Stores per-sound parameters:
     - Start/end offset (0-1 range)
     - Fade envelope positions
     - Filter cutoff frequencies
     - Pitch shift (semitones)
     - Velocity modulation amounts

2. **CustomSamplerVoice** (lines ~76-262)
   - Extends `juce::SamplerVoice`
   - Main DSP processing

**Key Methods**:
```cpp
void startNote();           // Initialize voice on MIDI note-on
void stopNote();            // Trigger release on MIDI note-off
void renderNextBlock();     // Generate audio per-sample
```

**DSP Chain**:
1. Calculate sample position with pitch ratio
2. Check for fade-out trigger
3. Perform linear interpolation (sample rate conversion)
4. Apply lowpass filter (with velocity modulation)
5. Apply highpass filter
6. Multiply by ADSR envelope
7. Apply gain
8. Write to output buffer

---

## UI Component Files

### DragAndDropButton.h / .cpp (58 + 213 lines)
**Purpose**: Drag-and-drop pad button

**Features**:
- Accept file drops from OS
- Accept drops from other pads (move sample)
- Visual feedback (color change, filename display)
- Right-click menu (Load / Clear)

**Key Methods**:
```cpp
void filesDropped();           // OS file dropped
void itemDropped();            // Pad-to-pad drag
void paint();                  // Draw button
void mouseUp();                // Right-click menu
```

---

### sliderController.h / .cpp (159 + 123 lines)
**Purpose**: Custom rotary slider with styling

**Features**:
- Gradient knob appearance
- Custom look-and-feel (NoBoxLookAndFeel)
- Rotary style with pointer
- Text display below knob

**Classes**:
1. **NoBoxLookAndFeel** - Custom appearance
2. **sliderController** - Rotary slider
3. **controlSlidersBlock** - Container for 6 sliders

---

### waveFormEditor.h / .cpp (144 + 548 lines)
**Purpose**: Waveform display and envelope overlay

**Classes**:

1. **waveFormEditor** (lines ~17-35)
   - Displays audio thumbnail
   - White area = active region (start to end offset)
   - Uses juce::AudioThumbnail for efficient rendering

2. **positionLine** (lines ~38-57)
   - Red vertical line showing playback position
   - Updates at ~30Hz via timer

3. **startLine** (lines ~60-83)
   - Draggable handle for sample start offset
   - Green vertical line
   - Syncs with APVTS parameters

4. **endLine** (lines ~86-99)
   - Same as startLine but for end offset

5. **ADSROverlay** (lines ~104-144)
   - Interactive envelope visualization
   - Three draggable handles (Attack, Fade-Start, Fade-End)
   - Draws colored envelope shape
   - Updates parameters on drag

---

## Build Files

### CMakeLists.txt (119 lines)
**Purpose**: CMake build configuration

**Key Sections**:
1. **Project setup** (lines ~15-20)
   - Project name and version
   - Minimum CMake version

2. **JUCE inclusion** (lines ~28-31)
   - Add JUCE as subdirectory

3. **Plugin definition** (lines ~36-50)
   - Plugin name, type (synth)
   - Format (VST3, AU, Standalone)
   - MIDI configuration

4. **Source files** (lines ~68-73)
   - List all .cpp files to compile
   - `target_sources()` command

5. **Linking** (lines ~97-104)
   - Link against JUCE modules
   - Link against system libraries

---

## Documentation Files (Created)

### README.md (185 lines)
- Feature overview
- Build instructions
- Usage guide
- Troubleshooting

### CODEBASE_ARCHITECTURE.md (465 lines)
- Component descriptions
- Data flow diagrams
- Threading model
- Parameter system
- Multi-output routing
- Performance notes
- Debugging tips

### DEVELOPMENT_GUIDELINES.md (380 lines)
- Code style conventions
- Memory management patterns
- Thread safety rules
- Error handling examples
- Testing checklist
- Common pitfalls
- Code review guidelines

### QUICK_START.md (350+ lines)
- Project overview
- Quick build instructions
- Key files summary
- Common tasks
- Debugging tips
- Resource links

### IMPLEMENTATION_SUMMARY.md (200+ lines)
- What was fixed
- Files modified
- Testing recommendations
- Next steps

---

## Build Output

### Generated After Build

```
build/
├── Juce_Drum_Sampler_artefacts/
│   ├── Standalone/
│   │   └── Juce_Drum_Sampler              # Executable
│   ├── VST3/
│   │   └── Juce_Drum_Sampler.vst3/        # VST3 plugin
│   └── AU/
│       └── Juce_Drum_Sampler.component/   # AU plugin
├── CMakeFiles/
├── JuceLibraryCode/                       # Auto-generated
└── Makefile (Linux/macOS) or .sln (Windows)
```

---

## File Dependency Graph

```
CMakeLists.txt
    ↓
[Compile]
    ↓
PluginProcessor.h
    ├── CustomSamplerVoice.h
    └── DrumPad structure
    
PluginEditor.h
    ├── PluginProcessor.h
    ├── waveFormEditor.h
    ├── DragAndDropButton.h
    └── sliderController.h
    
PluginProcessor.cpp
    ├── loadFile() → CustomSamplerSound
    ├── processBlock() → Synthesiser
    └── updateADSR()
    
PluginEditor.cpp
    ├── Creates DragAndDropButton (16×)
    ├── Creates sliderController (6×)
    └── Creates waveFormEditor, etc.
    
waveFormEditor.cpp
    ├── waveFormEditor class
    ├── positionLine class
    ├── startLine class
    ├── endLine class
    └── ADSROverlay class
```

---

## Important Constants

### In PluginProcessor.h
```cpp
static constexpr int MAX_VOICES = 16;                    // Polyphony
static constexpr int NUM_PADS = 16;                      // Pad count
static constexpr int CHANNELS_PER_PAD = 2;               // Stereo per pad
static constexpr int MULTI_OUT_BUFFER_CHANNELS = 32;    // 16 × 2
static constexpr int MIDI_NOTES[16] = { 36, 37, ..., 51 }; // Drum mapping

static constexpr float DEFAULT_SAMPLE_LOAD_ATTACK = 0.01f;   // 10ms
static constexpr float DEFAULT_SAMPLE_LOAD_RELEASE = 0.1f;   // 100ms
static constexpr float DEFAULT_SAMPLE_MAX_LENGTH = 10.0f;    // 10 seconds
```

### In waveFormEditor.cpp
```cpp
static constexpr float kHandleRadius = 6.0f;           // Envelope handle size
static constexpr float kHitTolerance = 10.0f;          // Click tolerance
```

---

## File Size Summary

| File | Lines | Purpose |
|------|-------|---------|
| PluginProcessor.h | 146 | Declares audio processor |
| PluginProcessor.cpp | 849 | Implements audio engine |
| PluginEditor.h | 60 | Declares GUI |
| PluginEditor.cpp | 181 | Implements GUI |
| CustomSamplerVoice.h | 262 | Declares DSP |
| DragAndDropButton.h | 58 | UI component header |
| DragAndDropButton.cpp | 213 | UI component impl |
| sliderController.h | 159 | Slider header |
| sliderController.cpp | 123 | Slider impl |
| waveFormEditor.h | 144 | Waveform header |
| waveFormEditor.cpp | 548 | Waveform impl |
| CMakeLists.txt | 119 | Build config |
| **Total Source** | **2,862** | C++ code |
| **Total Docs** | **2,000+** | Documentation |
| **Grand Total** | **4,862+** | All files |

---

## How Files Work Together

### User loads a sample:
1. Clicks on pad button (PluginEditor)
2. DragAndDropButton handles drop
3. Calls PluginProcessor::loadFile()
4. Creates CustomSamplerSound
5. Adds to Synthesiser

### User plays a note:
1. MIDI arrives in processBlock()
2. Synthesiser::noteOn()
3. CustomSamplerVoice::startNote()
4. CustomSamplerVoice::renderNextBlock()
5. Audio routed to output bus

### User changes parameter:
1. UI slider moved (sliderController)
2. APVTS parameter changed
3. valueTreePropertyChanged()
4. updateADSR() called
5. CustomSamplerSound parameters updated
6. Next audio block uses new values

---

**All files work together to create a professional audio plugin!** 🎵

For detailed info, check CODEBASE_ARCHITECTURE.md
