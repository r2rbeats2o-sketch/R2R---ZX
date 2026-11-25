#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class R2RZXAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit R2RZXAudioProcessorEditor (R2RZXAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    R2RZXAudioProcessor& processor;
    juce::ComboBox presetBox, oscillatorBox;
    juce::Slider range, portamento, attack, decay, sustain, release, cutoff, resonance, width;
    juce::Label title, subtitle, presetLabel, oscLabel;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SliderAttachment> rangeAttachment, portamentoAttachment, attackAttachment, decayAttachment,
        sustainAttachment, releaseAttachment, cutoffAttachment, resonanceAttachment, widthAttachment;
    std::unique_ptr<ComboAttachment> oscillatorAttachment;
    void setupSlider (juce::Slider&, const juce::String&);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (R2RZXAudioProcessorEditor)
};
