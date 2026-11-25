#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
class R2RZXVoice final : public juce::SynthesiserVoice
{
public:
    bool canPlaySound (juce::SynthesiserSound* sound) override { return dynamic_cast<juce::SynthesiserSound*> (sound) != nullptr; }

    void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int) override
    {
        level = velocity;
        noteHz = juce::MidiMessage::getMidiNoteInHertz (midiNote);
        targetHz = noteHz;
        phase = 0.0;
        shCounter = 0;
        shValue = random.nextFloat() * 2.0f - 1.0f;
        pitchEnv = 0.0f;
        env = 0.0f;
        releasing = false;
        active = true;
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff) releasing = true;
        else { active = false; clearCurrentNote(); }
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& output, int start, int count) override
    {
        if (! active) return;
        auto* left = output.getWritePointer (0);
        auto* right = output.getNumChannels() > 1 ? output.getWritePointer (1) : nullptr;
        const double sr = juce::jmax (1.0, getSampleRate());
        const float attack = 0.015f, release = 0.18f;
        const float range = 100.0f;
        for (int i = 0; i < count; ++i)
        {
            env = releasing ? juce::jmax (0.0f, env - 1.0f / (release * (float) sr))
                            : juce::jmin (1.0f, env + 1.0f / (attack * (float) sr));
            if (releasing && env <= 0.0001f) { active = false; clearCurrentNote(); break; }

            // Each voice owns its own S&H clock and random value: no shared pitch randomisation.
            if (++shCounter >= (int) (sr * 0.075))
            {
                shCounter = 0;
                shValue = random.nextFloat() * 2.0f - 1.0f;
            }
            const float cents = shValue * range * env;
            const double hz = noteHz * std::pow (2.0, cents / 1200.0);
            const double increment = juce::MathConstants<double>::twoPi * hz / sr;
            phase += increment;
            if (phase >= juce::MathConstants<double>::twoPi) phase -= juce::MathConstants<double>::twoPi;
            const float sample = (float) std::sin (phase) * level * env * 0.18f;
            left[start + i] += sample;
            if (right != nullptr) right[start + i] += sample;
        }
    }

private:
    double phase = 0.0, noteHz = 440.0, targetHz = 440.0;
    float level = 0.0f, env = 0.0f, pitchEnv = 0.0f, shValue = 0.0f;
    int shCounter = 0;
    bool active = false, releasing = false;
    juce::Random random;
};

class R2RZXSound final : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};
}

R2RZXAudioProcessor::APVTS::ParameterLayout R2RZXAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("SH_RANGE", "S&H Range", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 25.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("PORTAMENTO", "Portamento", juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("ATTACK", "Attack", juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f), 0.015f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("DECAY", "Decay", juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f), 0.25f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("SUSTAIN", "Sustain", juce::NormalisableRange<float> (0.0f, 1.0f), 0.7f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("RELEASE", "Release", juce::NormalisableRange<float> (0.001f, 8.0f, 0.001f), 0.18f));
    return { p.begin(), p.end() };
}

R2RZXAudioProcessor::R2RZXAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "R2RZX", createParameterLayout())
{
    for (int i = 0; i < 16; ++i) synth.addVoice (new R2RZXVoice());
    synth.addSound (new R2RZXSound());
}

void R2RZXAudioProcessor::prepareToPlay (double sr, int)
{
    synth.setCurrentPlaybackSampleRate (sr);
    synth.allNotesOff (0, false);
}
void R2RZXAudioProcessor::releaseResources() {}
void R2RZXAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    synth.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());
}
juce::AudioProcessorEditor* R2RZXAudioProcessor::createEditor() { return new R2RZXAudioProcessorEditor (*this); }
const juce::String R2RZXAudioProcessor::getProgramName (int index) { return "R2R ZX Preset " + juce::String (index + 1); }
void R2RZXAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, dest);
}
void R2RZXAudioProcessor::setStateInformation (const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, size));
    if (xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new R2RZXAudioProcessor(); }
