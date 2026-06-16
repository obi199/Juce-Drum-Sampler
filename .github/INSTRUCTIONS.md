# Juce Drum Sampler — Developer Instructions

## Project Overview
A 16-pad drum sampler VST3/AU/Standalone plugin built with JUCE (C++17). Each pad maps to a MIDI note (36–51) and has independent sample playback with per-pad DSP.

## Build
- **IDE**: Visual Studio 2022 (x64), or CMake 3.22+
- **CMake**: `mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && cmake --build .`
- **VS Solution**: `Builds/VisualStudio2022/`
- **C++ Standard**: C++17

## Source Files (`source/`)

| File | Responsibility |
|---|---|
| `PluginProcessor.h/.cpp` | Audio engine, APVTS parameters, sample loading, kit save/load |
| `PluginEditor.h/.cpp` | GUI layout, pad buttons, Save/Load Kit buttons |
| `CustomSamplerVoice.h` | `CustomSamplerSound` (data) + `CustomSamplerVoice` (DSP rendering) |
| `DragAndDropButton.h/.cpp` | Pad button with drag-and-drop, right-click menu |
| `sliderController.h/.cpp` | Custom rotary knob + `controlSlidersBlock` (9 knobs, 2 rows) |
| `waveFormEditor.h/.cpp` | Waveform display, draggable start/end/fade handles, ADSR overlay |

## Architecture

### Audio Processing Chain (per voice, per sample)
1. Linear interpolation (pitch/detune via `pitchRatio`)
2. Lowpass filter (IIR, velocity-modulated)
3. Highpass filter (IIR)
4. EQ Low shelf — 100 Hz, ±12 dB
5. EQ Mid peak — 1 kHz, ±12 dB
6. EQ High shelf — 8 kHz, ±12 dB
7. ADSR envelope × gain

### Per-Pad Parameters (18 per pad × 16 pads = 288 total)
Suffix convention: pad 1 = no suffix, pads 2–16 = suffix `"2"`–`"16"`.

`GAIN`, `ATTACK`, `DECAY`, `SUSTAIN`, `RELEASE`, `START_OFFSET`, `END_OFFSET`, `FADE_START`, `FADE_END`, `DETUNE`, `EQ_LOW`, `EQ_MID`, `EQ_HIGH`, `LOWPASS`, `HIGHPASS`, `VEL_TO_LOWPASS`, `VEL_TO_ATTACK`

### Multi-Output
17 stereo output buses: Bus 0 = main mix (all pads summed), Buses 1–16 = individual pads. Rendered into a 32-channel `multiOutBuffer` then distributed.

### Drum Kit Persistence
`.drumkit` files are XML: `<DrumSet version="1"><Samples><Pad index="0" path="..."/></Samples><APVTS state.../>`. `getStateInformation`/`setStateInformation` use the same format so DAW projects save sample paths too. Backwards-compatible with old APVTS-only presets.

## Key Patterns

### Adding a new per-pad parameter
1. `createParameters()` in `PluginProcessor.cpp` — add with suffix loop
2. `updateADSR()` — read and apply to `CustomSamplerSound`
3. `resetPadParametersToDefault()` — add default value
4. `CustomSamplerSound` — add field + getter/setter
5. `CustomSamplerVoice::startNote()` / `renderNextBlock()` — use it in DSP
6. `sliderController.h/.cpp` — add knob + attachment + label
7. `PluginEditor.cpp` `switchTopad()` — add `changeSliderParameter` call

### Switching pads
`switchTopad(padIndex)` in `PluginEditor.cpp` rebinds all `SliderAttachment`s to the new pad's parameter IDs and updates the waveform thumbnail.

### Parameter updates → audio
`valueTreePropertyChanged()` sets `mShouldUpdate` → `processBlock()` calls `updateADSR()` which pushes all values to `CustomSamplerSound`. Voices pick them up on the next `renderNextBlock()`.

## Important Notes
- `loadFile()` removes any existing sound for that MIDI note before adding the new one — avoids stale parameter application
- `DBG()` logs appear in the IDE output window
- IIR filter coefficients are recomputed every block so knob changes are immediate
- Pad 0 suffix is `""`, not `"1"` — be consistent with this throughout
