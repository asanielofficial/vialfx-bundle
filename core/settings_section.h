// VialFX - Licensed under GPL-3.0-or-later.
// The Oversampling and Display panels are adapted from Vital's Advanced tab (Matt Tytel, GPL-3.0).
#pragma once

#include "JuceHeader.h"

#include "synth_section.h"

class OpenGlToggleButton;
class TextSelector;

// Settings page opened by the cog in the top bar:
//   OVERSAMPLING   1x / 2x / 4x / 8x (Vital's own control, saved with the session)
//   DISPLAY        Frequency units (Semitones / Hz) and Skin, exactly as in Vital
//   KEYTRACK       filter plugin only: follow incoming MIDI notes
//   FFT SIZE       Morph only: 256 .. 32768, with the latency it adds
//   MIDI / TUNING / EXPRESSION   Resonator only: MIDI follow, MPE and bend range; tuning, A4 reference, glide;
//                  where slide and pressure act
class SettingsSection : public SynthSection {
  public:
    SettingsSection();
    ~SettingsSection() override;

    void paintBackground(Graphics& g) override;
    void resized() override;
    void setAllValues(vital::control_map& controls) override;
    void buttonClicked(Button* clicked_button) override;
    void sliderValueChanged(Slider* changed_slider) override;
    void parentHierarchyChanged() override;

  private:
    class OversampleBox;
    class DisplayBox;
    class KeytrackBox;
    class FftBox;
    class ResonatorBox;

    std::unique_ptr<OversampleBox> oversample_;
    std::unique_ptr<DisplayBox> display_;
    std::unique_ptr<KeytrackBox> keytrack_;
    std::unique_ptr<FftBox> fft_;
    std::unique_ptr<ResonatorBox> midi_box_, tuning_box_, expression_box_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsSection)
};
