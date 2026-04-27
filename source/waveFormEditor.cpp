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
    g.setColour(juce::Colours::white);
    g.fillAll(juce::Colours::white);

    g.setColour(juce::Colours::red);                               
    auto audioLength = (float)Processor.thumbnail.getTotalLength();

    Processor.thumbnail.drawChannels(g,                                      
        getLocalBounds(),
        0.0,                                    // start time
        audioLength,             // end time
        1.0f);                                  // vertical zoom
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
    
    g.setColour(juce::Colours::black);
    g.drawLine(lengthLineX, 0.0f, lengthLineX,(float)getHeight(), 3.0f);

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
    float sx = startX();
    float w  = (float)getWidth();
    return juce::jlimit(sx, w, sx + getParam("ATTACK") * w);
}

float ADSROverlay::decayX()
{
    float sx = startX();
    float w  = (float)getWidth();
    return juce::jlimit(sx, w, sx + (getParam("ATTACK") + getParam("DECAY")) * w);
}

float ADSROverlay::sustainY()
{
    return juce::jlimit(0.0f, (float)getHeight(),
                        (1.0f - getParam("SUSTAIN")) * (float)getHeight());
}

// Release is shown from the right edge; this returns where the release phase begins.
float ADSROverlay::releaseX()
{
    // param 0-1 maps to time fraction of TOTAL audio, anchored from the right edge
    float sx = startX();
    float w  = (float)getWidth();
    return juce::jlimit(sx, w, w - getParam("RELEASE") * w);
}

// ---- Hit testing ----

ADSROverlay::DragHandle ADSROverlay::hitTestForHandle(float x, float y)
{
    float ax  = attackX();
    float dx  = decayX();
    float sy  = sustainY();
    float rx  = juce::jmax(dx, releaseX());  // release start can't be before decay end

    // Vertical lines: tested by x proximity
    if (std::abs(x - ax) < kHitTolerance)  return DragHandle::Attack;
    if (std::abs(x - dx) < kHitTolerance)  return DragHandle::Decay;
    if (std::abs(x - rx) < kHitTolerance)  return DragHandle::Release;

    // Sustain horizontal line: test by y proximity while x is between decay and release
    if (std::abs(y - sy) < kHitTolerance && x > dx && x < rx)
        return DragHandle::Sustain;

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
        case DragHandle::Release:
            setMouseCursor(juce::MouseCursor::LeftRightResizeCursor); break;
        case DragHandle::Sustain:
            setMouseCursor(juce::MouseCursor::UpDownResizeCursor); break;
        default:
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
    float y = juce::jlimit(0.0f, (float)getHeight(), (float)event.y);
    float w = (float)getWidth();
    float h = (float)getHeight();
    float sx = startX();

    switch (activeDrag)
    {
        case DragHandle::Attack:
        {
            // Attack: distance from startX as fraction of TOTAL width
            float newVal = juce::jlimit(0.0f, 1.0f, (x - sx) / w);
            setParam("ATTACK", newVal);
            break;
        }
        case DragHandle::Decay:
        {
            // Decay: portion after attack, as fraction of TOTAL width
            float attack = getParam("ATTACK");
            float newVal = juce::jlimit(0.0f, 1.0f, (x - sx) / w - attack);
            setParam("DECAY", newVal);
            break;
        }
        case DragHandle::Sustain:
        {
            // Sustain level (inverted Y: bottom=0, top=1)
            float normY = y / h;
            float newVal = juce::jlimit(0.0f, 1.0f, 1.0f - normY);
            setParam("SUSTAIN", newVal);
            break;
        }
        case DragHandle::Release:
        {
            // Release: distance from right edge as fraction of TOTAL width
            float newVal = juce::jlimit(0.0f, 1.0f, (w - x) / w);
            setParam("RELEASE", newVal);
            break;
        }
        default: break;
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

    float w  = (float)getWidth();
    float h  = (float)getHeight();
    float sx = startX();
    float ax = attackX();
    float dx = decayX();
    float sy = sustainY();
    float rx = juce::jmax(dx, releaseX());   // release start can't be before decay end

    // --- Envelope shape ---
    juce::Path env;
    env.startNewSubPath(sx, h);   // start at sample start point
    env.lineTo(ax, 0.0f);            // attack peak
    env.lineTo(dx, sy);              // decay to sustain level
    env.lineTo(rx, sy);              // sustain flat
    env.lineTo(w,  h);               // release to zero
    env.closeSubPath();

    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.fillPath(env);
    g.setColour(juce::Colours::white.withAlpha(0.35f));
    g.strokePath(env, juce::PathStrokeType(1.2f));

    // --- Draw angled handle lines that follow the envelope slope ---
    
    // Attack line: diagonal from startX to peak (angled based on attack duration)
    g.setColour(juce::Colour(0xff44ff88).withAlpha(0.8f));
    g.drawLine(sx, h, ax, 0.0f, 2.0f);
    
    // Decay line: diagonal from peak to sustain level (angled based on decay duration)
    g.setColour(juce::Colour(0xffffd740).withAlpha(0.8f));
    g.drawLine(ax, 0.0f, dx, sy, 2.0f);
    
    // Sustain line: horizontal flat line
    g.setColour(juce::Colour(0xff40c8ff).withAlpha(0.8f));
    g.drawLine(dx, sy, rx, sy, 2.0f);
    
    // Release line: diagonal from sustain to end (angled based on release duration)
    g.setColour(juce::Colour(0xffff7043).withAlpha(0.8f));
    g.drawLine(rx, sy, w, h, 2.0f);

    // --- Handle circles with labels ---
    const auto drawHandleCircle = [&](float hx, float hy,
                                       juce::Colour colour,
                                       const juce::String& label)
    {
        g.setColour(colour);
        g.fillEllipse(hx - kHandleRadius, hy - kHandleRadius,
                      kHandleRadius * 2.0f, kHandleRadius * 2.0f);

        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.setColour(juce::Colours::black);
        g.drawText(label, (int)(hx - kHandleRadius), (int)(hy - kHandleRadius),
                   (int)(kHandleRadius * 2), (int)(kHandleRadius * 2),
                   juce::Justification::centred, false);
    };

    // Draw handle circles at the key points
    drawHandleCircle(ax,          0.0f,  juce::Colour(0xff44ff88), "A");  // Attack  – green
    drawHandleCircle(dx,          sy,    juce::Colour(0xffffd740), "D");  // Decay   – yellow
    drawHandleCircle((dx + rx) * 0.5f, sy, juce::Colour(0xff40c8ff), "S");  // Sustain – cyan
    drawHandleCircle(rx,          sy,    juce::Colour(0xffff7043), "R");  // Release – orange
}

