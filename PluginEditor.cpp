/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DrumSamplerAudioProcessorEditor::DrumSamplerAudioProcessorEditor (DrumSamplerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor(p){ 
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (800, 400);

    myButton.setButtonText("myButton");
    addAndMakeVisible(&myButton);
    myButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
    myButton.onClick = [this] {ButtonClicked(&myButton, 60); };
    addAndMakeVisible(&waveForm);
    addAndMakeVisible(&CBlock);

    
}

DrumSamplerAudioProcessorEditor::~DrumSamplerAudioProcessorEditor()
{
}

//==============================================================================
void DrumSamplerAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    //g.fillAll(juce::Colours::black);

    //g.setColour(juce::Colours::red);
    //g.setFont(15.0f);
    //
}

void DrumSamplerAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    myButton.setBounds(10, getHeight()-80, 70, 70);
    waveForm.setBounds((getWidth() / 2), (getHeight() / 2) - 270, 250, 250);
    CBlock.setBounds((getWidth() / 2), (getHeight() / 2), (getWidth() / 2)-50, 150);
   
    //GainSlider.setBounds(getWidth() / 2 + 150, getHeight() / 2 - 50, 80, 150);
}

void DrumSamplerAudioProcessorEditor::ButtonClicked(juce::Button* button, int noteNumber)
{
    if (button == &myButton)
    {   
        audioProcessor.playFile(noteNumber);
    }
}




//this is your drag and drop button//
DragAndDropButton::DragAndDropButton(DrumSamplerAudioProcessor& p): Processor(p) {
 }

DragAndDropButton::~DragAndDropButton(){}

bool DragAndDropButton::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto file : files)
    {
        if (file.contains(".wav") || file.contains(".mp3")) return true;
    }
    return false;
}

void DragAndDropButton::filesDropped(const juce::StringArray& files, int x, int y)
{
    for (auto file : files)
    {
        if (isInterestedInFileDrag(files)) {

            DBG("File dropped: " << file);
            Processor.loadFile(file);
        }
    }
    repaint();
}

void DragAndDropButton::paint(juce::Graphics& g)
{
  
    g.fillAll(juce::Colours::grey);

    if (Processor.getNumSamplerSounds() > 0)
    {
        g.fillAll(juce::Colours::red);

        g.drawText("Sound Loaded", getWidth() / 2 - 50, getHeight() / 2 - 10, 100, 20, juce::Justification::centred);
    }
    else
    {
        g.drawText("Drop here", getWidth() / 2 - 50, getHeight() / 2 - 10, 100, 20, juce::Justification::centred);
    }

}

waveFormEditor::waveFormEditor(DrumSamplerAudioProcessor& p): Processor(p)
{
    Processor.thumbnail.addChangeListener(this);
}


void waveFormEditor::paint(juce::Graphics& g) {

    juce::Rectangle<int> thumbnailBounds(10, 100, getWidth() - 20, getHeight() - 120);

    if (Processor.thumbnail.getNumChannels() == 0)
        paintIfNoFileLoaded(g, thumbnailBounds);
    else
        paintIfFileLoaded(g, thumbnailBounds);

}

void waveFormEditor::paintIfNoFileLoaded(juce::Graphics& g, const juce::Rectangle<int>& thumbnailBounds)
{
    g.setColour(juce::Colours::darkgrey);
    g.fillRect(thumbnailBounds);
    g.setColour(juce::Colours::white);
    g.drawFittedText("No File Loaded", thumbnailBounds, juce::Justification::centred, 1);
}

