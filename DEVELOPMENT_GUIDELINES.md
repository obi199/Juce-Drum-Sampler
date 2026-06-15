# Development Guidelines & Best Practices

## Code Style

### Naming Conventions
```cpp
// Classes and types: PascalCase
class DrumSamplerAudioProcessor { };
struct DrumPad { };

// Member variables: camelCase (prefix with 'm' for class members)
private:
    juce::Synthesiser mSampler;
    float mSamplerate;
    std::unique_ptr<juce::AudioFormatReader> mFormatReader;

// Local variables: camelCase
float noteVelocity = 0.8f;
auto padIndex = getPadIndexFromMidiNote(noteNumber);

// Constants: SCREAMING_SNAKE_CASE
static constexpr int NUM_PADS = 16;
static constexpr float MAX_ATTACK_SECONDS = 0.5f;

// Methods: camelCase
void updateADSR(int padIndex);
bool hasSampleLoaded(int padIndex);

// Boolean methods: startsWith 'is' or 'has'
bool isVoiceActive();
bool hasSampleLoaded(int padIndex);
```

### Documentation Style
```cpp
/**
 * Brief description of what this function does
 * 
 * Longer description explaining the purpose, behavior, and
 * any important side effects or preconditions.
 * 
 * @param paramName Description of parameter
 * @param anotherParam Another parameter
 * 
 * @return Description of return value
 * 
 * @note Important notes about implementation or usage
 * @warning Potential issues or gotchas
 * 
 * @see relatedFunction()
 * @see RelatedClass
 */
void importantFunction(int paramName, float anotherParam);
```

---

## Memory Management

### Smart Pointers Only
```cpp
// ✓ GOOD - Automatic cleanup
std::unique_ptr<juce::AudioFormatReader> mFormatReader;
auto sound = std::make_unique<CustomSamplerSound>(...);

// ✗ BAD - Manual cleanup required
juce::AudioFormatReader* mFormatReader;
auto sound = new CustomSamplerSound(...);  // Who deletes this?
```

### JUCE Object Ownership
```cpp
// ✓ GOOD - Let JUCE handle lifetime
mSampler.addSound(sound);           // Sampler takes ownership

// ✗ BAD - Double deletion risk
auto sound = std::make_unique<CustomSamplerSound>(...);
mSampler.addSound(sound.get());     // sound still owns it!
mSampler.addSound(sound.release()); // Proper way
```

---

## Thread Safety

### Audio Thread Rules
```cpp
// ✓ Safe in processBlock()
auto param = mAPVSTATE.getRawParameterValue("ATTACK");
float value = param->load();        // Atomic read
float output = filter.process(input);

// ✗ Unsafe in processBlock()
auto file = juce::File::chooseFileToOpen();  // UI dialog!
mFormatReader.reset(new AudioFormatReader()); // Allocation!
{
    juce::ScopedLock lock(mutex);             // Lock!
}
std::cout << "Debug" << std::endl;           // I/O!
```

### Parameter Updates
```cpp
// ✓ Correct - UI thread
if (auto* param = mAPVSTATE.getParameter("ATTACK"))
    param->setValueNotifyingHost(newValue);

// ✓ Correct - Audio thread (read-only)
if (auto* param = mAPVSTATE.getRawParameterValue("ATTACK"))
    float val = param->load();

// ✗ Wrong - Setting from audio thread
if (auto* param = mAPVSTATE.getParameter("ATTACK"))
    param->setValueNotifyingHost(newValue);  // Deadlock!
```

---

## Error Handling

### Logging Best Practices
```cpp
// ✓ GOOD - Informative message
if (!file.exists()) {
    juce::Logger::writeToLog("ERROR: Sample file not found: " + file.getFullPathName());
    return;
}

// ✓ GOOD - Log at load time, not audio time
// (Log I/O should never happen in processBlock!)

// ✗ BAD - Vague message
if (!file.exists()) {
    DBG("Error");
    return;
}

// ✗ BAD - Logging in audio thread
void renderNextBlock(...) {
    juce::Logger::writeToLog("Rendering block");  // WRONG! Audio thread!
}
```

### Bounds Checking
```cpp
// ✓ GOOD - Validate input
void updateADSR(int padIndex) {
    jassert(padIndex >= 0 && padIndex < NUM_PADS);  // Debug mode check
    if (padIndex < 0 || padIndex >= NUM_PADS) {     // Runtime check
        juce::Logger::writeToLog("Invalid pad index");
        return;
    }
}

// ✗ BAD - No validation
void updateADSR(int padIndex) {
    pads[padIndex].adsr.attack = value;  // Crash if out of bounds!
}
```

