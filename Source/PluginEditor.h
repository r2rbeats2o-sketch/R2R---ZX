#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class R2RZXLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
};

class R2RZXAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit R2RZXAudioProcessorEditor (R2RZXAudioProcessor&);
    ~R2RZXAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    R2RZXAudioProcessor& processor;
    R2RZXLookAndFeel r2rLookAndFeel;
    juce::ComboBox presetBox, oscillatorBox, wavetableBox;
    juce::Slider range, portamento, attack, decay, sustain, release, cutoff, resonance, width, wtPosition, oscMix, detune, unison, unisonDetune, subMix, drive, filterEnv;
    juce::Label title, subtitle, presetLabel, oscLabel, wavetableLabel;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SliderAttachment> rangeAttachment, portamentoAttachment, attackAttachment, decayAttachment,
        sustainAttachment, releaseAttachment, cutoffAttachment, resonanceAttachment, widthAttachment, wtPositionAttachment, oscMixAttachment, detuneAttachment,
        unisonAttachment, unisonDetuneAttachment, subMixAttachment, driveAttachment, filterEnvAttachment;
    std::unique_ptr<ComboAttachment> oscillatorAttachment, wavetableAttachment;
    void setupSlider (juce::Slider&, const juce::String&);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (R2RZXAudioProcessorEditor)
};
