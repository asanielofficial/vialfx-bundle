// VialFX - Licensed under GPL-3.0-or-later.
// Minimal VST3 host used in CI: loads a built plugin, checks parameters, processes audio,
// checks bypass and state round-trips. Usage: host_test <path-to.vst3> [--editor]
#include <JuceHeader.h>
#include <cmath>
#include <cstdio>
#include <vector>

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("FAIL: %s\n", msg); ++failures; } else std::printf("ok:   %s\n", msg); } while (0)

static bool allFinite(const AudioBuffer<float>& b) {
  for (int c = 0; c < b.getNumChannels(); ++c)
    for (int i = 0; i < b.getNumSamples(); ++i)
      if (!std::isfinite(b.getSample(c, i))) return false;
  return true;
}

int main(int argc, char** argv) {
  ScopedJuceInitialiser_GUI juce_init;
  if (argc < 2) { std::printf("usage: host_test plugin.vst3 [--editor]\n"); return 2; }
  const bool test_editor = argc > 2 && String(argv[2]) == "--editor";
  const bool keytrack_test = argc > 2 && String(argv[2]) == "--keytrack";   // filter plugin: MIDI note -> cutoff
  const bool pitch_test = argc > 2 && String(argv[2]) == "--pitch";   // pitch shifter: measured pitch per mode
  const bool editor_only = argc > 2 && String(argv[2]) == "--editor-only";   // keep window up ~15 s for screenshots

  AudioPluginFormatManager formats;
  formats.addFormat(new VST3PluginFormat());
  OwnedArray<PluginDescription> found;
  VST3PluginFormat vst3;
  vst3.findAllTypesForFile(found, File(argv[1]).getFullPathName());
  CHECK(found.size() == 1, "plugin file exposes exactly one VST3 plugin");
  if (found.size() != 1) return 1;
  std::printf("      name='%s' manufacturer='%s' category='%s' midi=%d\n",
              found[0]->name.toRawUTF8(), found[0]->manufacturerName.toRawUTF8(),
              found[0]->category.toRawUTF8(), (int) found[0]->isInstrument);

  String error;
  std::unique_ptr<AudioPluginInstance> plugin(formats.createPluginInstance(*found[0], 48000.0, 512, error));
  CHECK(plugin != nullptr, ("instantiates: " + error).toRawUTF8());
  if (!plugin) return 1;

  CHECK(!found[0]->isInstrument, "reported as an effect, not an instrument");
  CHECK(plugin->getTotalNumInputChannels() == 2 && plugin->getTotalNumOutputChannels() == 2, "stereo in / stereo out");

  auto& params = plugin->getParameters();
  std::printf("      %d host parameters\n", params.size());
  CHECK(params.size() > 5, "exposes parameters to the host");
  bool has_volume = false, has_bypass = false;
  AudioProcessorParameter* mix = nullptr;
  for (auto* p : params) {
    String n = p->getName(64);
    if (n.equalsIgnoreCase("Volume")) has_volume = true;
    if (n.containsIgnoreCase("bypass")) has_bypass = true;
    if ((n.containsIgnoreCase("dry") && n.containsIgnoreCase("wet")) || n.endsWithIgnoreCase(" mix")) mix = p;
  }
  CHECK(has_volume, "volume is a host parameter");
  CHECK(has_bypass, "bypass is a host parameter");

  if (std::getenv("VFX_VERBOSE"))
    for (auto* p : params) std::printf("      param %-40s = %.4f  (%s)\n", p->getName(64).toRawUTF8(), p->getValue(), p->getText(p->getValue(), 32).toRawUTF8());

  if (editor_only) {
    std::unique_ptr<AudioProcessorEditor> editor(plugin->createEditor());
    if (!editor) { std::printf("FAIL: no editor\n"); return 1; }
    std::printf("editor size %d x %d\n", editor->getWidth(), editor->getHeight());
    editor->setTopLeftPosition(40, 40);
    editor->addToDesktop(ComponentPeer::windowHasTitleBar);
    editor->setVisible(true);
    MessageManager::getInstance()->runDispatchLoopUntil(60000);
    editor->removeFromDesktop();
    return 0;
  }


  if (pitch_test) {
    auto find = [&](const char* name) -> AudioProcessorParameter* {
      for (auto* p : params) if (p->getName(64).equalsIgnoreCase(name)) return p;
      return nullptr;
    };
    auto *mode = find("Pitch Shifter Mode"), *pitch = find("Pitch Shifter Pitch"), *fc = find("Pitch Shifter Formant Control"),
         *formant = find("Pitch Shifter Formant"), *wet = find("Pitch Shifter Mix");
    CHECK(mode && pitch && fc && formant && wet, "pitch shifter parameters found");
    if (!(mode && pitch && fc && formant && wet)) return 1;
    const int block = 512;
    plugin->setPlayConfigDetails(2, 2, 48000.0, block);
    plugin->prepareToPlay(48000.0, block);
    wet->setValue(1.0f);

    // Plays a tone (sine or vowel-like harmonic series) and returns the output spectrum peak (Hz) and centroid (Hz).
    auto run = [&](int m, float semis, float formant_semis, bool formant_on, double f0, bool vowel, double& centroid, double& rms_ratio) {
      mode->setValue(m / 3.0f);
      pitch->setValue((semis + 24.0f) / 48.0f);
      formant->setValue((formant_semis + 24.0f) / 48.0f);
      fc->setValue(formant_on ? 1.0f : 0.0f);
      plugin->reset();
      MidiBuffer midi; AudioBuffer<float> buf(2, block);
      const int total_blocks = 120; const int keep_from = 60;
      std::vector<float> out, in_keep;
      double phase = 0, in_sq = 0, out_sq = 0;
      auto env = [](double f) { return std::exp(-std::pow(std::log2(f / 700.0) / 0.6, 2.0)) + 0.5 * std::exp(-std::pow(std::log2(f / 2400.0) / 0.5, 2.0)); };
      for (int b = 0; b < total_blocks; ++b) {
        for (int i = 0; i < block; ++i) {
          double v = 0;
          if (!vowel) v = 0.5 * std::sin(phase);
          else for (int h = 1; h * f0 < 8000; ++h) v += 0.12 * env(h * f0) * std::sin(phase * h);
          phase += 2.0 * MathConstants<double>::pi * f0 / 48000.0;
          buf.setSample(0, i, (float) v); buf.setSample(1, i, (float) v);
          if (b >= keep_from) in_sq += v * v;
        }
        plugin->processBlock(buf, midi);
        if (b >= keep_from) for (int i = 0; i < block; ++i) { out.push_back(buf.getSample(0, i)); out_sq += buf.getSample(0, i) * buf.getSample(0, i); }
      }
      rms_ratio = std::sqrt(out_sq / std::max(in_sq, 1e-12)) / 0.5012;   // plugin volume default is -6 dB
      // Spectrum via dsp::FFT, 16384 points, Hann.
      const int N = 16384;
      std::vector<double> re(N, 0.0), im(N, 0.0), data(N, 0.0);
      for (int i = 0; i < N && i < (int) out.size(); ++i)
        re[i] = out[i] * (0.5 - 0.5 * std::cos(2.0 * MathConstants<double>::pi * i / N));
      for (int i = 1, j = 0; i < N; ++i) {   // in-place radix-2 FFT
        int bit = N >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(re[i], re[j]);
      }
      for (int len = 2; len <= N; len <<= 1) {
        double ang = -2.0 * MathConstants<double>::pi / len;
        for (int i = 0; i < N; i += len)
          for (int k = 0; k < len / 2; ++k) {
            double wr = std::cos(ang * k), wi = std::sin(ang * k);
            double xr = re[i + k + len / 2] * wr - im[i + k + len / 2] * wi, xi = re[i + k + len / 2] * wi + im[i + k + len / 2] * wr;
            re[i + k + len / 2] = re[i + k] - xr; im[i + k + len / 2] = im[i + k] - xi;
            re[i + k] += xr; im[i + k] += xi;
          }
      }
      for (int k = 0; k < N; ++k) data[k] = std::sqrt(re[k] * re[k] + im[k] * im[k]);
      int peak = 1; double sum = 0, wsum = 0;
      for (int k = 1; k < N / 2; ++k) {
        if (data[k] > data[peak]) peak = k;
        double f = k * 48000.0 / N; if (f > 100 && f < 10000) { sum += data[k] * data[k]; wsum += f * data[k] * data[k]; }
      }
      centroid = sum > 0 ? wsum / sum : 0;
      return peak * 48000.0 / N;
    };

    const char* names[] = { "granular", "mb-granular", "retro", "spectral" };
    for (int m = 0; m < 4; ++m) {
      double c, r;
      double f = run(m, 0.0f, 0.0f, false, 440.0, false, c, r);
      std::printf("      %-11s 0 st:  peak %.1f Hz, level ratio %.3f\n", names[m], f, r);
      CHECK(std::abs(f - 440.0) < 6.0 && r > 0.8 && r < 1.2, (String(names[m]) + ": 0 st is transparent").toRawUTF8());
      for (float st : { 7.0f, -5.0f, 12.0f }) {
        f = run(m, st, 0.0f, false, 440.0, false, c, r);
        double expect = 440.0 * std::pow(2.0, st / 12.0);
        std::printf("      %-11s %+.0f st: peak %.1f Hz (expect %.1f), level ratio %.3f\n", names[m], st, f, expect, r);
        CHECK(std::abs(f - expect) < expect * 0.012 + 3.0 && r > 0.15 && r < 2.5, (String(names[m]) + ": shifts by the requested interval").toRawUTF8());
      }
    }
    // Formants: shift a vowel up 7 st. Auto formant keeps the spectral envelope (centroid stays near the original);
    // formant control with +12 st moves the envelope up.
    for (int m : { 0, 1, 3 }) {
      double c0, c_auto, c_up, c_off, r;
      run(m, 0.0f, 0.0f, false, 150.0, true, c0, r);
      run(m, 7.0f, 0.0f, false, 150.0, true, c_auto, r);
      run(m, 7.0f, 12.0f, true, 150.0, true, c_up, r);
      run(m, 7.0f, 0.0f, true, 150.0, true, c_off, r);
      std::printf("      %-11s vowel centroid: original %.0f, +7 st auto %.0f, +7 st formant 0 %.0f, +7 st formant +12 %.0f Hz\n", names[m], c0, c_auto, c_off, c_up);
      CHECK(std::abs(c_auto / c0 - 1.0) < 0.25, (String(names[m]) + ": auto formant keeps the vowel envelope").toRawUTF8());
      CHECK(c_up > c_auto * 1.2, (String(names[m]) + ": formant +12 st moves the envelope up").toRawUTF8());
    }
    std::printf(failures ? "\n%d FAILURE(S)\n" : "\nALL CHECKS PASSED\n", failures);
    return failures ? 1 : 0;
  }

  if (keytrack_test) {
    AudioProcessorParameter *kt = nullptr, *cutoff = nullptr, *on = nullptr;
    for (auto* p : params) {
      String n = p->getName(64);
      if (n.containsIgnoreCase("keytrack") || n.containsIgnoreCase("key track")) kt = p;
      else if (n.containsIgnoreCase("cutoff") && !cutoff) cutoff = p;
    }
    CHECK(kt != nullptr, "filter exposes a keytrack parameter");
    CHECK(cutoff != nullptr, "filter exposes a cutoff parameter");
    if (!kt || !cutoff) return 1;
    std::printf("      keytrack param '%s', cutoff param '%s'\n", kt->getName(64).toRawUTF8(), cutoff->getName(64).toRawUTF8());
    const int block = 512;
    plugin->setPlayConfigDetails(2, 2, 48000.0, block);
    plugin->prepareToPlay(48000.0, block);
    cutoff->setValue(0.5f);
    for (auto* p : params) if (p->getName(64).containsIgnoreCase("mix") || p->getName(64).containsIgnoreCase("dry")) p->setValue(1.0f);

    // Brightness of filtered noise (energy of the first difference over total energy) while holding `note`.
    auto brightness = [&](int note, float keytrack_norm) {
      kt->setValue(keytrack_norm);
      plugin->reset();
      Random rng(1234);
      double diff = 0, total = 0;
      for (int b = 0; b < 60; ++b) {
        AudioBuffer<float> buf(2, block);
        MidiBuffer midi;
        if (b == 0 && note >= 0) midi.addEvent(MidiMessage::noteOn(1, note, (juce::uint8) 100), 0);
        for (int c = 0; c < 2; ++c) for (int i = 0; i < block; ++i) buf.setSample(c, i, rng.nextFloat() * 0.5f - 0.25f);
        plugin->processBlock(buf, midi);
        if (b < 20) continue;
        for (int i = 1; i < block; ++i) {
          float x = buf.getSample(0, i), d = x - buf.getSample(0, i - 1);
          total += x * x; diff += d * d;
        }
      }
      return total > 0 ? diff / total : 0.0;
    };
    double off_low = brightness(36, 0.5f), off_high = brightness(96, 0.5f);   // 0.5 = keytrack 0.0
    double on_low = brightness(36, 1.0f), on_high = brightness(96, 1.0f);     // 1.0 = keytrack +1.0
    std::printf("      keytrack 0: note36=%.4f note96=%.4f | keytrack +1: note36=%.4f note96=%.4f\n", off_low, off_high, on_low, on_high);
    std::printf("      keytrack +1, no note: %.4f; cutoff low/high (norm 0.2 / 0.8), keytrack 0: %.4f / %.4f\n",
                brightness(-1, 1.0f), (cutoff->setValue(0.2f), brightness(-1, 0.5f)), (cutoff->setValue(0.8f), brightness(-1, 0.5f)));
    cutoff->setValue(0.5f);
    if (std::getenv("VFX_VERBOSE"))
      for (int n : { 0, 24, 48, 60, 72, 96, 127 }) std::printf("      keytrack +1 note %3d: %.5f\n", n, brightness(n, 1.0f));
    CHECK(std::abs(off_high - off_low) < 0.05 * std::max(off_high, off_low) + 1e-6, "keytrack 0: note does not change the filter");
    CHECK(std::abs(on_high - on_low) > 0.2 * std::max(on_high, on_low), "keytrack +1: higher note opens/moves the filter");
    CHECK(on_high != off_high, "keytrack setting changes the sound");
    std::printf(failures ? "\n%d FAILURE(S)\n" : "\nALL CHECKS PASSED\n", failures);
    return failures ? 1 : 0;
  }

  const int block = 512, blocks = 400;   // ~4.3 s at 48 kHz
  plugin->setPlayConfigDetails(2, 2, 48000.0, block);
  plugin->prepareToPlay(48000.0, block);

  auto render = [&](bool impulse, double& tail_energy, double& total_in, double& total_out, bool& finite) {
    plugin->reset();
    tail_energy = total_in = total_out = 0.0; finite = true;
    MidiBuffer midi;
    AudioBuffer<float> buf(2, block);
    for (int w = 0; w < 60; ++w) { buf.clear(); plugin->processBlock(buf, midi); }   // let ramps settle
    for (int b = 0; b < blocks; ++b) {
      buf.clear();
      if (impulse && b == 0) { buf.setSample(0, 0, 1.0f); buf.setSample(1, 0, 1.0f); }
      for (int c = 0; c < 2; ++c) for (int i = 0; i < block; ++i) total_in += std::abs(buf.getSample(c, i));
      plugin->processBlock(buf, midi);
      finite = finite && allFinite(buf);
      for (int c = 0; c < 2; ++c) for (int i = 0; i < block; ++i) {
        double v = std::abs(buf.getSample(c, i));
        total_out += v;
        if (b >= 2) tail_energy += v;
      }
    }
  };

  double tail, in, out; bool finite;
  if (std::getenv("VFX_PROBE")) {
    AudioProcessorParameter* sw = nullptr;
    for (auto* p : params) if (p->getName(64).containsIgnoreCase("switch")) sw = p;
    for (int on = 0; on <= 1; ++on) {
      if (sw) sw->setValue((float) on);
      render(true, tail, in, out, finite);
      std::printf("      PROBE switch=%d: in=%.4f out=%.6f tail=%.6f\n", on, in, out, tail);
    }
    if (sw) sw->setValue(1.0f);
  }
  {
    // Effect off, 1 kHz sine: the signal must come through at the default -6 dB volume (ratio 0.5).
    AudioProcessorParameter* sw = nullptr;
    for (auto* p : params) if (p->getName(64).containsIgnoreCase("switch")) sw = p;
    if (sw) {
      sw->setValue(0.0f);
      plugin->reset();
      MidiBuffer midi; AudioBuffer<float> buf(2, block);
      double in_sq = 0, out_sq = 0; long n = 0; double phase = 0;
      for (int b = 0; b < 160; ++b) {
        for (int i = 0; i < block; ++i, phase += 2.0 * MathConstants<double>::pi * 1000.0 / 48000.0) {
          float v = 0.5f * (float) std::sin(phase);
          buf.setSample(0, i, v); buf.setSample(1, i, v);
        }
        AudioBuffer<float> ref(buf);
        plugin->processBlock(buf, midi);
        if (b >= 100) for (int c = 0; c < 2; ++c) for (int i = 0; i < block; ++i) {
          in_sq += ref.getSample(c, i) * ref.getSample(c, i);
          out_sq += buf.getSample(c, i) * buf.getSample(c, i); ++n;
        }
      }
      double ratio = std::sqrt(out_sq / in_sq);
      std::printf("      effect off, 1 kHz sine: out/in RMS ratio = %.4f (expect 0.5012)\n", ratio);
      CHECK(std::abs(ratio - 0.5012) < 0.03, "effect off: sine passes at the -6 dB default volume");
      sw->setValue(1.0f);
    }
  }
  render(true, tail, in, out, finite);
  CHECK(finite, "output stays finite (impulse)");
  std::printf("      impulse: in=%.3f out=%.3f tail(after 2 blocks)=%.6f\n", in, out, tail);
  CHECK(out > 1.0, "audio comes out (dry impulse plus reverb tail)");
  // Only time-based effects keep sounding after the input stops.
  const bool has_tail = found[0]->name.containsIgnoreCase("reverb") || found[0]->name.containsIgnoreCase("delay");
  if (has_tail)
    CHECK(tail > 0.5, "tail rings on after the impulse");
  else
    std::printf("      (no tail expected for '%s')\n", found[0]->name.toRawUTF8());

  // Silence in must give silence out (no self-oscillation / DC from a wrongly wired chain).
  render(false, tail, in, out, finite);
  CHECK(finite && out < 1e-3, "silence in -> silence out");

  // Bypass: output must equal input exactly.
  if (auto* bp = [&]{ for (auto* p : params) if (p->getName(64).containsIgnoreCase("bypass")) return p; return (AudioProcessorParameter*) nullptr; }()) {
    bp->setValue(1.0f);
    MidiBuffer midi;
    AudioBuffer<float> buf(2, block), ref(2, block);
    for (int c = 0; c < 2; ++c) for (int i = 0; i < block; ++i) { float v = 0.5f * std::sin(0.05f * i); buf.setSample(c, i, v); ref.setSample(c, i, v); }
    plugin->processBlock(buf, midi);
    float maxdiff = 0; for (int c = 0; c < 2; ++c) for (int i = 0; i < block; ++i) maxdiff = std::max(maxdiff, std::abs(buf.getSample(c, i) - ref.getSample(c, i)));
    CHECK(maxdiff < 1e-6f, "bypass passes audio through untouched");
    bp->setValue(0.0f);
  }

  // Dry/wet at 0 must be (nearly) transparent for non-delay-based effects after settling.
  if (mix && !found[0]->name.containsIgnoreCase("pitch")) {   // impulse-tail heuristic; meaningless for a pitch shifter
    mix->setValue(0.0f);
    render(true, tail, in, out, finite);
    std::printf("      mix=0 tail=%.6f\n", tail);
    mix->setValue(1.0f);
    double wet_tail; render(true, wet_tail, in, out, finite);
    std::printf("      mix=1 tail=%.6f\n", wet_tail);
    CHECK(wet_tail > tail, "wet mix produces more tail than dry");
  }

  // State round trip: change a parameter, save, change again, restore.
  if (!params.isEmpty()) {
    AudioProcessorParameter* target = mix ? mix : params[0];
    target->setValue(0.3f);
    MemoryBlock state; plugin->getStateInformation(state);
    CHECK(state.getSize() > 100, "state serialises");
    target->setValue(0.9f);
    plugin->setStateInformation(state.getData(), (int) state.getSize());
    CHECK(std::abs(target->getValue() - 0.3f) < 0.02f, "state restores parameter values");
  }

  if (test_editor) {
    std::unique_ptr<AudioProcessorEditor> editor(plugin->createEditor());
    CHECK(editor != nullptr, "editor can be created");
    if (editor) {
      std::printf("      editor size %d x %d\n", editor->getWidth(), editor->getHeight());
      editor->setVisible(true);
      editor->addToDesktop(0);
      MessageManager::getInstance()->runDispatchLoopUntil(1500);
      Image shot = editor->createComponentSnapshot(editor->getLocalBounds(), true, 1.0f);
      File out = File::getCurrentWorkingDirectory().getChildFile(File(argv[1]).getFileNameWithoutExtension().replaceCharacter(' ', '_') + "_editor.png");
      FileOutputStream fos(out); fos.setPosition(0); fos.truncate();
      PNGImageFormat().writeImageToStream(shot, fos);
      std::printf("      snapshot: %s\n", out.getFullPathName().toRawUTF8());
      editor->removeFromDesktop();
    }
  }

  plugin->releaseResources();
  plugin.reset();
  std::printf(failures ? "\n%d FAILURE(S)\n" : "\nALL CHECKS PASSED\n", failures);
  return failures ? 1 : 0;
}