---

## Audio Processing

### Sample Rate Awareness
```cpp
// ✓ GOOD - Accounts for different sample rates
double hostRate = getSampleRate();
double srcRate = sound->getSourceSampleRate();
double pitchRatio = srcRate / hostRate;  // Adjust playback speed

// ✗ BAD - Assumes fixed 44.1kHz
float delayTime = 0.5f * 44100.0f;  // Breaks at 48kHz!
```

### Filter Management
```cpp
// ✓ GOOD - Update coefficients at block start
double hostRate = getSampleRate();
auto lpCoeffs = juce::IIRCoefficients::makeLowPass(hostRate, cutoffHz);
for (int ch = 0; ch < 2; ++ch)
    lowpassFilters[ch].setCoefficients(lpCoeffs);

// ✓ GOOD - Process per-sample
sample = lowpassFilters[channel].processSingleSampleRaw(sample);

// ✗ BAD - Updating coefficients per-sample (expensive!)
for (int i = 0; i < numSamples; ++i) {
    auto coeffs = juce::IIRCoefficients::makeLowPass(sr, cutoff);  // Ouch!
    sample = filter.processSingleSampleRaw(sample);
}
```

### ADSR Envelope
```cpp
// ✓ GOOD - Let JUCE handle timing
juce::ADSR adsr;
adsr.setSampleRate(sampleRate);
adsr.setParameters(params);  // Attack, Decay, Sustain, Release
float envelopeValue = adsr.getNextSample();

// ✓ GOOD - Check if envelope is finished
if (!adsr.isActive()) {
    clearCurrentNote();  // Note has fully released
}

// ✗ BAD - Manual envelope timing (complex, error-prone)
float envelope = attack * time;  // What about sustain? Release?
```

---

## UI Best Practices

### Component Attachment
```cpp
// ✓ GOOD - Unique components per slider
sliderController gainSlider{ "Gain" };
std::unique_ptr<SliderAttachment> mGainAttachment;

void setupUI() {
    mGainAttachment = std::make_unique<SliderAttachment>(
        audioProcessor.getAPVTS(), "GAIN", gainSlider
    );
}

// ✓ GOOD - Rebinding for different pads
void changeSliderParameter(const juce::String& newParamID) {
    mGainAttachment.reset();
    mGainAttachment = std::make_unique<SliderAttachment>(
        audioProcessor.getAPVTS(), newParamID, gainSlider
    );
}

// ✗ BAD - Creating many attachments (memory leak)
for (int i = 0; i < NUM_PADS; ++i) {
    new SliderAttachment(...);  // Never deleted!
}
```

### Draggable Components
```cpp
// ✓ GOOD - Proper hit testing
bool hitTest(int x, int y) override {
    // Only capture mouse when near handle
    return std::abs(x - handleX) < kHitTolerance;
}

// ✓ GOOD - Mouse state tracking
void mouseDown(const juce::MouseEvent& e) override {
    if (hitTest(e.x, e.y))
        isDragging = true;
}

void mouseDrag(const juce::MouseEvent& e) override {
    if (isDragging) {
        float newValue = (e.x - bounds.getX()) / (float)bounds.getWidth();
        updateParameter(newValue);
        repaint();
    }
}

void mouseUp(const juce::MouseEvent& e) override {
    isDragging = false;
}
```

---

## Performance Optimization

### Avoid in Audio Thread
```cpp
// ✗ Very expensive - avoids in real-time!
std::vector<float> data(100000);  // Allocation!
data.push_back(value);            // Reallocation!

// ✗ Expensive - can cause stutters
juce::File file = juce::File::chooseFileToOpen();  // UI dialog!

// ✗ Expensive - lock contention
{
    juce::ScopedLock lock(mutex);                 // Lock!
    dataStructure.append(value);
}

// ✓ Pre-allocate buffers
juce::AudioBuffer<float> multiOutBuffer;  // Allocated in prepareToPlay()

// ✓ Use atomic variables
std::atomic<float> currentValue;
currentValue.store(newValue);
```

### Lazy Evaluation
```cpp
// ✓ GOOD - Only update when needed
if (mUpdateCount > 0) {
    updateADSR(sampleIndex);
    mUpdateCount--;
}

// ✗ BAD - Every block (wasteful)
void processBlock(...) {
    updateADSR(sampleIndex);  // Called every ~20ms!
}
```

---

## Testing Checklist

### Before Shipping
- [ ] **Compile** without warnings
  ```bash
  cmake --build . -- VERBOSE=1 2>&1 | grep -i warning
  ```

