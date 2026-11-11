// VialFX - Licensed under GPL-3.0-or-later.
// Minimal VST3 host used in CI: loads a built plugin, checks parameters, processes audio,
// checks bypass and state round-trips. Usage: host_test <path-to.vst3> [--editor]
#include <JuceHeader.h>
#include <cmath>
#include <complex>
#include <atomic>
#include <thread>
#include <chrono>
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
  const bool utility_test = argc > 2 && String(argv[2]) == "--utility";   // utility: gain/pan/invert/swap/width/haas/mono bass
  const bool shifter_test = argc > 2 && String(argv[2]) == "--shifter";   // freq shifter: sideband, units, stereo, feedback
  const bool vibrato_test = argc > 2 && String(argv[2]) == "--vibrato";   // vibrato: depth, rate, sum of A+B, bounded
  const bool tremolo_test = argc > 2 && String(argv[2]) == "--tremolo";   // tremolo: depth, rate, A+B sum, bounded
  const bool morph_test = argc > 2 && String(argv[2]) == "--morph";   // morph: every spectral/warp mode finite + audible, latency
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
    if ((n.containsIgnoreCase("dry") && n.containsIgnoreCase("wet")) || (n.endsWithIgnoreCase(" mix") && found[0]->name.containsIgnoreCase("pitch"))) mix = p;
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
    // "--audio": run a 220 Hz tone through the plugin in real time so meters and viewers have something to show.
    std::atomic<bool> stop{false};
    std::thread audio;
    if (argc > 3 && String(argv[3]) == "--audio") {
      plugin->setPlayConfigDetails(2, 2, 48000.0, 512);
      plugin->prepareToPlay(48000.0, 512);
      audio = std::thread([&] {
        AudioBuffer<float> buf(2, 512); MidiBuffer midi; double ph = 0;
        while (!stop) {
          for (int i = 0; i < 512; ++i) {
            float v = 0.35f * (float) std::sin(ph) * (0.6f + 0.4f * (float) std::sin(ph * 0.004));
            ph += 2 * MathConstants<double>::pi * 220.0 / 48000.0;
            buf.setSample(0, i, v); buf.setSample(1, i, v * 0.7f);
          }
          plugin->processBlock(buf, midi);
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      });
    }
    // VFX_RESIZE="ms:WxH,ms:WxH": resize the editor after the given delays (for screenshots of the dynamic scaling).
    if (const char* resize_env = std::getenv("VFX_RESIZE")) {
      StringArray steps = StringArray::fromTokens(String(resize_env), ",", "");
      for (const String& step : steps) {
        int ms = step.upToFirstOccurrenceOf(":", false, false).getIntValue();
        String size = step.fromFirstOccurrenceOf(":", false, false);
        MessageManager::getInstance()->runDispatchLoopUntil(ms);
        editor->setSize(size.upToFirstOccurrenceOf("x", false, false).getIntValue(),
                        size.fromFirstOccurrenceOf("x", false, false).getIntValue());
      }
    }
    MessageManager::getInstance()->runDispatchLoopUntil(60000);
    stop = true;
    if (audio.joinable()) audio.join();
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

  if (utility_test) {
    auto find = [&](const char* name) -> AudioProcessorParameter* {
      for (auto* p : params) if (p->getName(64).equalsIgnoreCase(name)) return p;
      return nullptr;
    };
    const char* names[] = { "Utility Gain", "Utility Pan", "Utility Spread", "Utility Haas", "Utility Mono Bass Frequency",
                            "Utility Polarity Invert Left", "Utility Polarity Invert Right", "Utility Swap L/R",
                            "Utility DC Filter", "Utility Mono Bass" };
    AudioProcessorParameter* P[10];
    bool all = true;
    for (int i = 0; i < 10; ++i) { P[i] = find(names[i]); all = all && P[i]; if (!P[i]) std::printf("      missing '%s'\n", names[i]); }
    CHECK(all, "utility parameters found");
    if (!all) return 1;
    enum { kGain, kPan, kSpread, kHaas, kFreq, kInvL, kInvR, kSwap, kDc, kBass };
    const int block = 512;
    plugin->setPlayConfigDetails(2, 2, 48000.0, block);
    plugin->prepareToPlay(48000.0, block);
    auto reset = [&]() {
      P[kGain]->setValue(0.5f); P[kPan]->setValue(0.5f); P[kSpread]->setValue(0.5f); P[kHaas]->setValue(0.5f);
      P[kFreq]->setValue((float) ((std::log2(120.0) - 4.321928) / 4.643856));
      for (int i = kInvL; i <= kBass; ++i) P[i]->setValue(0.0f);
    };
    // Plays two sines (left f_l, right f_r, optional DC offset) and returns the settled output channels.
    struct Out { std::vector<float> l, r; };
    auto play = [&](double f_l, double f_r, float amp_l, float amp_r, float dc = 0.0f) {
      plugin->reset();
      Out out; MidiBuffer midi; AudioBuffer<float> buf(2, block);
      double pl = 0, pr = 0;
      for (int b = 0; b < 140; ++b) {
        for (int i = 0; i < block; ++i) {
          buf.setSample(0, i, amp_l * (float) std::sin(pl) + dc); buf.setSample(1, i, amp_r * (float) std::sin(pr) + dc);
          pl += 2 * MathConstants<double>::pi * f_l / 48000.0; pr += 2 * MathConstants<double>::pi * f_r / 48000.0;
        }
        plugin->processBlock(buf, midi);
        if (b >= 100) for (int i = 0; i < block; ++i) { out.l.push_back(buf.getSample(0, i)); out.r.push_back(buf.getSample(1, i)); }
      }
      return out;
    };
    auto rms = [](const std::vector<float>& v) { double s = 0; for (float x : v) s += x * x; return std::sqrt(s / std::max<size_t>(1, v.size())); };
    const double kVol = 0.5012;   // plugin volume default is -6 dB

    reset();
    Out base = play(1000, 1000, 0.4f, 0.2f);
    double bl = rms(base.l), br = rms(base.r);
    std::printf("      neutral: L rms %.4f (in %.4f), R rms %.4f (in %.4f)\n", bl, 0.4 * kVol / 1.4142, br, 0.2 * kVol / 1.4142);
    CHECK(std::abs(bl / (0.4 * kVol / 1.4142) - 1.0) < 0.03 && std::abs(br / (0.2 * kVol / 1.4142) - 1.0) < 0.03, "utility neutral settings pass audio through");

    P[kGain]->setValue((6.0f + 30.0f) / 60.0f);
    Out g = play(1000, 1000, 0.4f, 0.2f);
    std::printf("      gain +6 dB: ratio %.3f (expect 1.995)\n", rms(g.l) / bl);
    CHECK(std::abs(rms(g.l) / bl - 1.995) < 0.06, "gain +6 dB doubles the level");
    reset();

    P[kPan]->setValue(1.0f);
    Out pn = play(1000, 1000, 0.4f, 0.2f);
    std::printf("      pan +100%%: L %.4f R %.4f (neutral R %.4f)\n", rms(pn.l), rms(pn.r), br);
    CHECK(rms(pn.l) < 0.02 * bl && std::abs(rms(pn.r) / br - 1.0) < 0.03, "pan hard right silences the left, keeps the right");
    reset();

    P[kInvL]->setValue(1.0f);
    Out iv = play(1000, 1000, 0.4f, 0.2f);
    double corr = 0, ne = 0; for (size_t i = 0; i < iv.l.size(); ++i) { corr += iv.l[i] * base.l[i]; ne += base.l[i] * base.l[i]; }
    std::printf("      invert L: correlation with neutral %.3f, R rms ratio %.3f\n", corr / ne, rms(iv.r) / br);
    CHECK(corr / ne < -0.97 && std::abs(rms(iv.r) / br - 1.0) < 0.03, "polarity invert flips only the left channel");
    reset();

    P[kSwap]->setValue(1.0f);
    Out sw = play(1000, 1000, 0.4f, 0.2f);
    std::printf("      swap: L rms %.4f (was R %.4f), R rms %.4f (was L %.4f)\n", rms(sw.l), br, rms(sw.r), bl);
    CHECK(std::abs(rms(sw.l) / br - 1.0) < 0.03 && std::abs(rms(sw.r) / bl - 1.0) < 0.03, "swap L/R exchanges the channels");
    reset();

    P[kSpread]->setValue(0.0f);
    Out sp = play(1000, 1000, 0.4f, 0.0f);   // left only -> becomes mono at 0% spread
    std::printf("      spread 0%%: L %.4f R %.4f\n", rms(sp.l), rms(sp.r));
    CHECK(std::abs(rms(sp.l) - rms(sp.r)) < 0.02 * rms(sp.l) + 1e-6, "spread 0% collapses to mono");
    reset();

    P[kHaas]->setValue((10.0f + 20.0f) / 40.0f);   // +10 ms delays the right channel
    Out hs = play(1000, 1000, 0.4f, 0.4f);
    // 1 kHz sine: a 10 ms delay is exactly 10 cycles, so measure with a 1.1 kHz-like phase test instead using 125 Hz (10 ms = 1.25 cycles)
    reset(); P[kHaas]->setValue((10.0f + 20.0f) / 40.0f);
    Out hs2 = play(125, 125, 0.4f, 0.4f);
    double dot = 0, nl = 0, nr = 0;
    for (size_t i = 0; i < hs2.l.size(); ++i) { dot += hs2.l[i] * hs2.r[i]; nl += hs2.l[i] * hs2.l[i]; nr += hs2.r[i] * hs2.r[i]; }
    double c = dot / std::sqrt(nl * nr);
    std::printf("      haas +10 ms @125 Hz: L/R correlation %.3f (expect cos(90 deg) = 0)\n", c);
    CHECK(std::abs(c) < 0.08 && rms(hs.r) > 0.9 * bl, "haas delays the right channel by 10 ms");
    reset();

    P[kBass]->setValue(1.0f);
    Out mb = play(40, 40, 0.4f, -0.4f);   // out-of-phase bass below the crossover must cancel
    Out mh = play(600, 600, 0.4f, -0.4f);   // highs above the 120 Hz crossover are left alone
    std::printf("      mono bass @120 Hz: 40 Hz anti-phase -> L rms %.4f; 600 Hz anti-phase -> L rms %.4f (neutral %.4f)\n",
                rms(mb.l), rms(mh.l), 0.4 * kVol / 1.4142);
    CHECK(rms(mb.l) < 0.15 * 0.4 * kVol / 1.4142, "mono bass cancels out-of-phase lows");
    CHECK(rms(mh.l) > 0.9 * 0.4 * kVol / 1.4142, "mono bass leaves the highs alone");
    reset();

    P[kDc]->setValue(1.0f);
    Out dc = play(1000, 1000, 0.0f, 0.0f, 0.3f);
    double mean = 0; for (float x : dc.l) mean += x; mean /= dc.l.size();
    std::printf("      DC filter: mean after settling %.5f (input 0.3)\n", mean);
    CHECK(std::abs(mean) < 0.01, "DC filter removes a constant offset");
    for (int i = 0; i < 200; ++i) { AudioBuffer<float> b(2, block); MidiBuffer m; plugin->processBlock(b, m); }
    std::printf(failures ? "\n%d FAILURE(S)\n" : "\nALL CHECKS PASSED\n", failures);
    return failures ? 1 : 0;
  }

  if (shifter_test) {
    auto find = [&](const char* name) -> AudioProcessorParameter* {
      for (auto* p : params) if (p->getName(64).equalsIgnoreCase(name)) return p;
      return nullptr;
    };
    auto *shift = find("Freq Shifter Shift"), *unit = find("Freq Shifter Unit"), *range = find("Freq Shifter Range"),
         *period = find("Freq Shifter Range (Seconds)"), *stereo = find("Freq Shifter Stereo"),
         *feedback = find("Freq Shifter Feedback"), *fbdir = find("Freq Shifter Feedback Direction"),
         *mix = find("Freq Shifter Mix"), *quality = find("Freq Shifter Quality");
    CHECK(shift && unit && range && period && stereo && feedback && fbdir && mix && quality, "freq shifter parameters found");
    if (!(shift && unit && range && period && stereo && feedback && fbdir && mix && quality)) return 1;
    const int block = 512;
    plugin->setPlayConfigDetails(2, 2, 48000.0, block);
    plugin->prepareToPlay(48000.0, block);
    const double kVol = 0.5012;
    auto reset = [&]() {
      mix->setValue(1.0f); shift->setValue(0.5f); unit->setValue(0.0f); stereo->setValue(0.5f);
      feedback->setValue(0.0f); fbdir->setValue(0.5f); quality->setValue(1.0f / 3.0f);
      range->setValue((float) (std::log2(400.0) / 13.0));
    };
    struct Out { std::vector<float> l, r; };
    auto play = [&](double freq) {
      plugin->reset();
      Out out; MidiBuffer midi; AudioBuffer<float> buf(2, block);
      double ph = 0;
      for (int b = 0; b < 140; ++b) {
        for (int i = 0; i < block; ++i) {
          float v = 0.4f * (float) std::sin(ph); ph += 2 * MathConstants<double>::pi * freq / 48000.0;
          buf.setSample(0, i, v); buf.setSample(1, i, v);
        }
        plugin->processBlock(buf, midi);
        if (b >= 100) for (int i = 0; i < block; ++i) { out.l.push_back(buf.getSample(0, i)); out.r.push_back(buf.getSample(1, i)); }
      }
      return out;
    };
    auto amp = [](const std::vector<float>& v, double f) {   // single-bin DFT amplitude (Hann)
      std::complex<double> a = 0; double w = 0; const int n = (int) v.size();
      for (int i = 0; i < n; ++i) {
        double h = 0.5 - 0.5 * std::cos(2 * MathConstants<double>::pi * i / n);
        a += h * v[i] * std::polar(1.0, -2 * MathConstants<double>::pi * f * i / 48000.0); w += h;
      }
      return 2.0 * std::abs(a) / w;
    };
    const double in_amp = 0.4 * kVol;

    reset(); shift->setValue(0.5f);   // 0 shift
    Out n0 = play(1000);
    std::printf("      neutral: 1 kHz amplitude %.4f (expect %.4f)\n", amp(n0.l, 1000), in_amp);
    CHECK(std::abs(amp(n0.l, 1000) / in_amp - 1.0) < 0.05, "shift 0 passes the signal through");

    reset(); shift->setValue(0.75f);   // +50% of a 400 Hz range = +200 Hz
    Out up = play(1000);
    std::printf("      +200 Hz: 1200 Hz %.4f, 800 Hz (image) %.4f, 1000 Hz %.4f\n", amp(up.l, 1200), amp(up.l, 800), amp(up.l, 1000));
    CHECK(amp(up.l, 1200) > 0.9 * in_amp && amp(up.l, 800) < 0.03 * in_amp && amp(up.l, 1000) < 0.03 * in_amp,
          "positive shift moves 1 kHz to 1.2 kHz, no mirror image");

    reset(); shift->setValue(0.25f);   // -200 Hz
    Out down = play(1000);
    std::printf("      -200 Hz: 800 Hz %.4f, 1200 Hz (image) %.4f\n", amp(down.l, 800), amp(down.l, 1200));
    CHECK(amp(down.l, 800) > 0.9 * in_amp && amp(down.l, 1200) < 0.03 * in_amp, "negative shift moves 1 kHz to 800 Hz");

    reset(); unit->setValue(0.2f); period->setValue((float) ((std::log2(0.005) + 13.0) / 13.0)); shift->setValue(1.0f);   // +100% of 1/5ms = 200 Hz
    Out sec = play(1000);
    std::printf("      Seconds unit, 5 ms range, +100%%: 1200 Hz %.4f\n", amp(sec.l, 1200));
    CHECK(amp(sec.l, 1200) > 0.9 * in_amp, "Seconds unit: a 5 ms range shifts by 200 Hz");

    reset(); shift->setValue(0.5f); stereo->setValue(1.0f);   // +-100 Hz between channels (25% of 400 Hz)
    Out st = play(1000);
    std::printf("      stereo +100%%: L 1100 Hz %.4f, R 900 Hz %.4f\n", amp(st.l, 1100), amp(st.r, 900));
    CHECK(amp(st.l, 1100) > 0.9 * in_amp && amp(st.r, 900) > 0.9 * in_amp, "stereo offset shifts the channels in opposite directions");

    // Quality (pole count): every setting must shift correctly; image rejection improves with more poles.
    const double limits[4] = { 0.15, 0.05, 0.02, 0.02 };
    const char* names[4] = { "Rough", "Normal", "Better", "Excellent" };
    for (int q = 0; q < 4; ++q) {
      reset(); quality->setValue(q / 3.0f); shift->setValue(0.75f);
      Out hi = play(1000);
      double main_hi = amp(hi.l, 1200) / in_amp, image_hi = amp(hi.l, 800) / in_amp;
      std::printf("      quality %-9s: 1 kHz -> 1200 Hz %.3f, image 800 Hz %.4f\n", names[q], main_hi, image_hi);
      CHECK(std::abs(main_hi - 1.0) < 0.05 && image_hi < limits[q], "quality setting shifts with acceptable image rejection");
    }

    reset(); feedback->setValue(1.0f);
    Out fb = play(1000);
    double peak = 0; for (float x : fb.l) peak = std::max(peak, (double) std::fabs(x));
    std::printf("      feedback 100%%: peak %.3f\n", peak);
    CHECK(std::isfinite(peak) && peak < 2.0, "full feedback stays bounded");
    reset(); feedback->setValue(0.6f); fbdir->setValue(1.0f);
    Out fd = play(1000);
    peak = 0; for (float x : fd.l) peak = std::max(peak, (double) std::fabs(x));
    std::printf("      feedback 60%% with direction +100%%: peak %.3f\n", peak);
    CHECK(std::isfinite(peak) && peak < 2.0, "feedback direction stays bounded");

    std::printf(failures ? "\n%d FAILURE(S)\n" : "\nALL CHECKS PASSED\n", failures);
    return failures ? 1 : 0;
  }

  if (vibrato_test) {
    auto find = [&](const char* name) -> AudioProcessorParameter* {
      for (auto* p : params) if (p->getName(64).equalsIgnoreCase(name)) return p;
      return nullptr;
    };
    auto *mod_a = find("Vibrato Mod Type A"), *mod_b = find("Vibrato Mod Type B"), *rate_a = find("Vibrato Rate A"),
         *rate_b = find("Vibrato Rate B"), *amt_a = find("Vibrato Amount A"), *amt_b = find("Vibrato Amount B"),
         *sync_a = find("Vibrato Rate A Unit");
    CHECK(mod_a && mod_b && rate_a && rate_b && amt_a && amt_b && sync_a, "vibrato parameters found");
    if (!(mod_a && mod_b && rate_a && rate_b && amt_a && amt_b && sync_a)) return 1;
    const int block = 512;
    plugin->setPlayConfigDetails(2, 2, 48000.0, block);
    plugin->prepareToPlay(48000.0, block);
    const double kVol = 0.5012;
    auto hz = [](double f) { return (float) ((std::log2(f) + 3.0) / 8.4); };   // rate knob range -3..5.4 (log2 Hz)
    auto reset = [&]() {
      mod_a->setValue(0.0f); mod_b->setValue(0.0f); sync_a->setValue(0.0f);
      rate_a->setValue(hz(5.0)); rate_b->setValue(hz(5.0));
      amt_a->setValue(0.0f); amt_b->setValue(0.0f);
    };
    auto play = [&](double freq, int blocks) {
      plugin->reset();
      std::vector<float> out; MidiBuffer midi; AudioBuffer<float> buf(2, block);
      double ph = 0;
      for (int b = 0; b < blocks; ++b) {
        for (int i = 0; i < block; ++i) {
          float v = 0.4f * (float) std::sin(ph); ph += 2 * MathConstants<double>::pi * freq / 48000.0;
          buf.setSample(0, i, v); buf.setSample(1, i, v);
        }
        plugin->processBlock(buf, midi);
        if (b >= blocks - 190) for (int i = 0; i < block; ++i) out.push_back(buf.getSample(0, i));   // last ~2 s
      }
      return out;
    };
    auto amp = [](const std::vector<float>& v, double f) {
      std::complex<double> a = 0; double w = 0; const int n = (int) v.size();
      for (int i = 0; i < n; ++i) {
        double h = 0.5 - 0.5 * std::cos(2 * MathConstants<double>::pi * i / n);
        a += h * v[i] * std::polar(1.0, -2 * MathConstants<double>::pi * f * i / 48000.0); w += h;
      }
      return 2.0 * std::abs(a) / w;
    };
    const double in_amp = 0.4 * kVol;

    reset();
    auto n0 = play(1000, 260);
    std::printf("      amount 0: 1 kHz amplitude %.4f (expect %.4f)\n", amp(n0, 1000), in_amp);
    CHECK(std::abs(amp(n0, 1000) / in_amp - 1.0) < 0.05, "amount 0 passes the signal through");

    // 10 cents at 5 Hz on 1 kHz: peak deviation 5.8 Hz, modulation index ~1.16 -> J0 = 0.69, J1 = 0.50.
    reset(); amt_a->setValue(10.0f / 200.0f);
    auto v1 = play(1000, 260);
    double c = amp(v1, 1000) / in_amp, sb = 0.5 * (amp(v1, 1005) + amp(v1, 995)) / in_amp;
    std::printf("      10 cents @5 Hz: carrier %.3f (expect ~0.69), sidebands %.3f (expect ~0.50)\n", c, sb);
    CHECK(std::abs(c - 0.69) < 0.08 && std::abs(sb - 0.50) < 0.12, "10 cents at 5 Hz produces the expected FM spectrum");

    // Doubling the rate moves the sidebands out; the index halves (J1(0.58) = 0.28).
    reset(); amt_a->setValue(10.0f / 200.0f); rate_a->setValue(hz(10.0));
    auto v2 = play(1000, 260);
    double sb2 = 0.5 * (amp(v2, 1010) + amp(v2, 990)) / in_amp;
    std::printf("      10 cents @10 Hz: sidebands at +-10 Hz %.3f (expect ~0.28)\n", sb2);
    CHECK(std::abs(sb2 - 0.28) < 0.1 && amp(v2, 1005) / in_amp < 0.05, "rate sets the sideband spacing");

    // A and B add: 5 cents + 5 cents, same type and rate, equals 10 cents.
    reset(); amt_a->setValue(5.0f / 200.0f); amt_b->setValue(5.0f / 200.0f);
    auto v3 = play(1000, 260);
    double sb3 = 0.5 * (amp(v3, 1005) + amp(v3, 995)) / in_amp;
    std::printf("      5 + 5 cents: sidebands %.3f (expect ~0.50)\n", sb3);
    CHECK(std::abs(sb3 - 0.50) < 0.12, "modulators A and B add");

    // Large depth on every shape must stay bounded and finite.
    for (int type = 0; type < 6; ++type) {
      reset(); mod_a->setValue(type / 5.0f); mod_b->setValue(((type + 3) % 6) / 5.0f);
      amt_a->setValue(1.0f); amt_b->setValue(1.0f); rate_a->setValue(hz(30.0)); rate_b->setValue(hz(0.3));
      auto v = play(300, 140);
      double peak = 0; for (float x : v) peak = std::max(peak, (double) std::fabs(x));
      CHECK(std::isfinite(peak) && peak < 0.6 && peak > 0.05, "extreme settings stay bounded and audible");
    }

    // Tempo sync switch must not break the output.
    reset(); amt_a->setValue(0.25f); sync_a->setValue(1.0f);
    auto v4 = play(1000, 140);
    double peak4 = 0; for (float x : v4) peak4 = std::max(peak4, (double) std::fabs(x));
    CHECK(std::isfinite(peak4) && peak4 > 0.1 && peak4 < 0.6, "tempo sync produces a stable output");

    std::printf(failures ? "\n%d FAILURE(S)\n" : "\nALL CHECKS PASSED\n", failures);
    return failures ? 1 : 0;
  }

  if (morph_test) {
    auto find = [&](const char* name) -> AudioProcessorParameter* {
      for (auto* p : params) if (p->getName(64).equalsIgnoreCase(name)) return p;
      return nullptr;
    };
    auto *spec = find("Morph Spectral Mode"), *warp = find("Morph Warp Mode"), *amt = find("Morph Spectral Morph"),
         *wamt = find("Morph Distortion");
    CHECK(spec && warp && amt && wamt, "morph parameters found");
    if (!(spec && warp && amt && wamt)) return 1;
    const int block = 512;
    plugin->setPlayConfigDetails(2, 2, 48000.0, block);
    plugin->prepareToPlay(48000.0, block);
    auto run = [&](float& energy, bool& finite) {
      plugin->reset();
      MidiBuffer midi; AudioBuffer<float> buf(2, block); double ph = 0; energy = 0; finite = true;
      for (int b = 0; b < 120; ++b) {
        for (int i = 0; i < block; ++i) {
          float v = 0.4f * (float) std::sin(ph) + 0.1f * (float) std::sin(ph * 3.1); ph += 2 * MathConstants<double>::pi * 330.0 / 48000.0;
          buf.setSample(0, i, v); buf.setSample(1, i, v);
        }
        plugin->processBlock(buf, midi);
        for (int i = 0; i < block; ++i) {
          float v = buf.getSample(0, i);
          if (!std::isfinite(v) || std::abs(v) > 8.0f) finite = false;
          if (b >= 100) energy += v * v;
        }
      }
    };
    float e; bool fin;
    int n_spec = spec->getNumSteps(), n_warp = warp->getNumSteps();
    std::printf("      %d spectral modes, %d warp modes\n", n_spec, n_warp);
    CHECK(n_spec > 8 && n_warp > 4, "mode lists are populated");
    amt->setValue(0.7f); wamt->setValue(0.7f);
    warp->setValue(0.0f);
    spec->setValue(0.0f); run(e, fin);
    CHECK(fin && e > 0.01f, "None: passes audio, no latency");
    CHECK(plugin->getLatencySamples() == 0, "no latency with spectral mode None");
    for (int m = 1; m < n_spec; ++m) {
      spec->setValue((float) m / (float) (n_spec - 1)); run(e, fin);
      String name = spec->getText(spec->getValue(), 32);
      std::printf("      spectral %2d %-22s energy %.4f latency %d\n", m, name.toRawUTF8(), e, plugin->getLatencySamples());
      CHECK(fin, (String("spectral ") + name + " stays finite").toRawUTF8());
    }
    spec->setValue(1.0f / (float) (n_spec - 1)); run(e, fin);
    CHECK(plugin->getLatencySamples() > 0, "spectral mode reports FFT latency to the host");
    spec->setValue(0.0f);
    for (int m = 1; m < n_warp; ++m) {
      warp->setValue((float) m / (float) (n_warp - 1)); run(e, fin);
      String name = warp->getText(warp->getValue(), 32);
      std::printf("      warp %2d %-22s energy %.4f\n", m, name.toRawUTF8(), e);
      CHECK(fin, (String("warp ") + name + " stays finite").toRawUTF8());
    }
    std::printf(failures ? "\n%d FAILURE(S)\n" : "\nALL CHECKS PASSED\n", failures);
    return failures ? 1 : 0;
  }

  if (tremolo_test) {
    auto find = [&](const char* name) -> AudioProcessorParameter* {
      for (auto* p : params) if (p->getName(64).equalsIgnoreCase(name)) return p;
      return nullptr;
    };
    auto *mod_a = find("Tremolo Mod Type A"), *mod_b = find("Tremolo Mod Type B"), *rate_a = find("Tremolo Rate A"),
         *rate_b = find("Tremolo Rate B"), *amt_a = find("Tremolo Amount A"), *amt_b = find("Tremolo Amount B"),
         *unit_a = find("Tremolo Rate A Unit");
    CHECK(mod_a && mod_b && rate_a && rate_b && amt_a && amt_b && unit_a, "tremolo parameters found");
    if (!(mod_a && mod_b && rate_a && rate_b && amt_a && amt_b && unit_a)) return 1;
    const int block = 512;
    plugin->setPlayConfigDetails(2, 2, 48000.0, block);
    plugin->prepareToPlay(48000.0, block);
    const double kVol = 0.5012;
    auto hz = [](double f) { return (float) ((std::log2(f) + 3.0) / 8.4); };   // rate knob range -3..5.4 (log2 Hz)
    auto reset = [&]() {
      mod_a->setValue(0.0f); mod_b->setValue(0.0f); unit_a->setValue(0.0f);
      rate_a->setValue(hz(5.0)); rate_b->setValue(hz(5.0));
      amt_a->setValue(0.0f); amt_b->setValue(0.0f);
    };
    auto play = [&](double freq, int blocks) {
      plugin->reset();
      std::vector<float> out; MidiBuffer midi; AudioBuffer<float> buf(2, block);
      double ph = 0;
      for (int b = 0; b < blocks; ++b) {
        for (int i = 0; i < block; ++i) {
          float v = 0.4f * (float) std::sin(ph); ph += 2 * MathConstants<double>::pi * freq / 48000.0;
          buf.setSample(0, i, v); buf.setSample(1, i, v);
        }
        plugin->processBlock(buf, midi);
        if (b >= blocks - 190) for (int i = 0; i < block; ++i) out.push_back(buf.getSample(0, i));   // last ~2 s
      }
      return out;
    };
    auto amp = [](const std::vector<float>& v, double f) {
      std::complex<double> a = 0; double w = 0; const int n = (int) v.size();
      for (int i = 0; i < n; ++i) {
        double h = 0.5 - 0.5 * std::cos(2 * MathConstants<double>::pi * i / n);
        a += h * v[i] * std::polar(1.0, -2 * MathConstants<double>::pi * f * i / 48000.0); w += h;
      }
      return 2.0 * std::abs(a) / w;
    };
    const double in_amp = 0.4 * kVol;

    reset();
    auto n0 = play(1000, 260);
    std::printf("      amount 0: 1 kHz amplitude %.4f (expect %.4f)\n", amp(n0, 1000), in_amp);
    CHECK(std::abs(amp(n0, 1000) / in_amp - 1.0) < 0.05, "amount 0 passes the signal through");

    // 100% sine at 5 Hz: gain = 0.5 + 0.5 sin -> carrier 0.5, sidebands 0.25 each.
    reset(); amt_a->setValue(1.0f);
    auto v1 = play(1000, 260);
    double c = amp(v1, 1000) / in_amp, sb = 0.5 * (amp(v1, 1005) + amp(v1, 995)) / in_amp;

    std::printf("      100%% @5 Hz: carrier %.3f (expect 0.5), sidebands %.3f (expect 0.25)\n", c, sb);
    CHECK(std::abs(c - 0.5) < 0.04 && std::abs(sb - 0.25) < 0.04, "100% depth gives the expected spectrum");

    // 50%: gain = 0.75 + 0.25 sin -> carrier 0.75, sidebands 0.125.
    reset(); amt_a->setValue(0.5f);
    auto v2 = play(1000, 260);
    c = amp(v2, 1000) / in_amp; sb = 0.5 * (amp(v2, 1005) + amp(v2, 995)) / in_amp;
    std::printf("      50%% @5 Hz: carrier %.3f (expect 0.75), sidebands %.3f (expect 0.125)\n", c, sb);
    CHECK(std::abs(c - 0.75) < 0.04 && std::abs(sb - 0.125) < 0.03, "50% depth gives the expected spectrum");

    // Rate sets the sideband spacing.
    reset(); amt_a->setValue(1.0f); rate_a->setValue(hz(10.0));
    auto v3 = play(1000, 260);
    sb = 0.5 * (amp(v3, 1010) + amp(v3, 990)) / in_amp;
    std::printf("      100%% @10 Hz: sidebands at +-10 Hz %.3f (expect 0.25)\n", sb);
    CHECK(std::abs(sb - 0.25) < 0.04 && amp(v3, 1005) / in_amp < 0.03, "rate sets the sideband spacing");

    // A and B add: 50% + 50% with the same shape and rate equals 100%.
    reset(); amt_a->setValue(0.5f); amt_b->setValue(0.5f);
    auto v4 = play(1000, 260);
    sb = 0.5 * (amp(v4, 1005) + amp(v4, 995)) / in_amp;
    std::printf("      50%% + 50%%: sidebands %.3f (expect 0.25)\n", sb);
    CHECK(std::abs(sb - 0.25) < 0.04, "modulators A and B add");

    for (int type = 0; type < 6; ++type) {
      reset(); mod_a->setValue(type / 5.0f); mod_b->setValue(((type + 3) % 6) / 5.0f);
      amt_a->setValue(1.0f); amt_b->setValue(1.0f); rate_a->setValue(hz(30.0)); rate_b->setValue(hz(0.3));
      auto v = play(300, 140);
      double peak = 0; for (float x : v) peak = std::max(peak, (double) std::fabs(x));
      CHECK(std::isfinite(peak) && peak < 0.25, "extreme settings never exceed the input level");
    }

    reset(); amt_a->setValue(1.0f); unit_a->setValue(1.0f);
    auto v5 = play(1000, 140);
    double peak5 = 0; for (float x : v5) peak5 = std::max(peak5, (double) std::fabs(x));
    CHECK(std::isfinite(peak5) && peak5 > 0.05 && peak5 < 0.25, "tempo unit produces a stable output");

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
    if (!mix) for (auto* p : params) if (p->getNumSteps() > 100 && p->getName(64).contains(" ") && !p->getName(64).startsWithIgnoreCase("volume")) { target = p; break; }
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
