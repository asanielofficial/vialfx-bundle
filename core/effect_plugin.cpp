// VialFX - Licensed under GPL-3.0-or-later. Based on Vital's SynthPlugin by Matt Tytel (GPL-3.0).
#include "effect_plugin.h"

#include "effect_editor.h"
#include "load_save.h"
#include "sound_engine.h"
#include "synth_gui_interface.h"
#include "synth_parameters.h"

using vitalfx::EffectInfo;
using vitalfx::EffectKind;

AudioProcessor::BusesProperties EffectPlugin::makeBuses() {
  return BusesProperties()
      .withInput("Input", AudioChannelSet::stereo(), true)
      .withOutput("Output", AudioChannelSet::stereo(), true);
}

EffectPlugin::EffectPlugin() : AudioProcessor(makeBuses()) {
  const EffectInfo& info = vitalfx::thisEffect();

  // Only this effect's controls, the global bypass and the output volume are exposed to the host.
  int num_params = vital::Parameters::getNumParameters();
  for (int i = 0; i < num_params; ++i) {
    const vital::ValueDetails* details = vital::Parameters::getDetails(i);
    const std::string name = details->name;
    if (controls_.count(name) == 0 || !(isEffectParameter(name) || name == "bypass" || name == "volume"))
      continue;

    ValueBridge* bridge = new ValueBridge(name, controls_[name]);
    bridge->setListener(this);
    bridge_lookup_[name] = bridge;
    addParameter(bridge);
  }

  bypass_parameter_ = bridge_lookup_.count("bypass") ? bridge_lookup_["bypass"] : nullptr;

  // Vital's effects-only engine hosts the whole chain; this plugin turns on exactly one effect.
  if (controls_.count(info.on_control))
    controls_[info.on_control]->set(1.0f);

  input_buffer_.resize(vital::kMaxBufferSize);
  checkOversampling();
}

EffectPlugin::~EffectPlugin() {
  midi_manager_ = nullptr;
  keyboard_state_ = nullptr;
}

bool EffectPlugin::isEffectParameter(const std::string& name) const {
  const std::string prefix = vitalfx::thisEffect().control_prefix;
  return name.compare(0, prefix.size(), prefix) == 0;
}

SynthGuiInterface* EffectPlugin::getGuiInterface() {
  if (AudioProcessorEditor* editor = getActiveEditor())
    return dynamic_cast<SynthGuiInterface*>(editor);
  return nullptr;
}

void EffectPlugin::beginChangeGesture(const std::string& name) {
  if (bridge_lookup_.count(name))
    bridge_lookup_[name]->beginChangeGesture();
}

void EffectPlugin::endChangeGesture(const std::string& name) {
  if (bridge_lookup_.count(name))
    bridge_lookup_[name]->endChangeGesture();
}

void EffectPlugin::setValueNotifyHost(const std::string& name, vital::mono_float value) {
  if (bridge_lookup_.count(name)) {
    vital::mono_float plugin_value = bridge_lookup_[name]->convertToPluginValue(value);
    bridge_lookup_[name]->setValueNotifyHost(plugin_value);
  }
}

const CriticalSection& EffectPlugin::getCriticalSection() {
  return getCallbackLock();
}

void EffectPlugin::pauseProcessing(bool pause) {
  suspendProcessing(pause);
}

const String EffectPlugin::getName() const {
  return String("Vial ") + vitalfx::thisEffect().display_name;
}

const String EffectPlugin::getProgramName(int) {
  if (SynthGuiInterface* editor = getGuiInterface())
    if (editor->getSynth())
      return editor->getSynth()->getPresetName();
  return {};
}

double EffectPlugin::getTailLengthSeconds() const {
  switch (static_cast<EffectKind>(VITALFX_EFFECT)) {
    case EffectKind::kReverb: return 15.0;
    case EffectKind::kDelay: return 10.0;
    case EffectKind::kChorus:
    case EffectKind::kFlanger:
    case EffectKind::kPhaser: return 2.0;
    default: return 0.0;
  }
}

bool EffectPlugin::isBusesLayoutSupported(const BusesLayout& layouts) const {
  return layouts.getMainInputChannelSet() == AudioChannelSet::stereo() &&
         layouts.getMainOutputChannelSet() == AudioChannelSet::stereo();
}

void EffectPlugin::prepareToPlay(double sample_rate, int) {
  engine_->setSampleRate(static_cast<int>(sample_rate));
  engine_->updateAllModulationSwitches();
  midi_manager_->setSampleRate(sample_rate);
  engine_->checkOversampling();
}

