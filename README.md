# VialFX-Bundle

Vital's effects, each as its own VST3 plugin:

**Vial Chorus · Vial Compressor · Vial Delay · Vial Distortion · Vial EQ · Vial Filter · Vial Flanger · Vial Phaser · Vial Reverb**

Each plugin is the same module you see on Vital's effects tab, running Vital's own DSP, in its own window with:

- a top bar with a preset selector (browse, save, previous/next), Vital's volume slider and level meter, and a settings cog
- a settings page with **Oversampling** (1x to 8x) and Vital's **Display** options (Frequency Units: Semitones/Hz, and Skin)
- Vial Filter only: **Keytrack**, following incoming MIDI notes

Presets are saved as `.vialfx` files in `Documents/VialFX/Bundle/<Effect>/User/Presets` (sub-folders there show up as folders in the preset browser).

## Status

| | Linux | Windows | macOS | CLAP |
|---|---|---|---|---|
| VST3 | built and tested here (Reverb) | built by CI | off for now | off for now |

CLAP and macOS are wired into CMake but switched off while testing:
`-DVITALFX_ENABLE_CLAP=ON` and `-DVITALFX_ENABLE_MACOS=ON`.

## Building

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Linux needs JUCE's usual dev packages (X11, freetype, ALSA, OpenGL, curl). To build only some plugins:
`-DVITALFX_ONLY="Reverb;Delay"`.

`vialfx_host_test` loads a built plugin, checks parameters, audio, bypass and state:
`vialfx_host_test "Vial Reverb.vst3"`.

## License

GPL-3.0-or-later. This project contains code from [Vital](https://github.com/mtytel/vital) by Matt Tytel
(GPL-3.0), vendored under `vendor/vital` (see `vendor/vital/UPSTREAM.txt` for the exact commit), and
Vital's modified JUCE 6.0.5 modules. It is not affiliated with or endorsed by Vital Audio.
