#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
float basicR2RWave (int shape, double phase)
{
    const float p = (float) (phase / juce::MathConstants<double>::twoPi);
    const float sine = std::sin ((float) phase);
    const float saw = 2.0f * p - 1.0f;
    const float pulse = p < 0.5f ? 1.0f : -1.0f;
    const float triangle = 1.0f - 4.0f * std::abs (p - 0.5f);
    const float digital = 0.58f * sine + 0.27f * std::sin ((float) phase * 2.0f)
                        + 0.15f * std::sin ((float) phase * 3.0f);
    switch (shape)
    {
        case 0: return sine;
        case 1: return saw;
        case 2: return pulse;
        case 3: return triangle;
        default: return digital;
    }
}

// Five original, continuously morphable table families. WT position changes
// harmonic content; oscillator shape remains the starting character.
float renderR2RTable (double phase, int shape, int bank, float position)
{
    const float p = (float) phase;
    const float pos = juce::jlimit (0.0f, 1.0f, position);
    const float base = basicR2RWave (shape, phase);

    if (bank == 0)
    {
        const float next = basicR2RWave ((shape + 1) % 5, phase);
        return base + (next - base) * pos;
    }

    if (bank == 1) // Formant
    {
        const float formant = std::sin (p)
            + (0.32f + 0.36f * pos) * std::sin (2.0f * p + 0.18f)
            + (0.48f - 0.22f * pos) * std::sin (3.0f * p)
            + (0.12f + 0.24f * pos) * std::sin (4.0f * p + 0.31f)
            + 0.10f * std::sin (5.0f * p);
        return 0.22f * base + 0.78f * formant / 1.75f;
    }

    if (bank == 2) // Digital Glass
    {
        const float glass = std::sin (p)
            + 0.42f * std::sin (2.01f * p)
            + (0.30f + 0.18f * pos) * std::sin (3.97f * p + 0.2f)
            + 0.17f * std::sin (6.13f * p)
            + 0.08f * std::sin (8.31f * p + pos);
        return 0.18f * base + 0.82f * glass / 1.95f;
    }

    if (bank == 3) // Vocal Grain
    {
        const float centre = 2.0f + pos * 5.0f;
        float grains = 0.0f, weightSum = 0.0f;
        for (int harmonic = 1; harmonic <= 8; ++harmonic)
        {
            const float distance = std::abs ((float) harmonic - centre);
            const float weight = (0.12f + 0.88f / (1.0f + distance * 2.0f)) / (float) harmonic;
            grains += weight * std::sin (p * (float) harmonic);
            weightSum += weight;
        }
        return 0.18f * base + 0.82f * grains / juce::jmax (0.01f, weightSum);
    }

    // Harmonic Sweep: position progressively opens the upper partials.
    float harmonics = 0.0f, weightSum = 0.0f;
    const float spectralTilt = 0.55f + pos * 1.35f;
    for (int harmonic = 1; harmonic <= 9; ++harmonic)
    {
        const float n = (float) harmonic;
        const float weight = 1.0f / std::pow (n, spectralTilt);
        harmonics += weight * std::sin (p * n);
        weightSum += weight;
    }
    return 0.14f * base + 0.86f * harmonics / juce::jmax (0.01f, weightSum);
}

