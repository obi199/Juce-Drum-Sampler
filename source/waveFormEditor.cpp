/*
  ==============================================================================

    waveFormEditor.cpp
    Created: 17 Aug 2024 3:05:24pm
    Author:  obi

  ==============================================================================
*/

#include <JuceHeader.h>
#include "waveFormEditor.h"


//==============================================================================
waveFormEditor::waveFormEditor(DrumSamplerAudioProcessor& p)
    : Processor(p)
{
    Processor.thumbnail.addChangeListener(this);
    setWantsKeyboardFocus(true);
}

waveFormEditor::~waveFormEditor()
{
}

void waveFormEditor::paint(juce::Graphics& g) {

    if (Processor.thumbnail.getNumChannels() == 0)
        paintIfNoFileLoaded(g);
    else {
        paintIfFileLoaded(g);
        g.setColour(juce::Colours::green);
        //g.drawLine(lengthLineX, 0, lengthLineX, getHeight(), 4.5f);
    }
}

void waveFormEditor::paintIfNoFileLoaded(juce::Graphics& g)
{
    g.setColour(juce::Colours::darkgrey);
    g.fillAll(juce::Colours::white);
    g.setColour(juce::Colours::red);
    g.drawFittedText("No File Loaded", getLocalBounds(), juce::Justification::centred, 1);
}

void waveFormEditor::paintIfFileLoaded(juce::Graphics& g)
{
    // The background of the editor is black.
    // We only fill with white the part that is being played.
    // The unplayed part (before startOffset) remains transparent/black.

    g.setColour(juce::Colours::red);                               
    auto audioLength = (float)Processor.thumbnail.getTotalLength();

    int padIndex = Processor.getCurrentPadIndex();
    auto suffix = (padIndex == 0) ? juce::String("") : juce::String(padIndex + 1);
    float startOffset = 0.0f;
    if (auto* v = Processor.getAPVTS().getRawParameterValue("START_OFFSET" + suffix))
        startOffset = v->load();

    float endOffset = 1.0f;
    if (auto* v = Processor.getAPVTS().getRawParameterValue("END_OFFSET" + suffix))
        endOffset = v->load();

    auto bounds = getLocalBounds();
    auto startX = startOffset * (float)bounds.getWidth();
    auto endX   = endOffset   * (float)bounds.getWidth();

    // Fill only the played part background with white
    g.setColour(juce::Colours::white);
    g.fillRect(bounds.withLeft((int)startX).withRight((int)endX));

    g.setColour(juce::Colours::red);
    Processor.thumbnail.drawChannels(g,                                      
        bounds,
        0.0,                                   // always draw from the beginning
        audioLength,                           // end time
        1.0f);                                 // vertical zoom
}

void waveFormEditor::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &Processor.thumbnail)  thumbnailChanged();
}

void waveFormEditor::thumbnailChanged()
{
    repaint();
}

//
//Position line Class==============================================================================

positionLine::positionLine(DrumSamplerAudioProcessor& p) : Processor(p)
{
    startTimer(30);
}

positionLine::~positionLine()
{
    stopTimer();
}

void positionLine::paint(juce::Graphics& g)
{
    auto audioLength = (float)Processor.thumbnail.getTotalLength();
    if (audioLength > 0.0) {
        auto audioPosition = (float)Processor.getPosInSec();
        
        // Ensure the line is drawn relative to the whole thumbnail
        auto drawPosition = (audioPosition / audioLength) * (float)getWidth();
        
        // If we want it relative to the start line (optional, but usually preferred):
        // However, getPosInSec() now returns the absolute position in the sample 
        // because we get it from CustomSamplerVoice::currentSamplePos.
        
        g.setColour(juce::Colours::red);
        g.drawLine(drawPosition, 0.0f, drawPosition, (float)getHeight(), 1.0f);
    }
}

void positionLine::timerCallback()
{
    repaint();
}




//Start Line Class==============================================================================
startLine::startLine(DrumSamplerAudioProcessor& p) : Processor(p)
{
    // Poll for knob changes every 30ms (same rate as positionLine)
    startTimer(30);
}

startLine::~startLine()
{
    stopTimer();
}

void startLine::timerCallback()
{
    // Poll the current knob value and update if it changed
    int currentPad = Processor.getCurrentPadIndex();
    auto suffix = (currentPad == 0) ? juce::String("") : juce::String(currentPad + 1);
    
    if (auto* param = Processor.getAPVTS().getRawParameterValue("START_OFFSET" + suffix))
    {
        float currentKnobValue = param->load();
        
        // Only update if the knob value has changed (to avoid unnecessary repaints)
        if (std::abs(currentKnobValue - lastKnobValue) > 0.0001f)
        {
            lastKnobValue = currentKnobValue;
            setNormalizedOffset(currentKnobValue);
        }
    }
}

