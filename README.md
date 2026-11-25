# R2R-ZX

A JUCE-based polyphonic synthesizer project targeting VST3 and Standalone.

## Implemented in the current source

- 24-voice polyphonic JUCE synthesiser.
- Five oscillator shapes: sine, saw, pulse, triangle and a layered digital waveform.
- Per-voice independent random Sample & Hold pitch modulation, with user range from 0 to +/-100 cents.
- Pitch modulation follows its own ADSR envelope and is enabled when portamento is active.
- Per-voice portamento glide.
- Amp ADSR, low-pass tone control, resonance amount and stereo voice spread.
- Browser for all 100 factory presets. Selecting a preset loads oscillator, cutoff, resonance, ADSR, S&H range, portamento and stereo width into the synth.
- Preset names and category labels are included in the UI.

## Factory bank

The complete 100-preset source data is in `Presets/FactoryPresets.json`; the matching compile-time bank is in `Source/FactoryPresets.h`.

## Build

Requires CMake 3.22+, a C++17 compiler, internet access for JUCE FetchContent, and JUCE's platform dependencies.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

A GitHub Actions workflow is configured to build the VST3 and Standalone targets on pushes to `main`.

## Current limitations

This is an active development build, not yet a verified downloadable VST3 binary. The current oscillator set includes a layered digital shape but does not yet provide a full user-editable wavetable editor. Filter resonance is a basic tone-stage control, not a full resonant filter model. Chorus, delay, reverb, animated 3D graphics and preset save/rename are not implemented yet. The source should be compiled and tested in a DAW before being treated as release-ready.
