#include "PluginEditor.h"
#include <cmath>

namespace
{
const juce::Colour bgTop (0xff15171c);
const juce::Colour bgBottom (0xff050609);
const juce::Colour panelTop (0xff282a30);
const juce::Colour panelBottom (0xff0b0c10);
const juce::Colour edgeLight (0xff555861);
const juce::Colour edgeDark (0xff030406);
const juce::Colour red (0xffff263d);
const juce::Colour redDeep (0xff8f1024);
const juce::Colour redGlow (0xffff1736);
const juce::Colour white (0xfff4f1f2);
const juce::Colour muted (0xff96949a);

void drawRaisedPanel (juce::Graphics& g, juce::Rectangle<float> r,
                      const juce::String& heading, const juce::String& detail,
                      juce::Colour accent)
{
    auto shadow = r.translated (0.0f, 7.0f);
    g.setColour (juce::Colour (0xff000000).withAlpha (0.72f));
    g.fillRoundedRectangle (shadow, 13.0f);

    juce::ColourGradient body (panelTop, r.getX(), r.getY(),
                               panelBottom, r.getX(), r.getBottom(), false);
    g.setGradientFill (body);
    g.fillRoundedRectangle (r, 13.0f);

    g.setColour (edgeLight.withAlpha (0.46f));
    g.drawRoundedRectangle (r.reduced (0.7f), 13.0f, 1.0f);
    g.setColour (edgeDark);
    g.drawRoundedRectangle (r.reduced (2.4f), 11.0f, 1.0f);

    auto topLip = juce::Rectangle<float> (r.getX() + 13.0f, r.getY() + 9.0f, r.getWidth() - 26.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.fillRoundedRectangle (topLip, 0.5f);

    g.setColour (accent.withAlpha (0.95f));
    g.fillRoundedRectangle ({ r.getX() + 14.0f, r.getY() + 16.0f, 3.0f, 19.0f }, 1.5f);
    g.setColour (white);
    g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    g.drawText (heading, (int) r.getX() + 27, (int) r.getY() + 11,
                (int) r.getWidth() - 42, 21, juce::Justification::centredLeft);
    g.setColour (muted);
    g.setFont (juce::Font (juce::FontOptions (9.5f)));
    g.drawText (detail, (int) r.getX() + 16, (int) r.getY() + 35,
                (int) r.getWidth() - 32, 16, juce::Justification::centredLeft);
}

void drawWaveDisplay (juce::Graphics& g, juce::Rectangle<float> r,
                      const juce::String& label, float phaseOffset,
                      float shapeMix, float amplitude)
{
    g.setColour (juce::Colour (0xff07080b));
    g.fillRoundedRectangle (r.translated (0.0f, 3.0f), 7.0f);
    juce::ColourGradient glass (juce::Colour (0xff15171d), r.getX(), r.getY(),
                                juce::Colour (0xff08090d), r.getX(), r.getBottom(), false);
    g.setGradientFill (glass);
    g.fillRoundedRectangle (r, 7.0f);
    g.setColour (juce::Colour (0xff55515a).withAlpha (0.65f));
    g.drawRoundedRectangle (r, 7.0f, 1.0f);

    g.saveState();
    g.reduceClipRegion (r.toNearestInt().reduced (5));
    g.setColour (juce::Colour (0xff6d1725).withAlpha (0.42f));
    for (int i = 1; i < 5; ++i)
    {
        const float x = r.getX() + r.getWidth() * (float) i / 5.0f;
        g.drawVerticalLine ((int) x, r.getY() + 8.0f, r.getBottom() - 8.0f);
    }
    for (int i = 1; i < 4; ++i)
    {
        const float y = r.getY() + r.getHeight() * (float) i / 4.0f;
        g.drawHorizontalLine ((int) y, r.getX() + 7.0f, r.getRight() - 7.0f);
    }

    juce::Path wave;
    const float mid = r.getCentreY() + 5.0f;
    const float amp = r.getHeight() * 0.31f * amplitude;
    for (int i = 0; i <= 220; ++i)
    {
        const float t = (float) i / 220.0f;
        const float p = t * juce::MathConstants<float>::twoPi;
        const float sine = std::sin (p + phaseOffset);
        const float harmonic = 0.56f * std::sin (2.0f * p + phaseOffset * 0.6f)
                             + 0.24f * std::sin (3.0f * p - phaseOffset);
        const float saw = 2.0f * t - 1.0f;
        const float yValue = sine * (1.0f - shapeMix) + harmonic * shapeMix * 0.65f + saw * shapeMix * 0.25f;
        const float x = r.getX() + 8.0f + t * (r.getWidth() - 16.0f);
        const float y = mid - yValue * amp;
        if (i == 0) wave.startNewSubPath (x, y);
        else wave.lineTo (x, y);
    }

    g.setColour (redGlow.withAlpha (0.16f));
    g.strokePath (wave, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (juce::Colour (0xffff6675));
    g.strokePath (wave, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.restoreState();

    g.setColour (juce::Colour (0xffff7a88));
    g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
    g.drawText (label, (int) r.getX() + 9, (int) r.getY() + 5, (int) r.getWidth() - 18, 14,
                juce::Justification::centredLeft);
}
}

void R2RZXLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                         juce::Slider&)
{
    const float diameter = (float) juce::jmin (width, height) - 17.0f;
    const float cx = (float) x + (float) width * 0.5f;
    const float cy = (float) y + (float) height * 0.46f;
    auto knob = juce::Rectangle<float> (cx - diameter * 0.5f, cy - diameter * 0.5f, diameter, diameter);

    // Soft cast shadow gives the control a raised, physical profile.
    g.setColour (juce::Colours::black.withAlpha (0.78f));
    g.fillEllipse (knob.translated (0.0f, 5.0f).expanded (3.0f));
    g.setColour (redDeep.withAlpha (0.16f));
    g.fillEllipse (knob.expanded (7.0f));

    juce::ColourGradient outer (juce::Colour (0xff777a82), knob.getX(), knob.getY(),
                                juce::Colour (0xff17191e), knob.getRight(), knob.getBottom(), false);
    g.setGradientFill (outer);
    g.fillEllipse (knob.expanded (2.5f));
    g.setColour (juce::Colour (0xff030406));
    g.drawEllipse (knob.expanded (2.5f), 1.2f);

    auto face = knob.reduced (5.0f);
    juce::ColourGradient faceGradient (juce::Colour (0xff45474e), face.getX(), face.getY(),
                                       juce::Colour (0xff101115), face.getRight(), face.getBottom(), false);
    g.setGradientFill (faceGradient);
    g.fillEllipse (face);
    g.setColour (juce::Colour (0xff8a8c93).withAlpha (0.62f));
    g.drawEllipse (face, 1.0f);

    auto inner = face.reduced (7.0f);
    juce::ColourGradient innerGradient (juce::Colour (0xff24262c), inner.getX(), inner.getY(),
                                        juce::Colour (0xff08090c), inner.getRight(), inner.getBottom(), false);
    g.setGradientFill (innerGradient);
    g.fillEllipse (inner);

    const float start = rotaryStartAngle;
    const float end = rotaryEndAngle;
    const float angle = start + sliderPos * (end - start);
    const float radius = diameter * 0.5f + 1.0f;
    juce::Path track;
    track.addCentredArc (cx, cy, radius, radius, 0.0f, start, end, true);
    g.setColour (juce::Colour (0xff08090c));
    g.strokePath (track, juce::PathStrokeType (4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path active;
    active.addCentredArc (cx, cy, radius, radius, 0.0f, start, angle, true);
    g.setColour (redGlow.withAlpha (0.20f));
    g.strokePath (active, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (red);
    g.strokePath (active, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float tipX = cx + std::sin (angle) * diameter * 0.37f;
    const float tipY = cy - std::cos (angle) * diameter * 0.37f;
    juce::Path pointer;
    pointer.startNewSubPath (cx + std::sin (angle) * diameter * 0.12f,
                             cy - std::cos (angle) * diameter * 0.12f);
    pointer.lineTo (tipX, tipY);
    g.setColour (juce::Colour (0xfff8dfe2));
    g.strokePath (pointer, juce::PathStrokeType (3.2f));
    g.setColour (red);
    g.fillEllipse (tipX - 2.0f, tipY - 2.0f, 4.0f, 4.0f);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.fillEllipse (face.getX() + diameter * 0.18f, face.getY() + diameter * 0.12f,
                   diameter * 0.20f, diameter * 0.075f);
}

void R2RZXLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                                               int buttonX, int buttonY, int buttonW, int buttonH,
                                               juce::ComboBox&)
{
    auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);
    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.fillRoundedRectangle (bounds.translated (0.0f, 2.0f), 6.0f);
    juce::ColourGradient fill (juce::Colour (0xff303137), 0.0f, 0.0f,
                               juce::Colour (0xff101115), 0.0f, (float) height, false);
    g.setGradientFill (fill);
    g.fillRoundedRectangle (bounds.reduced (0.5f), 6.0f);
    g.setColour (juce::Colour (0xff64616a));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);
    g.setColour (red.withAlpha (0.85f));
    g.fillRoundedRectangle ({ 5.0f, 6.0f, 2.0f, (float) height - 12.0f }, 1.0f);

    const float cx = (float) buttonX + (float) buttonW * 0.5f;
    const float cy = (float) buttonY + (float) buttonH * 0.5f;
    juce::Path arrow;
    arrow.startNewSubPath (cx - 4.5f, cy - 2.0f);
    arrow.lineTo (cx, cy + 2.5f);
    arrow.lineTo (cx + 4.5f, cy - 2.0f);
    g.setColour (isButtonDown ? juce::Colours::white : red);
    g.strokePath (arrow, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
}

R2RZXAudioProcessorEditor::R2RZXAudioProcessorEditor (R2RZXAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&r2rLookAndFeel);
    setSize (1360, 880);
    setResizable (true, true);
    setResizeLimits (1120, 760, 1760, 1120);

    title.setText ("R2R  /  ZX", juce::dontSendNotification);
    title.setFont (juce::Font (juce::FontOptions (35.0f, juce::Font::bold)));
    title.setColour (juce::Label::textColourId, red);
    addAndMakeVisible (title);

    subtitle.setText ("ADVANCED WAVETABLE SYNTHESIZER     /     DUAL OSCILLATOR ENGINE", juce::dontSendNotification);
    subtitle.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    subtitle.setColour (juce::Label::textColourId, muted);
    addAndMakeVisible (subtitle);

    presetLabel.setText ("PRESET LIBRARY  /  100 FACTORY SOUNDS", juce::dontSendNotification);
    presetLabel.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    presetLabel.setColour (juce::Label::textColourId, muted);
    addAndMakeVisible (presetLabel);

    for (int i = 0; i < R2RZXFactoryPresetCount; ++i)
    {
        const auto& info = R2RZXFactoryPresets[i];
        presetBox.addItem (juce::String (i + 1).paddedLeft ('0', 3) + "   " + info.name + "   /   " + info.category, i + 1);
    }
    presetBox.setSelectedId (1, juce::dontSendNotification);
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id > 0) processor.selectPreset (id - 1);
        repaint();
    };
    presetBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff111216));
    presetBox.setColour (juce::ComboBox::textColourId, white);
    presetBox.setColour (juce::ComboBox::outlineColourId, redDeep);
    presetBox.setColour (juce::ComboBox::arrowColourId, red);
    presetBox.setColour (juce::ComboBox::focusedOutlineColourId, red);
    addAndMakeVisible (presetBox);

    oscLabel.setText ("OSCILLATOR SHAPE", juce::dontSendNotification);
    oscLabel.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    oscLabel.setColour (juce::Label::textColourId, muted);
    addAndMakeVisible (oscLabel);
    oscillatorBox.addItem ("Sine", 1);
    oscillatorBox.addItem ("Saw", 2);
    oscillatorBox.addItem ("Pulse", 3);
    oscillatorBox.addItem ("Triangle", 4);
    oscillatorBox.addItem ("Digital", 5);
    oscillatorBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff111216));
    oscillatorBox.setColour (juce::ComboBox::textColourId, white);
    oscillatorBox.setColour (juce::ComboBox::outlineColourId, redDeep);
    oscillatorBox.setColour (juce::ComboBox::arrowColourId, red);
    oscillatorBox.setColour (juce::ComboBox::focusedOutlineColourId, red);
    addAndMakeVisible (oscillatorBox);

    wavetableLabel.setText ("WAVETABLE BANK", juce::dontSendNotification);
    wavetableLabel.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    wavetableLabel.setColour (juce::Label::textColourId, muted);
    addAndMakeVisible (wavetableLabel);
    wavetableBox.addItem ("Basic Shapes", 1);
    wavetableBox.addItem ("Formant", 2);
    wavetableBox.addItem ("Digital Glass", 3);
    wavetableBox.addItem ("Vocal Grain", 4);
    wavetableBox.addItem ("Harmonic Sweep", 5);
    wavetableBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff111216));
    wavetableBox.setColour (juce::ComboBox::textColourId, white);
    wavetableBox.setColour (juce::ComboBox::outlineColourId, redDeep);
    wavetableBox.setColour (juce::ComboBox::arrowColourId, red);
    wavetableBox.setColour (juce::ComboBox::focusedOutlineColourId, red);
    addAndMakeVisible (wavetableBox);

    setupSlider (range, "S&H RANGE");
    setupSlider (portamento, "PORTAMENTO");
    setupSlider (attack, "ATTACK");
    setupSlider (decay, "DECAY");
    setupSlider (sustain, "SUSTAIN");
    setupSlider (release, "RELEASE");
    setupSlider (cutoff, "CUTOFF");
    setupSlider (resonance, "RESONANCE");
    setupSlider (width, "STEREO WIDTH");
    setupSlider (wtPosition, "WT POSITION");
    setupSlider (oscMix, "OSC B MIX");
    setupSlider (detune, "B DETUNE");

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
    wtPositionAttachment = std::make_unique<SliderAttachment> (state, "WT_POS", wtPosition);
    oscMixAttachment = std::make_unique<SliderAttachment> (state, "OSC_MIX", oscMix);
    detuneAttachment = std::make_unique<SliderAttachment> (state, "DETUNE", detune);
    oscillatorAttachment = std::make_unique<ComboAttachment> (state, "OSC_TYPE", oscillatorBox);
    wavetableAttachment = std::make_unique<ComboAttachment> (state, "WT_BANK", wavetableBox);

    auto repaintOnEdit = [this] { repaint(); };
    wtPosition.onValueChange = repaintOnEdit;
    wavetableBox.onChange = repaintOnEdit;
    oscMix.onValueChange = repaintOnEdit;
    detune.onValueChange = repaintOnEdit;
    cutoff.onValueChange = repaintOnEdit;
    resonance.onValueChange = repaintOnEdit;
    width.onValueChange = repaintOnEdit;
}