class R2RZXVoice final : public juce::SynthesiserVoice
{
public:
    explicit R2RZXVoice (R2RZXAudioProcessor::APVTS& apvts)
    {
        auto raw = [&] (const char* id) { return apvts.getRawParameterValue (id); };
        shRange = raw ("SH_RANGE"); portamento = raw ("PORTAMENTO");
        attack = raw ("ATTACK"); decay = raw ("DECAY"); sustain = raw ("SUSTAIN"); release = raw ("RELEASE");
        oscType = raw ("OSC_TYPE"); cutoff = raw ("CUTOFF"); resonance = raw ("RESONANCE"); width = raw ("WIDTH");
        wtPosition = raw ("WT_POS"); oscMix = raw ("OSC_MIX"); detune = raw ("DETUNE"); wavetableBank = raw ("WT_BANK");
        unisonVoices = raw ("UNISON"); unisonDetune = raw ("UNI_DETUNE");
        subMix = raw ("SUB_MIX"); drive = raw ("DRIVE"); filterEnvAmount = raw ("FILTER_ENV");
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
        filterEnv.setSampleRate (getSampleRate()); filterEnv.setParameters (ep); filterEnv.noteOn();
        shCounter = 0; shValue = random.nextFloat() * 2.0f - 1.0f;
        for (int u = 0; u < 6; ++u)
        {
            phaseA[u] = phaseB[u] = 0.0;
            ic1eq[u] = ic2eq[u] = 0.0f;
        }
        phaseSub = 0.0;
        active = true;
    }
    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff) { ampEnv.noteOff(); pitchEnv.noteOff(); filterEnv.noteOff(); }
        else { ampEnv.reset(); pitchEnv.reset(); filterEnv.reset(); clearCurrentNote(); active = false; }
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
        const float basePan = ((getCurrentlyPlayingNote() % 12) / 11.0f - 0.5f) * maxWidth;
        for (int i = 0; i < count; ++i)
        {
            const float amp = ampEnv.getNextSample();
            const float modEnv = pitchEnv.getNextSample();
            const float filterMod = filterEnv.getNextSample();
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

            const int shape = juce::jlimit (0, 4, (int) oscType->load());
            const int bank = juce::jlimit (0, 4, (int) wavetableBank->load());
            const float position = wtPosition->load();
            const float mix = juce::jlimit (0.0f, 1.0f, oscMix->load());
            const int voiceCount = juce::jlimit (1, 6, (int) unisonVoices->load());
            const float stackDetune = juce::jlimit (0.0f, 45.0f, unisonDetune->load());
            const float oscBDetune = juce::jlimit (0.0f, 30.0f, detune->load());
            const float driveAmount = juce::jlimit (0.0f, 1.0f, drive->load());
            const float envAmount = juce::jlimit (-1.0f, 1.0f, filterEnvAmount->load());
            const float envelopeCutoff = cutoff->load() * std::pow (2.0f, envAmount * 4.0f * filterMod);
            const float fc = juce::jlimit (30.0f, (float) (sr * 0.45), envelopeCutoff);
            const float g = std::tan (juce::MathConstants<float>::pi * fc / (float) sr);
            const float q = 0.5f + resonance->load() * 9.5f;
            const float k = 1.0f / q;
            const float a1 = 1.0f / (1.0f + g * (g + k));
            const float a2 = g * a1;
            const float a3 = g * a2;

            float sumL = 0.0f, sumR = 0.0f;
            for (int u = 0; u < voiceCount; ++u)
            {
                const float spread = voiceCount > 1
                    ? ((float) u / (float) (voiceCount - 1)) * 2.0f - 1.0f : 0.0f;
                const float laneDetune = spread * stackDetune;
                const double hzA = hz * std::pow (2.0, laneDetune / 1200.0);
                const double hzB = hz * std::pow (2.0, (laneDetune + oscBDetune) / 1200.0);
                phaseA[u] += juce::MathConstants<double>::twoPi * hzA / sr;
                phaseB[u] += juce::MathConstants<double>::twoPi * hzB / sr;
                if (phaseA[u] >= juce::MathConstants<double>::twoPi) phaseA[u] -= juce::MathConstants<double>::twoPi;
                if (phaseB[u] >= juce::MathConstants<double>::twoPi) phaseB[u] -= juce::MathConstants<double>::twoPi;

                const float sampleA = renderR2RTable (phaseA[u], shape, bank, position);
                const float sampleB = renderR2RTable (phaseB[u], shape, bank,
                    juce::jlimit (0.0f, 1.0f, position + 0.18f));
                float sample = sampleA * (1.0f - mix) + sampleB * mix;

                const float driveGain = 1.0f + driveAmount * 7.0f;
                sample = std::tanh (sample * driveGain) / std::tanh (driveGain);

                const float v3 = sample - ic2eq[u];
                const float v1 = a1 * ic1eq[u] + a2 * v3;
                const float v2 = ic2eq[u] + a2 * ic1eq[u] + a3 * v3;
                ic1eq[u] = 2.0f * v1 - ic1eq[u];
                ic2eq[u] = 2.0f * v2 - ic2eq[u];
                sample = v2;

                // Spread unison lanes across the stereo field without changing
                // the centre pitch or the MIDI-note pan position.
                const float lanePan = juce::jlimit (-0.95f, 0.95f, basePan + spread * maxWidth * 0.42f);
                const float gainL = std::sqrt (0.5f * (1.0f - lanePan));
                const float gainR = std::sqrt (0.5f * (1.0f + lanePan));
                sumL += sample * gainL / (float) voiceCount;
                sumR += sample * gainR / (float) voiceCount;
            }

            const float subAmount = juce::jlimit (0.0f, 1.0f, subMix->load());
            if (subAmount > 0.0001f)
            {
                phaseSub += juce::MathConstants<double>::twoPi * (hz * 0.5) / sr;
                if (phaseSub >= juce::MathConstants<double>::twoPi) phaseSub -= juce::MathConstants<double>::twoPi;
                const float sub = std::sin ((float) phaseSub) * subAmount * 0.38f;
                const float baseLeft = std::sqrt (0.5f * (1.0f - basePan));
                const float baseRight = std::sqrt (0.5f * (1.0f + basePan));
                sumL += sub * baseLeft;
                sumR += sub * baseRight;
            }

            const float outputGain = level * amp * 0.22f;
            l[start + i] += sumL * outputGain;
            if (r != nullptr) r[start + i] += sumR * outputGain;
        }
    }
