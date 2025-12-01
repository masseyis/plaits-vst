# PlaitsVST

A polyphonic VST3/AU plugin port of [Mutable Instruments Plaits](https://mutable-instruments.net/modules/plaits/) macro-oscillator.

## Features

- All 16 synthesis engines from Plaits:
  - **VA** - Virtual analog oscillator
  - **Waveshaper** - Waveshaping oscillator
  - **FM** - 2-operator FM
  - **Grain** - Granular formant oscillator
  - **Additive** - Harmonic oscillator
  - **Wavetable** - Wavetable oscillator
  - **Chord** - Chord engine
  - **Speech** - Speech synthesis
  - **Swarm** - Swarm oscillator
  - **Noise** - Filtered noise
  - **Particle** - Particle noise
  - **String** - Karplus-Strong string
  - **Modal** - Modal resonator
  - **Bass Drum** - Analog bass drum
  - **Snare** - Analog snare drum
  - **Hi-Hat** - Analog hi-hat
- Polyphonic (1-16 voices)
- Simple AD envelope per voice
- Tracker-style keyboard/mouse UI
- 16 factory presets
- User preset saving

## Building

Requirements:
- CMake 3.22+
- C++17 compiler
- macOS, Windows, or Linux

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

The plugin will be built to `build/PlaitsVST_artefacts/`.

## Running Tests

```bash
cd build
ctest --output-on-failure
```

## Usage

### Controls

| Row | Parameter | Range | Description |
|-----|-----------|-------|-------------|
| PRESET | - | - | Preset selection |
| ENGINE | 0-15 | - | Synthesis engine |
| HARMONICS | 0-127 | - | Engine-specific (frequency ratio, harmonics, etc.) |
| TIMBRE | 0-127 | - | Engine-specific (filter, brightness, etc.) |
| MORPH | 0-127 | - | Engine-specific (shape, damping, etc.) |
| ATTACK | 0-500ms | - | Envelope attack time |
| DECAY | 10-2000ms | - | Envelope decay time |
| VOICES | 1-16 | - | Polyphony |

### Keyboard Navigation

- **Up/Down**: Select row
- **Left/Right**: Adjust value (small step)
- **Shift+Left/Right**: Adjust value (large step)
- **Shift+S**: Save current settings as new preset

### Mouse

- **Click**: Select row
- **Drag**: Adjust value
- **Scroll**: Adjust value

## Presets

Factory presets are built-in. User presets are saved to:
- macOS: `~/Music/PlaitsVST/Presets/`
- Windows: `Documents/PlaitsVST/Presets/`
- Linux: `~/Music/PlaitsVST/Presets/`

## License

PlaitsVST is released under the MIT License.

The original Plaits firmware by Emilie Gillet (Mutable Instruments) is also MIT licensed.

## Credits

- Original Plaits design and DSP code: [Emilie Gillet / Mutable Instruments](https://github.com/pichenettes/eurorack)
- JUCE framework: [ROLI Ltd](https://juce.com/)
