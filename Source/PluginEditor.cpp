#include "PluginEditor.h"

namespace
{
const juce::Colour bg (0xff070b13), panel (0xff101927), panelEdge (0xff26384c);
const juce::Colour cyan (0xff00e5ff), red (0xffff405d), white (0xffeaf4ff);
}

R2RZXAudioProcessorEditor::R2RZXAudioProcessorEditor (R2RZXAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (1140, 730);
    setResizable (true, true);
    setResizeLimits (980, 650, 1500, 950);

    title.setText ("R2R  /  ZX", juce::dontSendNotification);
    title.setFont (juce::Font (juce::FontOptions (34.0f, juce::Font::bold)));
    title.setColour (juce::Label::textColourId, cyan);
    addAndMakeVisible (title);

    subtitle.setText ("POLYPHONIC SYNTH  •  RANDOM S&H PITCH ENGINE", juce::dontSendNotification);
    subtitle.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    subtitle.setColour (juce::Label::textColourId, red);
    addAndMakeVisible (subtitle);

    presetLabel.setText ("FACTORY BANK  /  100 PRESETS", juce::dontSendNotification);
    presetLabel.setColour (juce::Label::textColourId, white);
    addAndMakeVisible (presetLabel);
    for (int i = 0; i < R2RZXFactoryPresetCount; ++i)
    {
        const auto& info = R2RZXFactoryPresets[i];
        presetBox.addItem (juce::String (i + 1).paddedLeft ('0', 3) + "  " + info.name + "   /   " + info.category, i + 1);
    }
    presetBox.setSelectedId (1, juce::dontSendNotification);
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id > 0) processor.selectPreset (id - 1);
        repaint();
    };
    presetBox.setColour (juce::ComboBox::backgroundColourId, panel);
    presetBox.setColour (juce::ComboBox::textColourId, white);
    presetBox.setColour (juce::ComboBox::outlineColourId, cyan);
    presetBox.setColour (juce::ComboBox::arrowColourId, cyan);
    addAndMakeVisible (presetBox);

    oscLabel.setText ("WAVEFORM", juce::dontSendNotification);
    oscLabel.setColour (juce::Label::textColourId, white);
    addAndMakeVisible (oscLabel);
    oscillatorBox.addItem ("Sine", 1);
    oscillatorBox.addItem ("Saw", 2);
    oscillatorBox.addItem ("Pulse", 3);
    oscillatorBox.addItem ("Triangle", 4);
    oscillatorBox.addItem ("Digital", 5);
    oscillatorBox.setColour (juce::ComboBox::backgroundColourId, panel);
    oscillatorBox.setColour (juce::ComboBox::textColourId, white);
    oscillatorBox.setColour (juce::ComboBox::outlineColourId, red);
    oscillatorBox.setColour (juce::ComboBox::arrowColourId, red);
    addAndMakeVisible (oscillatorBox);

    setupSlider (range, "S&H RANGE");
    setupSlider (portamento, "PORTAMENTO");
    setupSlider (attack, "ATTACK");
    setupSlider (decay, "DECAY");
    setupSlider (sustain, "SUSTAIN");
    setupSlider (release, "RELEASE");
    setupSlider (cutoff, "CUTOFF");
    setupSlider (resonance, "RESONANCE");
    setupSlider (width, "STEREO WIDTH");

    auto& state = processor.parameters;
    rangeAttachment = std::make_unique<SliderAttachment> (state, "SH_RANGE", range);
    portamentoAttachment = std::make_unique<SliderAttachment> (state, "PORTAMENTO", portamento);
    attackAttachment = std::make_unique<SliderAttachment> (state, "ATTACK", attack);
    decayAttachment = std::make_unique<SliderAttachment> (state, "DECAY", decay);
    sustainAttachment = std::make_unique<SliderAttachment> (state, "SUSTAIN", sustain);
    releaseAttachment = std::make_unique<SliderAttachment> (state, "RELEASE", release);
    cutoffAttachment = std::make_unique<SliderAttachment> (state, "CUTOFF", cutoff);
    resonanceAttachment = std::make_unique<SliderAttachment> (state, "RESONANCE", resonance);
    widthAttachment = std::make_unique<SliderAttachment> (state, "WIDTH", width);
    oscillatorAttachment = std::make_unique<ComboAttachment> (state, "OSC_TYPE", oscillatorBox);
}

void R2RZXAudioProcessorEditor::setupSlider (juce::Slider& s, const juce::String& name)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 19);
    s.setColour (juce::Slider::rotarySliderFillColourId, cyan);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff34465b));
    s.setColour (juce::Slider::thumbColourId, red);
    s.setColour (juce::Slider::textBoxTextColourId, white);
    s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff0a111c));
    s.setColour (juce::Slider::textBoxOutlineColourId, panelEdge);
    s.setName (name);
    addAndMakeVisible (s);
}

