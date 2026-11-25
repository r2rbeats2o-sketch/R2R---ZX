# R2R-ZX

A JUCE-based polyphonic wavetable synthesizer project targeting VST3.

## Product direction

- Futuristic 3D-inspired interface: dark surfaces, neon cyan and neon red accents, depth, lighting, animated meters and waveform displays.
- Stereo/spatial sound design: stereo width, chorus, delay and reverb.
- 100 factory preset slots across Bass, Lead, Pad, Pluck, Keys, Arp, FX and Ambient.
- Polyphonic voice architecture with independent per-note portamento and sample-and-hold pitch modulation.
- Per-note modulation envelope with Attack, Decay, Sustain and Release.
- User-adjustable random pitch range, capped at +/-100 cents.
- Multiple oscillator waveforms and wavetable tables.
- VST3 plugin format.

## Build

This project uses CMake and JUCE. Configure with a recent CMake and C++17 compiler:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The initial source scaffold is being developed in stages. A successful source commit is not the same as a compiled or tested VST3 binary.
