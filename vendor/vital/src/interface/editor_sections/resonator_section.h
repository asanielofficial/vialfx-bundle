// VialFX - Resonator section. Licensed under GPL-3.0-or-later.
#pragma once

#include "JuceHeader.h"
#include "synth_section.h"

class ResonatorPeaksViewer;
class ResonatorTiltViewer;
class SynthButton;
class SynthSlider;
class TextSelector;

class ResonatorSection : public SynthSection {
  public:
    ResonatorSection(String name);
    virtual ~ResonatorSection();

    void paintBackground(Graphics& g) override;
    void paintBackgroundShadow(Graphics& g) override { if (isActive()) paintTabShadow(g); }
    void resized() override;
    void sliderValueChanged(Slider* changed_slider) override;
    void setAllValues(vital::control_map& controls) override;

  private:
    std::unique_ptr<SynthButton> on_;
    std::unique_ptr<TextSelector> style_, keytrack_, voices_;
    std::unique_ptr<SynthSlider> tune_, fine_, slide_, pressure_, resonance_, mix_, damping_, stretch_, color_;
    std::unique_ptr<ResonatorPeaksViewer> peaks_;
    std::unique_ptr<ResonatorTiltViewer> tilt_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResonatorSection)
};
