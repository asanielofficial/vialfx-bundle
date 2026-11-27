# VialFX-Bundle 1.3

Sixteen audio effect plugins, each one its own plugin in its own window, built from the effects of the open-source
synthesizer Vital and a few new ones in the same style. VST3 and CLAP for Windows, Linux and macOS.

## Next release (not yet published)

- **LV2 for Linux, Windows and macOS:** every plugin builds as an `.lv2` bundle (MIDI input for Filter and Resonator), also in the Windows and macOS installers. Tested in a lilv host on Linux; Windows and macOS builds are new.
- **AU for macOS:** every plugin builds as an Audio Unit (`.component`), also in the macOS .pkg installer. Untested on hardware so far.

## What's new in 1.3

- **New: Vial Resonator.** A tuned resonator with four styles (Comb, Modal, String, Formant), Resonance, Damping,
  Stretch, Color and Mix. It follows MIDI notes with up to 16 voices and supports **MPE** (per-note pitch bend, slide and
  pressure) and **microtonal tuning** (Scala scale files, A4 reference, glide). Tuning, MPE and expression targets live
  on its settings page.
- **Vial Morph:** the Spectral Table and Distort Table menus now have a wavetable name bar with previous/next, the
  wavetable browser and SAVE; the Random Amplitudes dice shows hover and pressed states; knobs that were shrunk for
  Random Amplitudes now return to full size.
- The Windows and macOS installers include the Resonator as its own entry.

## What was new in 1.2

- **Vial Morph, Random Amplitudes** (ported from VialSynth): a stronger amount range (the top setting leaves about an
  eighth of the bins standing), a pattern that holds still while the amount moves, and a new layout with **MIX** and
  **SHUFFLE** knobs, labelled pills, and room for the mode selector. The **dice** is now a real, automatable parameter:
  a click rolls a new seed, and a value flip from automation does the same.

## What was new in 1.1

- **Vial Distortion:** new types **Tube**, **Saturator** and **Overdrive** (with a **Stages** knob).
- **Vial Filter:** the **Other** category (Band Reject, All-Pass, Metal Comb, Metal Reverb, Disperser and a drawable
  **Custom** filter) and a drive knob on every model.
- **Vial Morph:** docked editors open side by side, and the curve editor's toolbar fits at half width.

## Downloads

| File | What it is |
|---|---|
| `VialFX-Bundle-1.3-Setup-windows.exe` | Windows installer: choose plugins and formats (VST3, CLAP) |
| `VialFX-Bundle-1.3-Setup-macos.pkg` | macOS installer (universal Intel + Apple Silicon): choose plugins and formats |
| `VialFX-Bundle-1.3-linux.tar.gz` | Linux VST3 + CLAP |
| `VialFX-Bundle-1.3-windows.zip`, `VialFX-Bundle-1.3-macos.zip` | Manual-install archives |

Builds are unsigned (macOS: ad-hoc signed). Windows: *More info > Run anyway* on the SmartScreen prompt. macOS:
right-click the installer and choose *Open*. See the README for install locations.

## The plugins

**Vial Chorus, Compressor, Delay, Distortion, EQ, Filter, Flanger, Phaser, Reverb** (Vital's own effects, with their
animated displays, tempo sync and, for the Filter, MIDI keytracking), and the new:

- **Vial Pitch Shifter**: Granular, MB-Granular, Retro and Spectral engines with a formant control and latency settings.
- **Vial Utility**: gain, pan, stereo width, Haas delay, mono bass, polarity invert, channel swap, DC filter and a level meter.
- **Vial Freq Shifter**: single-sideband shifting with a quality (all-pass poles) selector and feedback.
- **Vial Vibrato** and **Vial Tremolo**: two voices (A and B) each, with rate, amount, tempo sync and unit selectors.
- **Vial Resonator**: a tuned resonator with MIDI, MPE, microtonal tuning and up to 16 voices.
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
- Vial Resonator supports Scala scale files but not MTS-ESP, and has had little testing on macOS. Its VST3 exposes
  many hidden MIDI-controller parameters to the host (needed for pitch bend, slide and pressure).
- Vial Morph has no Vocode, FM or RM (they need a sidechain).
- Vial Morph reports its FFT size as latency while a spectral mode is active.

## License

GPL-3.0-or-later. Built on source code by Matt Tytel (GPL-3.0); VialFX-Bundle is independent and not affiliated with
or endorsed by him. See `NOTICE.md`.