void startLine::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::green);
    g.drawLine(lengthLineX, 0.0f, lengthLineX, (float)getHeight(), 5.0f);
}

bool startLine::hitTest(int x, int /*y*/)
{
    return std::abs((float)x - lengthLineX) <= 8.0f;
}


void startLine::mouseDrag(const juce::MouseEvent& event)
{
    //Update the position of the length line based on mouse drag
    if (isMouseOver()) DBG("mouseOver");
    //lengthLineX = event.x;
    lengthLineX = juce::jlimit(0.0f, (float)getWidth(), (float)event.x);
    DBG("lengthLineX=" << lengthLineX);
    auto audioLength = (float)Processor.thumbnail.getTotalLength();
    DBG("audioLength=" << audioLength);
    newPositionInSeconds = (lengthLineX / (float)getWidth()) * audioLength;
    DBG("newPositionInSeconds=" << newPositionInSeconds);
    Processor.newPositionSec = newPositionInSeconds;

    // Persist normalized offset immediately to APVTS for the current pad
    float ratio = 0.0f;
    if (audioLength > 0.0f)
        ratio = juce::jlimit(0.0f, 0.9999f, newPositionInSeconds / audioLength);

    int currentPad = Processor.getCurrentPadIndex();
    if (currentPad >= 0 && currentPad < NUM_PADS)
    {
        Processor.setStartOffsetForNote(MIDI_NOTES[currentPad], ratio);
    }

    //Optionally, update the sample length based on the new position
   //updateSampleLength();
    repaint();
}

void startLine::setPosition(float positionInSeconds)
{
    newPositionInSeconds = positionInSeconds;
    auto audioLength = (float)Processor.thumbnail.getTotalLength();
    if (audioLength > 0.0f)
        lengthLineX = (newPositionInSeconds / audioLength) * (float)getWidth();
    else
        lengthLineX = 0.0f;

    Processor.newPositionSec = newPositionInSeconds;
    repaint();
}

void startLine::setNormalizedOffset(float offsetRatio01)
{
    // Clamp ratio to [0,1]
    float r = juce::jlimit(0.0f, 1.0f, offsetRatio01);
    // Set visual immediately based on component width (no need for audio length here)
    lengthLineX = r * (float)getWidth();

    // Update processor seconds if we know audio length; otherwise leave as is
    auto audioLength = (float)Processor.thumbnail.getTotalLength();
    if (audioLength > 0.0f)
        newPositionInSeconds = r * audioLength;
    Processor.newPositionSec = newPositionInSeconds;

    repaint();
}


// End Line Class==============================================================================
endLine::endLine(DrumSamplerAudioProcessor& p) : Processor(p)
{
    startTimer(30);
}

endLine::~endLine()
{
    stopTimer();
}

void endLine::timerCallback()
{
    int currentPad = Processor.getCurrentPadIndex();
    auto suffix = (currentPad == 0) ? juce::String("") : juce::String(currentPad + 1);

    if (auto* param = Processor.getAPVTS().getRawParameterValue("END_OFFSET" + suffix))
    {
        float currentKnobValue = param->load();
        if (std::abs(currentKnobValue - lastKnobValue) > 0.0001f)
        {
            lastKnobValue = currentKnobValue;
            setNormalizedOffset(currentKnobValue);
        }
    }
}

void endLine::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::green);
    g.drawLine(lengthLineX, 0.0f, lengthLineX, (float)getHeight(), 5.0f);
}

bool endLine::hitTest(int x, int /*y*/)
{
    return std::abs((float)x - lengthLineX) <= 8.0f;
}

void endLine::mouseDrag(const juce::MouseEvent& event)
{
    lengthLineX = juce::jlimit(0.0f, (float)getWidth(), (float)event.x);

    auto audioLength = (float)Processor.thumbnail.getTotalLength();
    float ratio = 1.0f;
    if (audioLength > 0.0f)
        ratio = juce::jlimit(0.0001f, 1.0f, (lengthLineX / (float)getWidth()));

    int currentPad = Processor.getCurrentPadIndex();
    if (currentPad >= 0 && currentPad < NUM_PADS)
    {
        Processor.setEndOffsetForNote(MIDI_NOTES[currentPad], ratio);
    }

    repaint();
}

void endLine::setNormalizedOffset(float offsetRatio01)
{
    float r = juce::jlimit(0.0f, 1.0f, offsetRatio01);
    lengthLineX = r * (float)getWidth();
    repaint();
}


// ADSROverlay ============================================================================

ADSROverlay::ADSROverlay(DrumSamplerAudioProcessor& p) : Processor(p)
{
    startTimer(30);
}

