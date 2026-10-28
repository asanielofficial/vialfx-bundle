// VialFX - Licensed under GPL-3.0-or-later. Based on Vital's SynthPlugin by Matt Tytel (GPL-3.0).
#pragma once

#include "JuceHeader.h"

#include <map>
#include <vector>

#include "effect_info.h"
#include "effect_state.h"
#include "synth_base.h"
#include "value_bridge.h"

class EffectPlugin : public SynthBase, public AudioProcessor, public ValueBridge::Listener {
  public:
    EffectPlugin();
    ~EffectPlugin() override;

    // SynthBase
    SynthGuiInterface* getGuiInterface() override;
    void beginChangeGesture(const std::string& name) override;
    void endChangeGesture(const std::string& name) override;
    void setValueNotifyHost(const std::string& name, vital::mono_float value) override;
    const CriticalSection& getCriticalSection() override;
    void pauseProcessing(bool pause) override;

    // AudioProcessor
    const String getName() const override;
    void prepareToPlay(double sample_rate, int buffer_size) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(AudioBuffer<float>&, MidiBuffer&) override;
    using AudioProcessor::processBlock;

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    bool acceptsMidi() const override { return vitalfx::thisEffect().is_filter; }
    bool producesMidi() const override { return false; }
    bool supportsMPE() const override { return false; }
    double getTailLengthSeconds() const override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override { }
    const String getProgramName(int) override;
    void changeProgramName(int, const String&) override { }

    void getStateInformation(MemoryBlock& dest_data) override;
    void setStateInformation(const void* data, int size_in_bytes) override;
    AudioProcessorParameter* getBypassParameter() const override { return bypass_parameter_; }

    void parameterChanged(std::string name, vital::mono_float value) override;

    // Plugin-level settings (settings popup in the editor).
    vitalfx::EffectSettings& getSettings() { return settings_; }
    void setKeytrackFromMidi(bool enabled);

    bool isEffectParameter(const std::string& name) const;

  private:
    static BusesProperties makeBuses();

    ValueBridge* bypass_parameter_ = nullptr;
    double last_seconds_time_ = 0.0;
    vitalfx::EffectSettings settings_;
    std::map<std::string, ValueBridge*> bridge_lookup_;
    std::vector<vital::poly_float> input_buffer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectPlugin)
};
