#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
class R2RZXVoice final : public juce::SynthesiserVoice
{
public:
    explicit R2RZXVoice (R2RZXAudioProcessor::APVTS& apvts)
    {
        auto raw = [&] (const char* id) { return apvts.getRawParameterValue (id); };
        shRange = raw ("SH_RANGE"); portamento = raw ("PORTAMENTO");
        attack = raw ("ATTACK"); decay = raw ("DECAY"); sustain = raw ("SUSTAIN"); release = raw ("RELEASE");
        oscType = raw ("OSC_TYPE"); cutoff = raw ("CUTOFF"); resonance = raw ("RESONANCE"); width = raw ("WIDTH");
    }

    bool canPlaySound (juce::SynthesiserSound* sound) override { return sound != nullptr; }
    void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int) override
    {
        const double nextHz = juce::MidiMessage::getMidiNoteInHertz (midiNote);
        targetHz = nextHz;
        if (currentHz < 1.0) currentHz = nextHz;
        glideSeconds = portamento->load();
        glideStep = glideSeconds > 0.0001 ? (targetHz - currentHz) / juce::jmax (1.0, glideSeconds * getSampleRate()) : 0.0;
        level = velocity;
        juce::ADSR::Parameters ep;
        ep.attack = attack->load(); ep.decay = decay->load(); ep.sustain = sustain->load(); ep.release = release->load();
        ampEnv.setSampleRate (getSampleRate()); ampEnv.setParameters (ep); ampEnv.noteOn();
        pitchEnv.setSampleRate (getSampleRate()); pitchEnv.setParameters (ep); pitchEnv.noteOn();
        shCounter = 0; shValue = random.nextFloat() * 2.0f - 1.0f;
        phase = 0.0; filterState = 0.0f; active = true;
    }
    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff) { ampEnv.noteOff(); pitchEnv.noteOff(); }
        else { ampEnv.reset(); pitchEnv.reset(); clearCurrentNote(); active = false; }
    }
    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& out, int start, int count) override
    {
        if (! active) return;
        auto* l = out.getWritePointer (0);
        auto* r = out.getNumChannels() > 1 ? out.getWritePointer (1) : nullptr;
        const double sr = juce::jmax (1.0, getSampleRate());
        const float maxWidth = juce::jlimit (0.0f, 1.0f, width->load());
        const float pan = ((getCurrentlyPlayingNote() % 12) / 11.0f - 0.5f) * maxWidth;
        const float gainL = std::sqrt (0.5f * (1.0f - pan));
        const float gainR = std::sqrt (0.5f * (1.0f + pan));
        for (int i = 0; i < count; ++i)
        {
            const float amp = ampEnv.getNextSample();
            const float modEnv = pitchEnv.getNextSample();
            if (! ampEnv.isActive()) { active = false; clearCurrentNote(); break; }

            if (glideSeconds > 0.0001 && std::abs (targetHz - currentHz) > 0.001)
            {
                currentHz += glideStep;
                if ((glideStep > 0 && currentHz >= targetHz) || (glideStep < 0 && currentHz <= targetHz)) currentHz = targetHz;
            }
            else currentHz = targetHz;

            // Each voice has its own random generator, S&H value and clock.
            // Pitch S&H is active only while portamento is enabled.
            if (glideSeconds > 0.0001 && ++shCounter >= (int) (sr * 0.075))
            {
                shCounter = 0;
                shValue = random.nextFloat() * 2.0f - 1.0f;
            }
            const float cents = glideSeconds > 0.0001 ? shValue * shRange->load() * modEnv : 0.0f;
            const double hz = currentHz * std::pow (2.0, cents / 1200.0);
            const double inc = juce::MathConstants<double>::twoPi * hz / sr;
            phase += inc;
            if (phase >= juce::MathConstants<double>::twoPi) phase -= juce::MathConstants<double>::twoPi;
            const float ph = (float) (phase / juce::MathConstants<double>::twoPi);
            float sample = 0.0f;
            const int shape = (int) oscType->load();
            if (shape == 0) sample = std::sin ((float) phase);
            else if (shape == 1) sample = 2.0f * ph - 1.0f;
            else if (shape == 2) sample = ph < 0.5f ? 1.0f : -1.0f;
            else if (shape == 3) sample = 1.0f - 4.0f * std::abs (ph - 0.5f);
            else sample = 0.62f * std::sin ((float) phase) + 0.25f * std::sin ((float) phase * 2.0f) + 0.13f * std::sin ((float) phase * 3.0f);

            const float fc = juce::jlimit (30.0f, (float) (sr * 0.45), cutoff->load());
            const float alpha = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * fc / (float) sr);
            filterState += alpha * (sample - filterState);
            const float res = resonance->load();
            sample = filterState * (1.0f + res * 0.35f);
            const float s = sample * level * amp * 0.22f;
            l[start + i] += s * gainL;
            if (r != nullptr) r[start + i] += s * gainR;
        }
    }