ADSROverlay::~ADSROverlay()
{
    stopTimer();
}

void ADSROverlay::timerCallback()
{
    repaint();
}

// ---- APVTS helpers ----

float ADSROverlay::getParam(const juce::String& name)
{
    int padIndex = Processor.getCurrentPadIndex();
    auto suffix = (padIndex == 0) ? juce::String("") : juce::String(padIndex + 1);
    if (auto* v = Processor.getAPVTS().getRawParameterValue(name + suffix))
        return v->load();
    return 0.0f;
}

void ADSROverlay::setParam(const juce::String& name, float value)
{
    int padIndex = Processor.getCurrentPadIndex();
    auto suffix = (padIndex == 0) ? juce::String("") : juce::String(padIndex + 1);
    if (auto* p = Processor.getAPVTS().getParameter(name + suffix))
        p->setValueNotifyingHost(value);
}

// ---- Coordinate helpers (all in component pixels) ----
// Parameters store normalized 0-1 values which map to pixel positions on the waveform.
// These positions are visual only - the actual time mapping (0-1 → 0-5 seconds) happens
// in the audio processor's updateADSR() function.
// Sustain (0-1 level) maps to Y with 0=bottom, 1=top.

// startX returns the pixel x-position of the sample start point.
float ADSROverlay::startX()
{
    int padIndex = Processor.getCurrentPadIndex();
    auto suffix = (padIndex == 0) ? juce::String("") : juce::String(padIndex + 1);
    float startOffset = 0.0f;
    if (auto* v = Processor.getAPVTS().getRawParameterValue("START_OFFSET" + suffix))
        startOffset = v->load();
    return startOffset * (float)getWidth();
}

float ADSROverlay::attackX()
{
    // param 0-1 maps to time fraction of TOTAL audio, visual origin shifted to startX
    // A minimum gap of 12px is kept so the attack handle never sits on the start line.
    float sx = startX();
    float w  = (float)getWidth();
    constexpr float kMinGap = 12.0f;
    return juce::jlimit(sx + kMinGap, w, sx + kMinGap + getParam("ATTACK") * w);
}

float ADSROverlay::decayX()
{
    // F handle = where the fade-out STARTS (FADE_START is a position 0-1 on the waveform)
    int padIndex = Processor.getCurrentPadIndex();
    auto suffix = (padIndex == 0) ? juce::String("") : juce::String(padIndex + 1);
    float fadeStart = 0.8f;
    if (auto* v = Processor.getAPVTS().getRawParameterValue("FADE_START" + suffix))
        fadeStart = v->load();
    float ax = attackX();
    float w  = (float)getWidth();
    return juce::jmax(ax + 12.0f, fadeStart * w);
}

float ADSROverlay::fadeEndX()
{
    int padIndex = Processor.getCurrentPadIndex();
    auto suffix = (padIndex == 0) ? juce::String("") : juce::String(padIndex + 1);
    float fadeEnd = 1.0f;
    if (auto* v = Processor.getAPVTS().getRawParameterValue("FADE_END" + suffix))
        fadeEnd = v->load();
    float dx = decayX();
    float ex = endX();
    return juce::jlimit(dx + 12.0f, ex, fadeEnd * (float)getWidth());
}

float ADSROverlay::endX()
{
    int padIndex = Processor.getCurrentPadIndex();
    auto suffix = (padIndex == 0) ? juce::String("") : juce::String(padIndex + 1);
    float endOffset = 1.0f;
    if (auto* v = Processor.getAPVTS().getRawParameterValue("END_OFFSET" + suffix))
        endOffset = v->load();
    // Clamp so end is always to the right of the fade start
    float dx = decayX();
    return juce::jmax(dx + 12.0f, endOffset * (float)getWidth());
}

// ---- Hit testing ----

ADSROverlay::DragHandle ADSROverlay::hitTestForHandle(float x, float y)
{
    float ax  = attackX();
    float dx  = decayX();
    float fex = fadeEndX();

    if (std::abs(x - ax)  < kHitTolerance)  return DragHandle::Attack;
    if (std::abs(x - dx)  < kHitTolerance)  return DragHandle::Decay;
    if (std::abs(x - fex) < kHitTolerance)  return DragHandle::FadeEnd;

    return DragHandle::None;
}

bool ADSROverlay::hitTest(int x, int y)
{
    // During an active drag keep capturing all events
    if (activeDrag != DragHandle::None) return true;
    // Otherwise only intercept when the cursor is near a handle
    return hitTestForHandle((float)x, (float)y) != DragHandle::None;
}

// ---- Mouse handlers ----

