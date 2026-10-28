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

    std::unique_ptr<OversampleBox> oversample_;
    std::unique_ptr<DisplayBox> display_;
    std::unique_ptr<KeytrackBox> keytrack_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsSection)
};