void R2RZXAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);
    auto bounds = getLocalBounds().toFloat().reduced (14.0f);
    g.setColour (panel);
    g.fillRoundedRectangle (bounds, 16.0f);
    g.setColour (cyan.withAlpha (0.85f));
    g.drawRoundedRectangle (bounds, 16.0f, 1.4f);

    g.setColour (red);
    g.fillRoundedRectangle (juce::Rectangle<float> (28.0f, 104.0f, (float) getWidth() - 56.0f, 2.0f), 1.0f);

    auto drawPanel = [&] (juce::Rectangle<float> r, const juce::String& heading, const juce::String& detail, juce::Colour accent)
    {
        g.setColour (juce::Colour (0xff0b1320));
        g.fillRoundedRectangle (r, 12.0f);
        g.setColour (panelEdge);
        g.drawRoundedRectangle (r, 12.0f, 1.0f);
        g.setColour (accent);
        g.fillRoundedRectangle (juce::Rectangle<float> (r.getX() + 14.0f, r.getY() + 14.0f, 3.0f, 22.0f), 1.5f);
        g.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::bold)));
        g.drawText (heading, (int) r.getX() + 26, (int) r.getY() + 12, (int) r.getWidth() - 40, 22, juce::Justification::centredLeft);
        g.setColour (juce::Colour (0xff8194aa));
        g.setFont (juce::Font (juce::FontOptions (10.5f)));
        g.drawText (detail, (int) r.getX() + 16, (int) r.getY() + 38, (int) r.getWidth() - 32, 18, juce::Justification::centredLeft);
    };

    const float y = 190.0f;
    const float h = (float) getHeight() - y - 44.0f;
    drawPanel ({ 28.0f, y, 330.0f, h }, "OSCILLATOR  /  PITCH MOD", "Independent sample & hold per voice", cyan);
    drawPanel ({ 374.0f, y, 350.0f, h }, "FILTER  /  STEREO", "Tone shaping and stereo placement", red);
    drawPanel ({ 740.0f, y, (float) getWidth() - 768.0f, h }, "AMP ENVELOPE", "Shape the note over time", cyan);

    g.setColour (red);
    g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    auto knobLabel = [&] (const juce::String& text, int x, int y, int width)
    {
        g.drawText (text, x, y, width, 16, juce::Justification::centred);
    };
    knobLabel ("S&H RANGE", 50, 250, 142);
    knobLabel ("PORTAMENTO", 208, 250, 142);
    knobLabel ("CUTOFF", 394, 250, 142);
    knobLabel ("RESONANCE", 562, 250, 142);
    knobLabel ("STEREO SPREAD", 478, 408, 142);
    const int envX = 760;
    const int envW = juce::jmax (126, (getWidth() - 790) / 2);
    knobLabel ("ATTACK", envX, 250, envW);
    knobLabel ("DECAY", envX + envW + 12, 250, envW);
    knobLabel ("SUSTAIN", envX, 408, envW);
    knobLabel ("RELEASE", envX + envW + 12, 408, envW);

    g.setColour (juce::Colour (0xff91a5ba));
    g.setFont (juce::Font (juce::FontOptions (10.0f)));
    g.drawText ("S&H pitch modulation follows the portamento state. Range is measured in cents.",
                44, getHeight() - 28, getWidth() - 88, 16, juce::Justification::centred);
}

void R2RZXAudioProcessorEditor::resized()
{
    const int w = getWidth();
    title.setBounds (30, 22, 300, 48);
    subtitle.setBounds (32, 68, 560, 22);
    presetLabel.setBounds (30, 118, 250, 20);
    presetBox.setBounds (30, 140, juce::jmin (570, w - 430), 32);
    oscLabel.setBounds (w - 330, 118, 180, 20);
    oscillatorBox.setBounds (w - 330, 140, 290, 32);

    const int top = 266;
    const int panel1x = 40, panel2x = 386, panel3x = 752;
    const int knobW = 142, knobH = 145;
    range.setBounds (panel1x + 10, top, knobW, knobH);
    portamento.setBounds (panel1x + 168, top, knobW, knobH);

    cutoff.setBounds (panel2x + 8, top, knobW, knobH);
    resonance.setBounds (panel2x + 176, top, knobW, knobH);
    width.setBounds (panel2x + 92, top + 158, knobW, knobH);

    const int envW = juce::jmax (126, (w - 790) / 2);
    attack.setBounds (panel3x + 8, top, envW, knobH);
    decay.setBounds (panel3x + envW + 12, top, envW, knobH);
    sustain.setBounds (panel3x + 8, top + 158, envW, knobH);
    release.setBounds (panel3x + envW + 12, top + 158, envW, knobH);
}