void ADSROverlay::mouseMove(const juce::MouseEvent& event)
{
    switch (hitTestForHandle((float)event.x, (float)event.y))
    {
        case DragHandle::Attack:
        case DragHandle::Decay:
        case DragHandle::FadeEnd:
            setMouseCursor(juce::MouseCursor::LeftRightResizeCursor); break;
        case DragHandle::None:
            setMouseCursor(juce::MouseCursor::NormalCursor); break;
    }
}

void ADSROverlay::mouseDown(const juce::MouseEvent& event)
{
    activeDrag = hitTestForHandle((float)event.x, (float)event.y);
}

void ADSROverlay::mouseDrag(const juce::MouseEvent& event)
{
    float x = juce::jlimit(0.0f, (float)getWidth(),  (float)event.x);
    float w = (float)getWidth();
    float sx = startX();
    constexpr float kMinGap = 12.0f;

    switch (activeDrag)
    {
        case DragHandle::Attack:
        {
            float newVal = juce::jlimit(0.0f, 1.0f, (x - sx - kMinGap) / w);
            setParam("ATTACK", newVal);
            break;
        }
        case DragHandle::Decay:
        {
            float ax = attackX();
            float minX = ax + 12.0f;
            float clampedX = juce::jlimit(minX, w, x);
            float newVal = juce::jlimit(0.0f, 1.0f, clampedX / w);
            int padIndex = Processor.getCurrentPadIndex();
            auto suffix = (padIndex == 0) ? juce::String("") : juce::String(padIndex + 1);
            if (auto* p = Processor.getAPVTS().getParameter("FADE_START" + suffix))
                p->setValueNotifyingHost(newVal);
            Processor.updateADSR(padIndex);
            break;
        }
        case DragHandle::FadeEnd:
        {
            // Clamp between decayX and endX
            float dx = decayX();
            float ex = endX();
            float clampedX = juce::jlimit(dx + 12.0f, ex, x);
            float newVal = juce::jlimit(0.0f, 1.0f, clampedX / w);
            int padIndex = Processor.getCurrentPadIndex();
            auto suffix = (padIndex == 0) ? juce::String("") : juce::String(padIndex + 1);
            if (auto* p = Processor.getAPVTS().getParameter("FADE_END" + suffix))
                p->setValueNotifyingHost(newVal);
            Processor.updateADSR(padIndex);
            break;
        }
        case DragHandle::None:
            break;
    }

    repaint();
}

void ADSROverlay::mouseUp(const juce::MouseEvent&)
{
    activeDrag = DragHandle::None;
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

// ---- Paint ----

void ADSROverlay::paint(juce::Graphics& g)
{
    if (Processor.thumbnail.getNumChannels() == 0)
        return;   // nothing loaded yet

    float w   = (float)getWidth();
    float h   = (float)getHeight();
    float sx  = startX();
    float ax  = attackX();
    float dx  = decayX();
    float fex = fadeEndX();   // where the fade reaches zero

    // --- Envelope shape: ramp up → flat at full → fade to zero at fade-end ---
    juce::Path env;
    env.startNewSubPath(sx,  h);    // silent at start
    env.lineTo(ax,  0.0f);          // ramp up to peak
    env.lineTo(dx,  0.0f);          // flat at full volume
    env.lineTo(fex, h);             // fade to zero
    env.closeSubPath();

    g.setColour(juce::Colours::red.withAlpha(0.1f));
    g.fillPath(env);
    g.setColour(juce::Colours::red.withAlpha(0.5f));
    g.strokePath(env, juce::PathStrokeType(1.2f));

    // --- Handle lines ---
    g.setColour(juce::Colours::red);
    g.drawLine(sx,  h,    ax,  0.0f, 2.0f);  // attack ramp
    g.drawLine(ax,  0.0f, dx,  0.0f, 2.0f);  // flat plateau
    g.drawLine(dx,  0.0f, fex, h,    2.0f);  // fade-out slope

    // --- Handle circles ---
    const auto drawHandleCircle = [&](float hx, float hy,
                                       juce::Colour colour,
                                       const juce::String& label)
    {
        g.setColour(colour);
        g.fillEllipse(hx - kHandleRadius, hy - kHandleRadius,
                      kHandleRadius * 2.0f, kHandleRadius * 2.0f);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.setColour(juce::Colours::white);
        g.drawText(label, (int)(hx - kHandleRadius), (int)(hy - kHandleRadius),
                   (int)(kHandleRadius * 2), (int)(kHandleRadius * 2),
                   juce::Justification::centred, false);
    };

    drawHandleCircle(ax,  0.0f, juce::Colours::orangered, "A");   // attack end
    drawHandleCircle(dx,  0.0f, juce::Colours::orangered, "F");   // fade start
    drawHandleCircle(fex, h,    juce::Colours::orangered, "E");   // fade end
}