R2RZXAudioProcessorEditor::~R2RZXAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void R2RZXAudioProcessorEditor::setupSlider (juce::Slider& s, const juce::String& name)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 68, 18);
    s.setColour (juce::Slider::textBoxTextColourId, white);
    s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff090a0e));
    s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff4c151f));
    s.setColour (juce::Slider::thumbColourId, red);
    s.setName (name);
    addAndMakeVisible (s);
}

void R2RZXAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient background (bgTop, 0.0f, 0.0f, bgBottom, 0.0f, (float) getHeight(), false);
    g.setGradientFill (background);
    g.fillAll();

    // Fine diagonal metal grain and outer chassis.
    g.setColour (juce::Colours::white.withAlpha (0.018f));
    for (int x = -getHeight(); x < getWidth(); x += 26)
        g.drawLine ((float) x, 0.0f, (float) x + (float) getHeight(), (float) getHeight(), 0.45f);

    auto chassis = getLocalBounds().toFloat().reduced (11.0f);
    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.fillRoundedRectangle (chassis.translated (0.0f, 5.0f), 19.0f);
    juce::ColourGradient metal (juce::Colour (0xff3b3d44), chassis.getX(), chassis.getY(),
                                juce::Colour (0xff090a0d), chassis.getRight(), chassis.getBottom(), false);
    g.setGradientFill (metal);
    g.fillRoundedRectangle (chassis, 19.0f);
    g.setColour (juce::Colour (0xff777981).withAlpha (0.45f));
    g.drawRoundedRectangle (chassis, 19.0f, 1.2f);
    g.setColour (red.withAlpha (0.75f));
    g.fillRoundedRectangle ({ 27.0f, 94.0f, (float) getWidth() - 54.0f, 2.0f }, 1.0f);

    const float margin = 24.0f;
    const float usable = (float) getWidth() - margin * 2.0f;
    const float topY = 194.0f;
    const float topH = 278.0f;
    const float leftW = usable * 0.64f;
    const float rightX = margin + leftW + 14.0f;
    const float rightW = usable - leftW - 14.0f;
    const float bottomY = 488.0f;
    const float bottomH = (float) getHeight() - bottomY - 24.0f;
    const float filterW = (usable - 14.0f) * 0.39f;
    const float envX = margin + filterW + 14.0f;
    const float envW = usable - filterW - 14.0f;

    drawRaisedPanel (g, { margin, topY, leftW, topH }, "DUAL WAVETABLE OSCILLATORS",
                     "FRAME SCAN  /  CROSSFADE  /  PHASE OFFSET", red);
    drawRaisedPanel (g, { rightX, topY, rightW, topH }, "VOICE / MOVEMENT",
                     "PER-VOICE PERFORMANCE CONTROLS", red);
    drawRaisedPanel (g, { margin, bottomY, filterW, bottomH }, "FILTER / STEREO STAGE",
                     "SHAPE THE TONE AND SPACE", red);
    drawRaisedPanel (g, { envX, bottomY, envW, bottomH }, "AMPLITUDE ENVELOPE",
                     "DYNAMIC SHAPING  /  ADSR", red);

    const float waveY = topY + 61.0f;
    const float waveGap = 12.0f;
    const float waveW = (leftW - 54.0f) * 0.5f;
    const float scan = (float) wtPosition.getValue();
    const float blend = (float) oscMix.getValue();
    drawWaveDisplay (g, { margin + 17.0f, waveY, waveW, 91.0f }, "OSC A  /  WAVETABLE",
                     0.0f, scan, 0.90f);
    drawWaveDisplay (g, { margin + 17.0f + waveW + waveGap, waveY, waveW, 91.0f }, "OSC B  /  DETUNED",
                     0.7f, blend, 0.78f);

    auto label = [&] (const juce::String& text, float x, float y, float w)
    {
        g.setColour (red.withAlpha (0.94f));
        g.setFont (juce::Font (juce::FontOptions (9.2f, juce::Font::bold)));
        g.drawText (text, (int) x, (int) y, (int) w, 15, juce::Justification::centred);
    };

    const float oscKnobW = juce::jmin (116.0f, (leftW - 58.0f) / 3.0f);
    label ("WT POSITION", margin + 20.0f, topY + 151.0f, oscKnobW);
    label ("OSC B MIX", margin + leftW * 0.5f - oscKnobW * 0.5f, topY + 151.0f, oscKnobW);
    label ("B DETUNE", margin + leftW - oscKnobW - 20.0f, topY + 151.0f, oscKnobW);

    const float voiceKnobW = juce::jmin (138.0f, (rightW - 42.0f) * 0.5f);
    label ("S&H RANGE", rightX + 12.0f, topY + 84.0f, voiceKnobW);
    label ("PORTAMENTO", rightX + rightW - voiceKnobW - 12.0f, topY + 84.0f, voiceKnobW);

    const float filterKnobW = juce::jmin (124.0f, (filterW - 28.0f) / 3.0f);
    label ("CUTOFF", margin + 8.0f, bottomY + 47.0f, filterKnobW);
    label ("RESONANCE", margin + filterW * 0.5f - filterKnobW * 0.5f, bottomY + 47.0f, filterKnobW);
    label ("STEREO WIDTH", margin + filterW - filterKnobW - 8.0f, bottomY + 47.0f, filterKnobW);

    const float envKnobW = juce::jmin (146.0f, (envW - 52.0f) / 4.0f);
    const float envGap = (envW - envKnobW * 4.0f) / 5.0f;
    label ("ATTACK", envX + envGap, bottomY + 47.0f, envKnobW);
    label ("DECAY", envX + envGap * 2.0f + envKnobW, bottomY + 47.0f, envKnobW);
    label ("SUSTAIN", envX + envGap * 3.0f + envKnobW * 2.0f, bottomY + 47.0f, envKnobW);
    label ("RELEASE", envX + envGap * 4.0f + envKnobW * 3.0f, bottomY + 47.0f, envKnobW);

    g.setColour (muted.withAlpha (0.8f));
    g.setFont (juce::Font (juce::FontOptions (9.0f)));
    g.drawText ("R2R AUDIO LAB  /  24 VOICE POLYPHONY  /  BLACK METAL SERIES",
                30, getHeight() - 17, getWidth() - 60, 12, juce::Justification::centred);
}