private:
    std::atomic<float>* shRange{}; std::atomic<float>* portamento{};
    std::atomic<float>* attack{}; std::atomic<float>* decay{}; std::atomic<float>* sustain{}; std::atomic<float>* release{};
    std::atomic<float>* oscType{}; std::atomic<float>* cutoff{}; std::atomic<float>* resonance{}; std::atomic<float>* width{};
    std::atomic<float>* wtPosition{}; std::atomic<float>* oscMix{}; std::atomic<float>* detune{}; std::atomic<float>* wavetableBank{};
    std::atomic<float>* unisonVoices{}; std::atomic<float>* unisonDetune{}; std::atomic<float>* subMix{}; std::atomic<float>* drive{};
    std::atomic<float>* filterEnvAmount{};
    juce::ADSR ampEnv, pitchEnv, filterEnv;
    juce::Random random;
    double phaseA[6]{}, phaseB[6]{}, phaseSub = 0.0, currentHz = 0.0, targetHz = 440.0, glideStep = 0.0, glideSeconds = 0.0;
    float ic1eq[6]{}, ic2eq[6]{};
    float level = 0.0f, shValue = 0.0f;
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
    p.push_back (std::make_unique<juce::AudioParameterChoice> ("WT_BANK", "Wavetable Bank", juce::StringArray { "Basic Shapes", "Formant", "Digital Glass", "Vocal Grain", "Harmonic Sweep" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterInt> ("UNISON", "Unison Voices", 1, 6, 1));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("UNI_DETUNE", "Unison Detune", juce::NormalisableRange<float> (0.0f, 45.0f, 0.1f), 8.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("SUB_MIX", "Sub Mix", juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("DRIVE", "Drive", juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("FILTER_ENV", "Filter Envelope Amount", juce::NormalisableRange<float> (-1.0f, 1.0f), 0.0f));
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
    const juce::String bank (p.wavetable);
    const int bankIndex = bank == "Basic Shapes" ? 0 : bank == "Formant" ? 1
                        : bank == "Digital Glass" ? 2 : bank == "Vocal Grain" ? 3 : 4;
    setParameter ("WT_BANK", (float) bankIndex);
    setParameter ("WT_POS", p.wavetablePosition);
    setParameter ("OSC_MIX", p.oscillatorMix);
    setParameter ("DETUNE", p.detuneCents);
    setParameter ("UNISON", (float) p.unisonVoices);
    setParameter ("UNI_DETUNE", p.unisonDetuneCents);
    setParameter ("SUB_MIX", p.subMix);
    setParameter ("DRIVE", p.drive);
    setParameter ("FILTER_ENV", p.filterEnvAmount);
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
