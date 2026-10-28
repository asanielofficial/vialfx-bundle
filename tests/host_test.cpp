// VialFX - Licensed under GPL-3.0-or-later.
// Minimal VST3 host used in CI: loads a built plugin, checks parameters, processes audio,
// checks bypass and state round-trips. Usage: host_test <path-to.vst3> [--editor]
#include <JuceHeader.h>
#include <cmath>
#include <cstdio>

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
    if (n.containsIgnoreCase("dry") && n.containsIgnoreCase("wet")) mix = p;
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
    MessageManager::getInstance()->runDispatchLoopUntil(15000);
    editor->removeFromDesktop();
    return 0;
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
  if (mix) {
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