void R2RZXAudioProcessorEditor::resized()
{
    const float margin = 24.0f;
    const float usable = (float) getWidth() - margin * 2.0f;
    const float topY = 194.0f;
    const float leftW = usable * 0.64f;
    const float rightX = margin + leftW + 14.0f;
    const float rightW = usable - leftW - 14.0f;
    const float bottomY = 488.0f;
    const float filterW = (usable - 14.0f) * 0.39f;
    const float envX = margin + filterW + 14.0f;
    const float envW = usable - filterW - 14.0f;

    title.setBounds (30, 19, 300, 48);
    subtitle.setBounds (32, 65, 650, 20);
    presetLabel.setBounds (30, 108, 310, 18);
    presetBox.setBounds (30, 130, 520, 38);
    oscLabel.setBounds (570, 108, 165, 18);
    oscillatorBox.setBounds (570, 130, 165, 38);
    wavetableLabel.setBounds (755, 108, 210, 18);
    wavetableBox.setBounds (755, 130, 210, 38);

    const float oscKnobW = juce::jmin (116.0f, (leftW - 58.0f) / 3.0f);
    const float oscY = topY + 165.0f;
    wtPosition.setBounds ((int) (margin + 20.0f + (oscKnobW - 108.0f) * 0.5f), (int) oscY, 108, 112);
    oscMix.setBounds ((int) (margin + leftW * 0.5f - 54.0f), (int) oscY, 108, 112);
    detune.setBounds ((int) (margin + leftW - oscKnobW - 20.0f + (oscKnobW - 108.0f) * 0.5f), (int) oscY, 108, 112);

    const float voiceW = juce::jmin (138.0f, (rightW - 42.0f) * 0.5f);
    const float voiceY = topY + 108.0f;
    range.setBounds ((int) (rightX + 12.0f + (voiceW - 122.0f) * 0.5f), (int) voiceY, 122, 154);
    portamento.setBounds ((int) (rightX + rightW - voiceW - 12.0f + (voiceW - 122.0f) * 0.5f), (int) voiceY, 122, 154);

    const float bottomH = (float) getHeight() - bottomY - 24.0f;
    const float filterKnobW = juce::jmin (124.0f, (filterW - 28.0f) / 3.0f);
    const float filterY = bottomY + 69.0f;
    const float filterSliderW = juce::jmin (118.0f, filterKnobW);
    cutoff.setBounds ((int) (margin + 8.0f + (filterKnobW - filterSliderW) * 0.5f), (int) filterY, (int) filterSliderW, (int) juce::jmin (158.0f, bottomH - 76.0f));
    resonance.setBounds ((int) (margin + filterW * 0.5f - filterSliderW * 0.5f), (int) filterY, (int) filterSliderW, (int) juce::jmin (158.0f, bottomH - 76.0f));
    width.setBounds ((int) (margin + filterW - filterKnobW - 8.0f + (filterKnobW - filterSliderW) * 0.5f), (int) filterY, (int) filterSliderW, (int) juce::jmin (158.0f, bottomH - 76.0f));

    const float envKnobW = juce::jmin (146.0f, (envW - 52.0f) / 4.0f);
    const float envGap = (envW - envKnobW * 4.0f) / 5.0f;
    const float envY = bottomY + 69.0f;
    const int envH = (int) juce::jmin (158.0f, bottomH - 76.0f);
    attack.setBounds ((int) (envX + envGap), (int) envY, (int) envKnobW, envH);
    decay.setBounds ((int) (envX + envGap * 2.0f + envKnobW), (int) envY, (int) envKnobW, envH);
    sustain.setBounds ((int) (envX + envGap * 3.0f + envKnobW * 2.0f), (int) envY, (int) envKnobW, envH);
    release.setBounds ((int) (envX + envGap * 4.0f + envKnobW * 3.0f), (int) envY, (int) envKnobW, envH);
}
