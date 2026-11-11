# VialFX-Bundle

VialFX-Bundle is a set of fifteen audio effect plugins, each one a module from the effects tab of the open-source
synthesizer [Vital](https://github.com/mtytel/vital) (and a few new ones in the same style), as its own plugin in
its own window: **Vial Chorus, Compressor, Delay, Distortion, EQ, Filter, Flanger, Phaser, Reverb, Pitch Shifter,
Utility, Freq Shifter, Vibrato, Tremolo and Morph.** It is built from GPLv3 source code (see
[License](#license) for the origin of the code) and shares its look, skins and presets with the main
[VialFX](https://github.com/asanielofficial/vialfx) plugin.

> **Status:** version 1.0. VST3 and CLAP builds for Windows, Linux and macOS, plus a Windows installer (.exe) and a
> macOS installer (.pkg), are produced by CI. Windows and Linux are tested by hand; the macOS builds (universal
> Intel + Apple Silicon, ad-hoc signed) are built by CI and have had limited testing on hardware.

## Download

The current stable version is [**VialFX-Bundle 1.0**](https://github.com/asanielofficial/vialfx-bundle/releases/latest):

| File | What it is |
|---|---|
| `VialFX-Bundle-1.0-Setup-windows.exe` | Windows installer: pick the plugins and the formats (VST3, CLAP) you want |
| `VialFX-Bundle-1.0-Setup-macos.pkg` | macOS installer: pick the plugins and formats |
| `VialFX-Bundle-1.0-linux.tar.gz` | Linux VST3 + CLAP (copy into `~/.vst3` and `~/.clap`) |
| `VialFX-Bundle-1.0-windows.zip`, `VialFX-Bundle-1.0-macos.zip` | Manual-install archives |

The newest `main` build is published as the [nightly pre-release](https://github.com/asanielofficial/vialfx-bundle/releases/tag/nightly).
These builds are unsigned (macOS: ad-hoc signed) and meant for testing; on macOS you may need to allow them in
System Settings > Privacy & Security the first time.

## The plugins

Every plugin runs Vital's own DSP for that module and has the same window: a top bar with the preset browser,
the volume slider and level meter, an input spectrum display and a settings cog; below it the effect module
itself, with an on/off button, its knobs and sliders, and tempo sync wherever Vital has it.

| Plugin | What it is |
|---|---|
| **Vial Chorus** | Vital's chorus: voices, delay, feedback, mod depth and tempo sync, with its animated voice display |
| **Vial Compressor** | Vital's multiband compressor with its band display |
| **Vial Delay** | Stereo delay with sync, feedback and filtering |
| **Vial Distortion** | Distortion with Vital's types, drive, filter and spectrum display |
| **Vial EQ** | Vital's three-band EQ with its response display |
| **Vial Filter** | Vital's filter models, with **Keytrack** that follows incoming MIDI notes |
| **Vial Flanger** | Flanger with sync, feedback and its animated display |
| **Vial Phaser** | Phaser with sync, feedback and its animated display |
| **Vial Reverb** | Vital's reverb with its size, decay, filter and chorus controls |
| **Vial Pitch Shifter** | Four engines (Granular, MB-Granular, Retro, Spectral) with a formant control and latency compensation settings |
| **Vial Utility** | Gain, pan, stereo width, Haas delay, mono bass, polarity invert, channel swap, DC filter and a level meter |
| **Vial Freq Shifter** | Single-sideband frequency shifting with a quality (all-pass poles) selector and feedback |
| **Vial Vibrato** | Two vibrato voices (A and B) with rate, amount, sync and unit selectors |
| **Vial Tremolo** | Two tremolo voices (A and B) with rate, amount, sync and unit selectors |
| **Vial Morph** | The spectral morph and distortion warp of the main VialFX plugin as an effect (below) |

### Vial Morph

Morph is the spectral stage and the distortion warp of VialFX in one effect: a **spectral mode** and amount (left)
and a **warp mode** and amount (right), with a swap button between them.

- **Spectral modes:** Formant Scale, Harmonic Stretch, Inharmonic Stretch, Smear, Random Amplitudes (with the dice
  button that picks a new seed), Low Pass, High Pass, Phase Disperse, Shepard Tone, Spectral Time Skew, Spectral
  Gate, and the drawn and table modes below.
- **Warp modes:** Sync, Formant, Quantize, Bend, Squeeze, Pulse and Distort Table. (FM, RM and Vocode need a
  sidechain lane and are not part of the effect.)
- **Drawn modes:** Spectral Filter, Spectral Phaser and Spectral Flanger have a pencil button that docks a curve
  editor under the module: paint brush and patterns, grid sizes, smoothing, a shape selector, a logarithmic
  frequency axis and a bar of detail controls (gain, width/peak, notches/resonance, taps/spread/depth/phase).
- **Table modes:** Spectral Table and Distort Table each have a pencil that opens a full wavetable editor (save,
  import, edit, drop audio files to import) and a cogwheel that docks the table's settings menu (a live 3D view
  following the knob, plus phase, bias, smooth, drive, DC filter and mix, or gain, pitch, resonance and mix).
- **Split view:** two docked editors open side by side; the window grows to make room.
- The drawn curves, the wavetables and the dice seed are saved with the preset.
- **FFT size** is set on the settings page (below), with a live latency readout. With no spectral mode chosen
  there is no latency; with one, the FFT size is reported to the host for delay compensation.

## Settings

The settings cog of each plugin opens a page with:

- **Oversampling:** 1x (Draft), 2x (Recommended), 4x and 8x.
- **Display:** Frequency Units (Semitones / Hz) and the **Skin** selector, plus **Sync all**, which publishes the
  skin to every VialFX plugin (see [Skins](#skins)).
- **Vial Filter:** Keytrack (follow MIDI notes).
- **Vial Morph:** **FFT Size** (256, 512, 1024 (default), 2048, 4096, 8192, 16384 or 32768) with a latency readout
  in samples and milliseconds.

## Presets

Presets are `.vialfx` files saved in `Documents/VialFX/Bundle/<Effect>/User/Presets`; sub-folders there show up as
folders in the preset browser. Older `.vital` presets still open. Each plugin has its own preset folder, and
**Initialize Preset** resets the plugin without turning the effect off.

## Skins

Skins use the `.vialskin` extension (older `.vitalskin` files still open); the default skin is bundled. In a
plugin's settings, **Sync all** publishes the current skin to `Documents/VialFX/Bundle/Shared/bundle.vialskin`;
every open or newly opened Vial plugin picks it up (unless you later choose a different skin in that plugin).
Skins are shared with the main VialFX plugin.

### Skin Editor

Open it by Alt-clicking the menu (≡) button next to the preset name and choosing **Open Skin Editor**. It is the
editor from the main VialFX plugin: it opens next to the plugin window (never on top of it), stays on top and can be
resized.

- **Bins:** the interface is grouped into bins (Header, Effects, Preset Browser, Settings / Overlays) plus a Global bin for the
  base skin. The Effects bin edits every effect module together and each module (Chorus ... Morph) has its own
  sub-bin. **+ Add override** adds more colours or values to a bin.
- **Colour picker:** hex (AARRGGBB), RGB, alpha, undo/redo, Default (full reset), Save and Import of `.vialskin` files.
- **Hover outline / Alt inspector:** the selected bin stays outlined in the plugin; hovering the plugin outlines the
  section and its bin; hold Alt to inspect a single knob, slider, button or text and Alt-click it to edit the colours
  it uses.
- **Glow, gradients and bevels:** every colour has Glow Strength and Glow Size knobs, can become a 2-3 stop gradient
  with an angle, and can be given a bevel (size, depth, gloss and light direction). All of these are drawn by a
  full-screen pass and saved in the skin.
- The plugin window keeps its proportions when the skin's spacing changes.

## Window and host support

- **Formats:** VST3 and CLAP (build with `-DVITALFX_ENABLE_CLAP=OFF` to skip CLAP).
- **Platforms:** Windows (x64), Linux (x64) and macOS (universal x86_64 + arm64, 10.13+).
- **Resizing:** every plugin window can be resized with its proportions locked; sizes scale smoothly.
- **Hosts:** only the effect's own parameters, bypass and the volume are exposed for automation.

## Building

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Linux needs JUCE's usual dev packages (X11, freetype, ALSA, OpenGL, curl):

```
sudo apt-get install build-essential ninja-build pkg-config libasound2-dev libfreetype6-dev libgl1-mesa-dev \
  libcurl4-openssl-dev libx11-dev libxext-dev libxinerama-dev libxrandr-dev libxcursor-dev
```

To build only some plugins: `-DVITALFX_ONLY="Reverb;Delay"`. The GitHub Actions workflows are `Build VialFX-Bundle`
(all platforms, installers, host tests; run on demand and publishes the nightly pre-release) and `Test build` (one
portable VST3 zip for Windows or Linux).

## Tests

`vialfx_host_test` (build with `-DVITALFX_BUILD_TESTS=ON`) loads a built plugin in a headless host and checks its
parameters, audio, bypass and state, plus effect-specific checks: `--pitch` (measured pitch and formants per
mode), `--utility`, `--shifter`, `--vibrato`, `--tremolo` and `--morph` (every spectral and warp mode stays finite,
latency is reported). CI runs them on Linux before every full build.

## Acknowledgements

**Matt Tytel.** VialFX-Bundle would not exist without him. It is built on the open-source synthesizer code he wrote
and released under the GPLv3, including the DSP, the interface and the modified JUCE modules. Thank you for
sharing that work openly so that others can learn from it and build on it. VialFX-Bundle is an independent project
and is not affiliated with or endorsed by him or Vital Audio.

**-LightningPig** made the art for VialFX. Thank you for giving the plugins their look.

**Every tester.** Thank you to everyone who installed early builds, reported bugs and sent screenshots.

## License

GPL-3.0-or-later, see [LICENSE](LICENSE). This project contains code from [Vital](https://github.com/mtytel/vital)
by Matt Tytel (GPL-3.0), vendored under `vendor/vital` (see `vendor/vital/UPSTREAM.txt` for the exact commit), and
Vital's modified JUCE 6.0.5 modules. If you distribute builds you must comply with that license, including making the
corresponding source available.
