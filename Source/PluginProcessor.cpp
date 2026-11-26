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
        wtPosition = raw ("WT_POS"); oscMix = raw ("OSC_MIX"); detune = raw ("DETUNE");
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
        phase = phaseB = 0.0; ic1eq = ic2eq = 0.0f; active = true;
    }
    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff) { ampEnv.noteOff(); pitchEnv.noteOff(); }
        else { ampEnv.reset(); pitchEnv.reset(); clearCurrentNote(); active = false; }
    }
    void pitchWheelMoved (int value) override
    {
        pitchWheelSemitones = ((float) value - 8192.0f) / 8192.0f * 2.0f;
    }
    void controllerMoved (int controller, int value) override
    {
        if (controller == 1) modWheel = (float) value / 127.0f;
    }

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
            const double hz = currentHz * std::pow (2.0, (cents + pitchWheelSemitones * 100.0f + modWheel * shValue * shRange->load() * 0.25f) / 1200.0);
            const float detuneCents = detune->load();
            const double incA = juce::MathConstants<double>::twoPi * hz / sr;
            const double incB = juce::MathConstants<double>::twoPi * hz * std::pow (2.0, detuneCents / 1200.0) / sr;
            phase += incA; phaseB += incB;
            if (phase >= juce::MathConstants<double>::twoPi) phase -= juce::MathConstants<double>::twoPi;
            if (phaseB >= juce::MathConstants<double>::twoPi) phaseB -= juce::MathConstants<double>::twoPi;
            const int shape = (int) oscType->load();
            const float position = wtPosition->load();
            auto tableSample = [&] (double phs, float pos)
            {
                const float ph = (float) (phs / juce::MathConstants<double>::twoPi);
                const float sine = std::sin ((float) phs);
                const float saw = 2.0f * ph - 1.0f;
                const float pulse = ph < 0.5f ? 1.0f : -1.0f;
                const float tri = 1.0f - 4.0f * std::abs (ph - 0.5f);
                const float digital = 0.62f * sine + 0.25f * std::sin ((float) phs * 2.0f) + 0.13f * std::sin ((float) phs * 3.0f);
                const float frames[5] = { sine, saw, pulse, tri, digital };
                const float frame = juce::jlimit (0.0f, 4.0f, pos * 4.0f + (float) shape * 0.12f);
                const int a = (int) frame, b = juce::jmin (4, a + 1);
                const float blend = frame - (float) a;
                const float scanned = frames[a] + (frames[b] - frames[a]) * blend;
                return scanned;
            };
            const float sampleA = tableSample (phase, position);
            const float sampleB = tableSample (phaseB, juce::jlimit (0.0f, 1.0f, position + 0.18f));
            const float mix = juce::jlimit (0.0f, 1.0f, oscMix->load());
            float sample = sampleA * (1.0f - mix) + sampleB * mix;

            const float fc = juce::jlimit (30.0f, (float) (sr * 0.45), cutoff->load());
            const float g = std::tan (juce::MathConstants<float>::pi * fc / (float) sr);
            const float q = 0.5f + resonance->load() * 9.5f;
            const float k = 1.0f / q;
            const float a1 = 1.0f / (1.0f + g * (g + k));
            const float a2 = g * a1;
            const float a3 = g * a2;
            const float v3 = sample - ic2eq;
            const float v1 = a1 * ic1eq + a2 * v3;
            const float v2 = ic2eq + a2 * ic1eq + a3 * v3;
            ic1eq = 2.0f * v1 - ic1eq;
            ic2eq = 2.0f * v2 - ic2eq;
            sample = v2;
            const float s = sample * level * amp * 0.22f;
            l[start + i] += s * gainL;
            if (r != nullptr) r[start + i] += s * gainR;
        }
    }
private:
    std::atomic<float>* shRange{}; std::atomic<float>* portamento{};
    std::atomic<float>* attack{}; std::atomic<float>* decay{}; std::atomic<float>* sustain{}; std::atomic<float>* release{};
    std::atomic<float>* oscType{}; std::atomic<float>* cutoff{}; std::atomic<float>* resonance{}; std::atomic<float>* width{};
    std::atomic<float>* wtPosition{}; std::atomic<float>* oscMix{}; std::atomic<float>* detune{};
    juce::ADSR ampEnv, pitchEnv;
    juce::Random random;
    double phase = 0.0, phaseB = 0.0, currentHz = 0.0, targetHz = 440.0, glideStep = 0.0, glideSeconds = 0.0;
    float level = 0.0f, shValue = 0.0f, ic1eq = 0.0f, ic2eq = 0.0f;
    float pitchWheelSemitones = 0.0f, modWheel = 0.0f;
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
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("WT_POS", "Wavetable Position", juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("OSC_MIX", "Oscillator B Mix", juce::NormalisableRange<float> (0.0f, 1.0f), 0.35f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("DETUNE", "Oscillator B Detune", juce::NormalisableRange<float> (0.0f, 30.0f), 7.0f));
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