private:
    std::atomic<float>* shRange{}; std::atomic<float>* portamento{};
    std::atomic<float>* attack{}; std::atomic<float>* decay{}; std::atomic<float>* sustain{}; std::atomic<float>* release{};
    std::atomic<float>* oscType{}; std::atomic<float>* cutoff{}; std::atomic<float>* resonance{}; std::atomic<float>* width{};
    juce::ADSR ampEnv, pitchEnv;
    juce::Random random;
    double phase = 0.0, currentHz = 0.0, targetHz = 440.0, glideStep = 0.0, glideSeconds = 0.0;
    float level = 0.0f, shValue = 0.0f, filterState = 0.0f;
    int shCounter = 0;
    bool active = false;
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
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("PORTAMENTO", "Portamento", juce::NormalisableRange<float> (0.0f, 1.5f, 0.001f), 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("ATTACK", "Attack", juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f), 0.015f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("DECAY", "Decay", juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f), 0.25f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("SUSTAIN", "Sustain", juce::NormalisableRange<float> (0.0f, 1.0f), 0.7f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("RELEASE", "Release", juce::NormalisableRange<float> (0.001f, 8.0f, 0.001f), 0.18f));
    p.push_back (std::make_unique<juce::AudioParameterChoice> ("OSC_TYPE", "Oscillator", juce::StringArray { "Sine", "Saw", "Pulse", "Triangle", "Digital" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("CUTOFF", "Cutoff", juce::NormalisableRange<float> (30.0f, 18000.0f, 1.0f, 0.35f), 12000.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("RESONANCE", "Resonance", juce::NormalisableRange<float> (0.0f, 1.0f), 0.2f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("WIDTH", "Stereo Width", juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
    return { p.begin(), p.end() };
}

R2RZXAudioProcessor::R2RZXAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "R2RZX", createParameterLayout())
{
    for (int i = 0; i < 24; ++i) synth.addVoice (new R2RZXVoice (parameters));
    synth.addSound (new R2RZXSound());
    selectPreset (0);
}
void R2RZXAudioProcessor::setParameter (const juce::String& id, float value)
{
    if (auto* param = parameters.getParameter (id))
        param->setValueNotifyingHost (param->convertTo0to1 (value));
}
void R2RZXAudioProcessor::selectPreset (int index)
{
    currentProgram = juce::jlimit (0, R2RZXFactoryPresetCount - 1, index);
    const auto& p = R2RZXFactoryPresets[currentProgram];
    const juce::String osc (p.oscillator);
    const int shape = osc == "sine" ? 0 : osc == "saw" ? 1 : osc == "pulse" ? 2 : osc == "triangle" ? 3 : 4;
    setParameter ("OSC_TYPE", (float) shape);
    setParameter ("CUTOFF", p.cutoff); setParameter ("RESONANCE", p.resonance);
    setParameter ("ATTACK", p.attack); setParameter ("DECAY", p.decay);
    setParameter ("SUSTAIN", p.sustain); setParameter ("RELEASE", p.release);
    setParameter ("SH_RANGE", p.shRangeCents); setParameter ("PORTAMENTO", p.portamentoSeconds);
    setParameter ("WIDTH", p.stereoWidth);
}
void R2RZXAudioProcessor::setCurrentProgram (int index) { selectPreset (index); }
const juce::String R2RZXAudioProcessor::getProgramName (int index)
{
    return index >= 0 && index < R2RZXFactoryPresetCount ? juce::String (R2RZXFactoryPresets[index].name) : juce::String();
}
void R2RZXAudioProcessor::prepareToPlay (double sr, int) { synth.setCurrentPlaybackSampleRate (sr); synth.allNotesOff (0, false); }
void R2RZXAudioProcessor::releaseResources() {}
void R2RZXAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    synth.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());
}
juce::AudioProcessorEditor* R2RZXAudioProcessor::createEditor() { return new R2RZXAudioProcessorEditor (*this); }
void R2RZXAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = parameters.copyState();
    state.setProperty ("currentProgram", currentProgram, nullptr);
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, dest);
}
void R2RZXAudioProcessor::setStateInformation (const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, size));
    if (xml != nullptr && xml->hasTagName (parameters.state.getType()))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        const int savedProgram = (int) tree.getProperty ("currentProgram", 0);
        parameters.replaceState (tree);
        currentProgram = juce::jlimit (0, R2RZXFactoryPresetCount - 1, savedProgram);
    }
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new R2RZXAudioProcessor(); }
