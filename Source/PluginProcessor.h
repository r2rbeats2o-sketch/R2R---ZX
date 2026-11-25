#pragma once
#include <JuceHeader.h>
#include "FactoryPresets.h"

class R2RZXAudioProcessor final : public juce::AudioProcessor
{
public:
    R2RZXAudioProcessor();
    ~R2RZXAudioProcessor() override = default;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "R2R-ZX"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }
    int getNumPrograms() override { return R2RZXFactoryPresetCount; }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;
    void selectPreset (int index);
    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS parameters;
private:
    juce::Synthesiser synth;
    int currentProgram = 0;
    static APVTS::ParameterLayout createParameterLayout();
    void setParameter (const juce::String& id, float value);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (R2RZXAudioProcessor)
};