void waveFormEditor::paintIfFileLoaded(juce::Graphics& g, const juce::Rectangle<int>& thumbnailBounds)
{
    g.setColour(juce::Colours::white);
    g.fillRect(thumbnailBounds);

    g.setColour(juce::Colours::red);                               // [8]

    Processor.thumbnail.drawChannels(g,                                      // [9]
        thumbnailBounds,
        0.0,                                    // start time
        Processor.thumbnail.getTotalLength(),             // end time
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

sliderController::sliderController(juce::String name) {

    addAndMakeVisible(&nameLabel);
    setTextBoxStyle(TextBoxBelow, true, getTextBoxWidth()-20, getTextBoxHeight());
    nameLabel.setFont(15.0);
    nameLabel.setText(name, juce::dontSendNotification);
    nameLabel.attachToComponent(this, true);
    nameLabel.setJustificationType(juce::Justification::centredTop);

}
void sliderController::attachLabel(Component* owner, bool onLeft)
{
    nameLabel.attachToComponent(owner, onLeft);
}

controlSlidersBlock::controlSlidersBlock(DrumSamplerAudioProcessor& p) :  audioProcessor(p)
{   
    //setSize(200, 100);
    addAndMakeVisible(&VolSlider);
    VolSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    VolSlider.setRange(-50.0f, 0.0f, 0.01f);
    VolSlider.setValue(-12.0f);
    VolSlider.onValueChange = [this] { sliderValueChanged(&VolSlider); };
    VolSlider.attachLabel(&VolSlider, false);

    addAndMakeVisible(&AttackSlider);
    AttackSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    AttackSlider.setRange(0.0f, 1.0f, 0.01f);
    AttackSlider.setValue(0.0f);
    AttackSlider.onValueChange = [this] { sliderValueChanged(&AttackSlider); };
    AttackSlider.attachLabel(&AttackSlider, false);

    addAndMakeVisible(&DecaySlider);
    DecaySlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    DecaySlider.setRange(0.0f, 1.0f, 0.01f);
    DecaySlider.setValue(1.0f);
    DecaySlider.onValueChange = [this] { sliderValueChanged(&DecaySlider); };
    DecaySlider.attachLabel(&DecaySlider, false);

    addAndMakeVisible(&ReleaseSlider);
    ReleaseSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    ReleaseSlider.setRange(0.0f, 1.0f, 0.01f);
    ReleaseSlider.setValue(1.0f);
    ReleaseSlider.onValueChange = [this] { sliderValueChanged(&ReleaseSlider); };
    ReleaseSlider.attachLabel(&ReleaseSlider, false);

    addAndMakeVisible(&SustainSlider);
    SustainSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    SustainSlider.setRange(0.0f, 1.0f, 0.01f);
    SustainSlider.setValue(1.0f);
    SustainSlider.onValueChange = [this] { sliderValueChanged(&SustainSlider); };
    SustainSlider.attachLabel(&SustainSlider, false);

}

void controlSlidersBlock::sliderValueChanged(juce::Slider* slider) {
    if (slider == &VolSlider)
    {
        audioProcessor.gain = VolSlider.getValue();
        DBG(VolSlider.getValue());
    }
    else if (slider == &AttackSlider)
    {
        audioProcessor.getADSRparams().attack = AttackSlider.getValue();
    }
    else if (slider == &DecaySlider)
    {
        audioProcessor.getADSRparams().decay = DecaySlider.getValue();
    }
    else if (slider == &SustainSlider)
    {
        audioProcessor.getADSRparams().sustain = SustainSlider.getValue();
    }
    else if (slider == &ReleaseSlider)
    {
        audioProcessor.getADSRparams().release = ReleaseSlider.getValue();
    }

    audioProcessor.updateADSR();

}

void controlSlidersBlock::resized() {


    const auto x = 0;
    const auto y = (getHeight() / 2);
    const auto sWidth = 70;
    const auto sHeight = 70;

    VolSlider.setBounds(x, y, sWidth, sHeight);
    AttackSlider.setBounds(x + sWidth, (getHeight() / 2), sWidth, sHeight);
    DecaySlider.setBounds(x + sWidth * 2, (getHeight() / 2), sWidth, sHeight);
    SustainSlider.setBounds(x + sWidth * 3, (getHeight() / 2), sWidth, sHeight);
    ReleaseSlider.setBounds(x + sWidth * 4, (getHeight() / 2), sWidth, sHeight);
}

void controlSlidersBlock::paint(juce::Graphics& g)
{
  

    g.fillAll(juce::Colours::grey);
    //juce::Rectangle<int> area(0, 0, getWidth(), getHeight());
    //g.fillRect(area);

}