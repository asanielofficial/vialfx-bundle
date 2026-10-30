# VialFX-Bundle

Vital's effects, each as its own VST3 plugin:

**Vial Chorus · Vial Compressor · Vial Delay · Vial Distortion · Vial EQ · Vial Filter · Vial Flanger · Vial Phaser · Vial Reverb**

Each plugin is the same module you see on Vital's effects tab, running Vital's own DSP, in its own window with:

- a top bar with a preset selector (browse, save, previous/next), Vital's volume slider and level meter, and a settings cog
- a settings page with **Oversampling** (1x to 8x) and Vital's **Display** options (Frequency Units: Semitones/Hz, and Skin)
- Vial Filter only: **Keytrack**, following incoming MIDI notes

Presets are saved as `.vialfx` files in `Documents/VialFX/Bundle/<Effect>/User/Presets` (sub-folders there show up as folders in the preset browser).

## Status

| | Linux | Windows | macOS |
|---|---|---|---|
| VST3 | built and tested here (Reverb) | built by CI | built by CI (universal, ad-hoc signed, untested on hardware) |
| CLAP | built here (Reverb, loads and enumerates) | built by CI | built by CI (untested on hardware) |

CLAP is built alongside VST3; pass `-DVITALFX_ENABLE_CLAP=OFF` to skip it.

## Downloads

Every push to `main` refreshes the **nightly** pre-release on the Releases page with the plain files (no extra zip layer):

| File | What it is |
|---|---|
| `VialFX-Bundle-Setup-windows.exe` | Windows installer (pick plugins and formats) |
| `VialFX-Bundle-Setup-macos.pkg` | macOS installer (pick plugins and formats) |
| `VialFX-Bundle-linux.tar.gz` | Linux VST3 + CLAP (copy into `~/.vst3` and `~/.clap`) |
| `VialFX-Bundle-windows.zip`, `VialFX-Bundle-macos.zip` | Manual-install archives |

## Skins

Skins use the `.vialskin` extension. In a plugin's settings (cog), **Sync skin to all VialFX plugins** publishes
the current skin to `Documents/VialFX/Bundle/Shared/bundle.vialskin`; every open or newly opened Vial plugin
picks it up (unless you later choose a different skin in that plugin).

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
