#include "PluginEditor.h"

R2RZXAudioProcessorEditor::R2RZXAudioProcessorEditor (R2RZXAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (940, 700);
    title.setText ("R2R-ZX", juce::dontSendNotification);
    title.setFont (juce::Font (juce::FontOptions (36.0f, juce::Font::bold)));
    title.setColour (juce::Label::textColourId, juce::Colour (0xff00eaff));
    addAndMakeVisible (title);
    subtitle.setText ("POLYPHONIC SYNTH  /  PER-VOICE S&H PITCH ENGINE", juce::dontSendNotification);
    subtitle.setColour (juce::Label::textColourId, juce::Colour (0xffff405d));
    addAndMakeVisible (subtitle);

    presetLabel.setText ("FACTORY PRESET  /  100 SOUNDS", juce::dontSendNotification);
    presetLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (presetLabel);
    for (int i = 0; i < R2RZXFactoryPresetCount; ++i)
    {
        const auto& pinfo = R2RZXFactoryPresets[i];
        presetBox.addItem (juce::String (i + 1).paddedLeft ('0', 3) + "  " + pinfo.name + "  [" + pinfo.category + "]", i + 1);
    }
    presetBox.setSelectedId (1, juce::dontSendNotification);
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id > 0) processor.selectPreset (id - 1);
        repaint();
    };
    presetBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff101a29));
    presetBox.setColour (juce::ComboBox::textColourId, juce::Colours::white);
    presetBox.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff00dff2));
    addAndMakeVisible (presetBox);

    oscLabel.setText ("OSCILLATOR", juce::dontSendNotification);
    oscLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (oscLabel);
    oscillatorBox.addItem ("Sine", 1); oscillatorBox.addItem ("Saw", 2);
    oscillatorBox.addItem ("Pulse", 3); oscillatorBox.addItem ("Triangle", 4);
    oscillatorBox.addItem ("Digital", 5);
    oscillatorBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff101a29));
    oscillatorBox.setColour (juce::ComboBox::textColourId, juce::Colours::white);
    oscillatorBox.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xffff405d));
    addAndMakeVisible (oscillatorBox);

    setupSlider (range, "S&H RANGE");
    setupSlider (portamento, "PORTAMENTO");
    setupSlider (attack, "ATTACK");
    setupSlider (decay, "DECAY");
    setupSlider (sustain, "SUSTAIN");
    setupSlider (release, "RELEASE");
    setupSlider (cutoff, "FILTER CUTOFF");
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
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 20);
    s.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff00dff2));
    s.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff25374a));
    s.setColour (juce::Slider::thumbColourId, juce::Colour (0xffff405d));
    s.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    s.setName (name);
    addAndMakeVisible (s);
}
void R2RZXAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff050811));
    auto b = getLocalBounds().toFloat().reduced (16.0f);
    g.setColour (juce::Colour (0xff101a29)); g.fillRoundedRectangle (b, 18.0f);
    g.setColour (juce::Colour (0xff00dff2)); g.drawRoundedRectangle (b, 18.0f, 1.5f);
    g.setColour (juce::Colour (0x4439dfff));
    for (int i = 1; i < 6; ++i) g.drawRoundedRectangle (b.reduced ((float) i * 2.0f), 18.0f, 0.5f);
    g.setColour (juce::Colour (0xffff405d)); g.drawLine (28.0f, 112.0f, (float) getWidth() - 28.0f, 112.0f, 1.2f);
    g.setColour (juce::Colour (0xff00dff2)); g.setFont (13.0f);
    g.drawText ("INDEPENDENT RANDOM PITCH PER NOTE  |  S&H RANGE: 0–100 CENTS  |  24 VOICES", 30, 174, getWidth() - 60, 22, juce::Justification::centredLeft);
    const juce::StringArray names { "S&H RANGE", "PORTAMENTO", "ATTACK", "DECAY", "SUSTAIN", "RELEASE", "FILTER CUTOFF", "RESONANCE", "STEREO WIDTH" };
    g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    for (int i = 0; i < names.size(); ++i)
    {
        const int col = i % 3, row = i / 3;
        g.setColour (juce::Colour (0xffff405d));
        g.drawText (names[i], 28 + col * 290, 203 + row * 148, 270, 18, juce::Justification::centred);
    }
    const int presetId = juce::jlimit (1, R2RZXFactoryPresetCount, presetBox.getSelectedId());
    g.setColour (juce::Colours::white);
    g.drawText ("LOADED: " + juce::String (R2RZXFactoryPresets[presetId - 1].category), 620, 169, 250, 18, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xffff405d)); g.setFont (12.0f);
    g.drawText ("R2R AUDIO LAB  •  FACTORY BANK: 100 PRESETS", 30, getHeight() - 24, getWidth() - 60, 16, juce::Justification::centred);
}
void R2RZXAudioProcessorEditor::resized()
{
    title.setBounds (30, 20, 300, 50);
    subtitle.setBounds (32, 72, 600, 24);
    presetLabel.setBounds (30, 119, 240, 22);
    presetBox.setBounds (30, 139, 560, 30);
    oscLabel.setBounds (620, 119, 180, 22);
    oscillatorBox.setBounds (620, 139, 250, 30);

    juce::Slider* controls[] = { &range, &portamento, &attack, &decay, &sustain, &release, &cutoff, &resonance, &width };
    const int cellW = 270, cellH = 140, x0 = 28, y0 = 220, gapX = 20, gapY = 8;
    for (int i = 0; i < 9; ++i)
    {
        const int col = i % 3, row = i / 3;
        controls[i]->setBounds (x0 + col * (cellW + gapX), y0 + row * (cellH + gapY), cellW, cellH);
    }
}
