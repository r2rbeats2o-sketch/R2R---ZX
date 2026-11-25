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
    juce::Slider range, attack, decay, sustain, release;
    juce::Label title, subtitle;
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> rangeAttachment, attackAttachment, decayAttachment, sustainAttachment, releaseAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (R2RZXAudioProcessorEditor)
};
