// VialFX - Licensed under GPL-3.0-or-later. Based on Vital's SynthEditor by Matt Tytel (GPL-3.0).
#pragma once

#include "JuceHeader.h"

#include "border_bounds_constrainer.h"
#include "effect_plugin.h"
#include "full_interface.h"
#include "synth_gui_interface.h"

class EffectEditor : public AudioProcessorEditor, public SynthGuiInterface {
  public:
    explicit EffectEditor(EffectPlugin&);

    void paint(Graphics&) override { }
    void resized() override;
    void setScaleFactor(float newScale) override;
    void updateFullGui() override;

  private:
    EffectPlugin& plugin_;
    BorderBoundsConstrainer constrainer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectEditor)
};