- [ ] **Test all 16 pads** loading different samples
- [ ] **Test MIDI** triggering from external controller
- [ ] **Test parameter changes** at various sample rates
- [ ] **Test extreme values** (very low/high frequencies, long sustains)
- [ ] **Test with large files** (10+ MB samples)
- [ ] **Monitor CPU** with audio meter during polyphonic playback
- [ ] **Test UI** responsiveness during audio playback
- [ ] **Memory profile** for leaks (valgrind, Instruments)

### Audio Quality Checks
```cpp
// Ensure audio remains clean
✓ No clicks/pops at note boundaries
✓ Filter sweeps smooth (no artifacts)
✓ Pitch shifts are transparent
✓ No phase discontinuities
✓ Output stays within [-1.0, 1.0]
```

---

## Common Pitfalls

### 1. Parameter Suffix Management
```cpp
// ✗ Easy to mess up
auto suffix = padIndex > 0 ? juce::String(padIndex) : "";

// ✓ Consistent pattern
auto suffix = (padIndex == 0) ? juce::String("") : juce::String(padIndex + 1);
// Pad 1: ""    → "GAIN", "ATTACK"
// Pad 2: "2"   → "GAIN2", "ATTACK2"
// Pad 16: "16" → "GAIN16", "ATTACK16"
```

### 2. Array Out of Bounds
```cpp
// ✗ No bounds check
pads[padIndex].gain = value;

// ✓ Proper bounds check
if (padIndex >= 0 && padIndex < NUM_PADS)
    pads[padIndex].gain = value;
```

### 3. Null Pointer Dereference
```cpp
// ✗ Assumes pointer is valid
auto sound = dynamic_cast<CustomSamplerSound*>(mSampler.getSound(i).get());
sound->setGainLinear(0.5f);  // Crash if cast fails!

// ✓ Check before use
if (auto* sound = dynamic_cast<CustomSamplerSound*>(mSampler.getSound(i).get()))
    sound->setGainLinear(0.5f);
```

### 4. Audio Thread Blocking
```cpp
// ✗ Blocks real-time thread
void processBlock(...) {
    for (int i = 0; i < 10000000; ++i) {
        doExpensiveCalculation();  // CPU spikes, audio drops
    }
}

// ✓ Defer to background thread if possible
void processBlock(...) {
    if (mNeedsUpdate) {
        triggerBackgroundUpdate();  // Non-blocking
        mNeedsUpdate = false;
    }
}
```

---

## Code Review Checklist

**When reviewing pull requests, check:**

- [ ] **Memory**: All `new` paired with `delete`? Use `unique_ptr`?
- [ ] **Threading**: No allocations/I/O in audio thread?
- [ ] **Parameters**: Bounds checked? Suffix naming consistent?
- [ ] **Documentation**: Methods documented? Edge cases explained?
- [ ] **Error Handling**: Invalid inputs handled gracefully?
- [ ] **Performance**: No unnecessary copies? DSP optimized?
- [ ] **JUCE**: Following JUCE conventions? Using JUCE utilities?
- [ ] **Compilation**: No warnings? Cross-platform compatible?

---

## Resources

### JUCE Learning
- Official Tutorial: https://docs.juce.com/master/tutorial_main.html
- Audio Programming: https://www.juce.com/learn/documentation
- API Reference: https://docs.juce.com/master/classJUCE.html

### Audio DSP
- IIR Filter Design: https://en.wikipedia.org/wiki/Infinite_impulse_response
- ADSR Envelope: https://en.wikipedia.org/wiki/Envelope_(music)
- Sample Rate Conversion: https://en.wikipedia.org/wiki/Sample_rate_conversion

### C++ Best Practices
- C++ Core Guidelines: https://github.com/isocpp/CppCoreGuidelines
- Effective C++: Scott Meyers (book)
- Herb Sutter's Blog: https://herbsutter.com/

---

## Quick Reference

### Parameter Declaration Pattern
```cpp
parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID(name + suffix, 1),
    name,
    minValue, maxValue,
    defaultValue,
    unit
));
```

### Slider Attachment Pattern
```cpp
mAttachment = std::make_unique<SliderAttachment>(
    audioProcessor.getAPVTS(),
    "PARAMETER_ID",
    slider
);
```

### Voice Activation Pattern
```cpp
if (auto* v = dynamic_cast<CustomSamplerVoice*>(mSampler.getVoice(i))) {
    if (v->isVoiceActive()) {
        // Process active voice
    }
}
```

---

Last Updated: June 2026
