#include "PluginEditor.h"

R2RZXAudioProcessorEditor::R2RZXAudioProcessorEditor (R2RZXAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (920, 620);
    title.setText ("R2R-ZX", juce::dontSendNotification);
    title.setFont (juce::Font (juce::FontOptions (34.0f, juce::Font::bold)));
    title.setColour (juce::Label::textColourId, juce::Colour (0xff00eaff));
    addAndMakeVisible (title);
    subtitle.setText ("POLYPHONIC WAVETABLE SYNTH  /  SPATIAL ENGINE", juce::dontSendNotification);
    subtitle.setColour (juce::Label::textColourId, juce::Colour (0xffff405d));
    addAndMakeVisible (subtitle);

    auto setup = [this] (juce::Slider& s, const juce::String& name)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 22);
        s.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff00dff2));
        s.setColour (juce::Slider::thumbColourId, juce::Colour (0xffff405d));
        s.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
        s.setName (name);
        addAndMakeVisible (s);
    };
    setup (range, "S&H RANGE");
    setup (attack, "ATTACK");
    setup (decay, "DECAY");
    setup (sustain, "SUSTAIN");
    setup (release, "RELEASE");
    rangeAttachment = std::make_unique<Attachment> (processor.parameters, "SH_RANGE", range);
    attackAttachment = std::make_unique<Attachment> (processor.parameters, "ATTACK", attack);
    decayAttachment = std::make_unique<Attachment> (processor.parameters, "DECAY", decay);
    sustainAttachment = std::make_unique<Attachment> (processor.parameters, "SUSTAIN", sustain);
    releaseAttachment = std::make_unique<Attachment> (processor.parameters, "RELEASE", release);
}
void R2RZXAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff080b13));
    auto bounds = getLocalBounds().toFloat().reduced (18.0f);
    g.setColour (juce::Colour (0xff101a29));
    g.fillRoundedRectangle (bounds, 18.0f);
    g.setColour (juce::Colour (0xff00dff2));
    g.drawRoundedRectangle (bounds, 18.0f, 1.6f);
    g.setColour (juce::Colour (0x5539dfff));
    for (int i = 0; i < 8; ++i)
        g.drawRoundedRectangle (bounds.reduced (float (i * 2)), 18.0f, 0.5f);
    g.setColour (juce::Colour (0xffff405d));
    g.drawLine (28.0f, 112.0f, (float) getWidth() - 28.0f, 112.0f, 1.2f);
    g.setColour (juce::Colour (0xff00dff2));
    g.setFont (14.0f);
    g.drawText ("PER-VOICE RANDOM PITCH MODULATION  |  MAX RANGE: +/-100 CENTS", 30, 135, getWidth() - 60, 24, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xffff405d));
    g.drawText ("OSCILLATOR / WAVETABLE / FILTER / SPATIAL FX MODULES - IN DEVELOPMENT", 30, getHeight() - 42, getWidth() - 60, 22, juce::Justification::centred);
}
void R2RZXAudioProcessorEditor::resized()
{
    title.setBounds (30, 24, 320, 52);
    subtitle.setBounds (32, 76, 520, 24);
    const int y = 205, w = 150, gap = 20;
    juce::Slider* controls[] = { &range, &attack, &decay, &sustain, &release };
    for (int i = 0; i < 5; ++i)
        controls[i]->setBounds (30 + i * (w + gap), y, w, 190);
}
