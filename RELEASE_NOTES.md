# VialFX-Bundle 1.1

Fifteen audio effect plugins, each one its own plugin in its own window, built from the effects of the open-source
synthesizer Vital and a few new ones in the same style. VST3 and CLAP for Windows, Linux and macOS.

## What's new in 1.1

- **Vial Distortion** gains three new types: **Tube** (asymmetric, even harmonics), **Saturator** (smooth cubic) and
  **Overdrive** (exponential soft clip, with a **Stages** knob).
- **Vial Filter** gains the **Other** filter category: Band Reject, All-Pass, Metal Comb, Metal Reverb, Disperser and a
  drawable **Custom** filter (three response curves A, B, C morphed by the blend slider, edited with the pencil on
  the response display and saved with the preset). Every filter model now has a drive knob.
- **Vial Morph:** two docked editors now open side by side without overlapping, and the curve editor's toolbar
  fits at half width.
- Fixes to the Test build workflow and the macOS installer title.

## Downloads

| File | What it is |
|---|---|
| `VialFX-Bundle-1.1-Setup-windows.exe` | Windows installer: choose plugins and formats (VST3, CLAP) |
| `VialFX-Bundle-1.1-Setup-macos.pkg` | macOS installer (universal Intel + Apple Silicon): choose plugins and formats |
| `VialFX-Bundle-1.1-linux.tar.gz` | Linux VST3 + CLAP |
| `VialFX-Bundle-1.1-windows.zip`, `VialFX-Bundle-1.1-macos.zip` | Manual-install archives |

Builds are unsigned (macOS: ad-hoc signed). Windows: *More info > Run anyway* on the SmartScreen prompt. macOS:
right-click the installer and choose *Open*. See the README for install locations.

## The plugins

**Vial Chorus, Compressor, Delay, Distortion, EQ, Filter, Flanger, Phaser, Reverb** (Vital's own effects, with their
animated displays, tempo sync and, for the Filter, MIDI keytracking), and the new:

- **Vial Pitch Shifter**: Granular, MB-Granular, Retro and Spectral engines with a formant control and latency settings.
- **Vial Utility**: gain, pan, stereo width, Haas delay, mono bass, polarity invert, channel swap, DC filter and a level meter.
- **Vial Freq Shifter**: single-sideband shifting with a quality (all-pass poles) selector and feedback.
- **Vial Vibrato** and **Vial Tremolo**: two voices (A and B) each, with rate, amount, tempo sync and unit selectors.
- **Vial Morph**: spectral modes and distortion warp modes with drawn curves (Spectral Filter, Phaser, Flanger),
  wavetable editors (Spectral Table, Distort Table), split-view docked editors and an **FFT size** setting
  (256 to 32768) with a live latency readout. Curves, wavetables and the dice seed are saved in the preset.

## Everywhere

- Preset browser (`.vialfx` presets, one folder per plugin), volume slider and level meter, input spectrum display.
- Settings page: oversampling (1x to 8x), frequency units, skin selector and **Sync all**.
- `.vialskin` skins and a full **Skin Editor** (bins, colour picker, glow, gradients, bevels, Alt inspector).
- Resizable windows with locked proportions.

## Known limitations

- macOS builds have had limited testing on real hardware.
- Only the effect's own parameters, bypass and volume are automatable; Morph's drawn curves, wavetables and seed are
  preset data only.
- Vial Morph has no Vocode, FM or RM (they need a sidechain).
- Vial Morph reports its FFT size as latency while a spectral mode is active.

## License

GPL-3.0-or-later. Built on source code by Matt Tytel (GPL-3.0); VialFX-Bundle is independent and not affiliated with
or endorsed by him. See `NOTICE.md`.