void EffectPlugin::releaseResources() { }

void EffectPlugin::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi_messages) {
  ScopedNoDenormals no_denormals;

  if (bypass_parameter_ && bypass_parameter_->getValue() > 0.5f) {
    processBlockBypassed(buffer, midi_messages);
    return;
  }

  const int total_samples = buffer.getNumSamples();
  if (total_samples == 0)
    return;

  // Make sure only this plugin's effect runs, whatever a loaded preset says about the others.
  {
    const EffectInfo& mine = vitalfx::thisEffect();
    for (int k = 0; k < 9; ++k) {
      const EffectInfo& other = vitalfx::getEffectInfo(static_cast<EffectKind>(k));
      if (&other != &mine) {
        auto it = controls_.find(other.on_control);
        if (it != controls_.end() && it->second->value() != 0.0f)
          it->second->set(0.0f);
      }
    }
  }

  if (AudioPlayHead* play_head = getPlayHead()) {
    AudioPlayHead::CurrentPositionInfo position;
    if (play_head->getCurrentPosition(position)) {
      if (position.bpm > 0.0)
        engine_->setBpm(static_cast<vital::mono_float>(position.bpm));
      if (position.isPlaying && position.bpm > 0.0)
        last_seconds_time_ = position.ppqPosition / (position.bpm / 60.0);
    }
  }

  processModulationChanges();

  const bool use_midi = vitalfx::thisEffect().is_filter && settings_.keytrack_from_midi;
  if (use_midi)
    processKeyboardEvents(midi_messages, total_samples);

  const int num_channels = std::min(2, buffer.getNumChannels());
  const double sample_time = 1.0 / AudioProcessor::getSampleRate();

  for (int offset = 0; offset < total_samples;) {
    const int num_samples = std::min<int>(total_samples - offset, vital::kMaxBufferSize);

    // Vital carries stereo audio in the first two lanes of each poly_float.
    const float* left = buffer.getReadPointer(0, offset);
    const float* right = num_channels > 1 ? buffer.getReadPointer(1, offset) : left;
    float* in = reinterpret_cast<float*>(input_buffer_.data());
    for (int i = 0; i < num_samples; ++i) {
      in[vital::poly_float::kSize * i + 0] = left[i];
      in[vital::poly_float::kSize * i + 1] = right[i];
      in[vital::poly_float::kSize * i + 2] = 0.0f;
      in[vital::poly_float::kSize * i + 3] = 0.0f;
    }

    engine_->correctToTime(last_seconds_time_);
    if (use_midi)
      processMidi(midi_messages, offset, offset + num_samples);
    processAudioWithInput(&buffer, input_buffer_.data(), num_channels, num_samples, offset);

    last_seconds_time_ += num_samples * sample_time;
    offset += num_samples;
  }
}

AudioProcessorEditor* EffectPlugin::createEditor() {
  return new EffectEditor(*this);
}

void EffectPlugin::parameterChanged(std::string name, vital::mono_float value) {
  valueChangedExternal(name, value);
}

void EffectPlugin::setKeytrackFromMidi(bool enabled) {
  settings_.keytrack_from_midi = enabled;
}

void EffectPlugin::getStateInformation(MemoryBlock& dest_data) {
  json data = LoadSave::stateToJson(this, getCallbackLock());

  data["vitalfx"]["effect"] = vitalfx::thisEffect().display_name;
  data["vitalfx"]["keytrack_from_midi"] = settings_.keytrack_from_midi;

  MemoryOutputStream stream;
  stream.writeString(String(data.dump()));
  dest_data.append(stream.getData(), stream.getDataSize());
}

void EffectPlugin::setStateInformation(const void* data, int size_in_bytes) {
  MemoryInputStream stream(data, static_cast<size_t>(size_in_bytes), false);
  String data_string = stream.readEntireStreamAsString();

  pauseProcessing(true);
  try {
    json json_data = json::parse(data_string.toStdString());
    if (json_data.count("vitalfx")) {
      const json& fx = json_data["vitalfx"];
      if (fx.count("keytrack_from_midi"))
        settings_.keytrack_from_midi = fx["keytrack_from_midi"].get<bool>();
    }
    LoadSave::jsonToState(this, save_info_, json_data);
    checkOversampling();
  }
  catch (const json::exception&) {
    // Corrupt state from a host session: keep current values rather than interrupting the user.
  }
  pauseProcessing(false);

  if (SynthGuiInterface* editor = getGuiInterface())
    editor->updateFullGui();
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new EffectPlugin();
}
