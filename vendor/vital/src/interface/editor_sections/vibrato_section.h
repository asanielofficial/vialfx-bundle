// VialFX - Vibrato section. Licensed under GPL-3.0-or-later.
#pragma once

#include "JuceHeader.h"
#include "synth_section.h"

class SynthButton;
class SynthSlider;
class TextSelector;
class VibratoLeds;
class VibratoMeter;
class VibratoViewer;

class VibratoSection : public SynthSection {
  public:
    VibratoSection(String name);
    virtual ~VibratoSection();

    void paintBackground(Graphics& g) override;
    void paintBackgroundShadow(Graphics& g) override { if (isActive()) paintTabShadow(g); }
    void resized() override;
    void setActive(bool active) override;
    void buttonClicked(Button* clicked_button) override;
    void sliderValueChanged(Slider* changed_slider) override;
    void setAllValues(vital::control_map& controls) override;

  private:
    void layoutKnobs();
    SynthSlider* visibleRate(bool a) const;

    std::unique_ptr<SynthButton> on_;
    std::unique_ptr<TextSelector> mod_a_, mod_b_;
    std::unique_ptr<SynthSlider> rate_a_, rate_b_, tempo_a_, tempo_b_, amount_a_, amount_b_;
    std::unique_ptr<SynthButton> sync_a_, sync_b_;
    std::unique_ptr<VibratoLeds> leds_on_, leds_off_;
    std::unique_ptr<VibratoViewer> viewer_;
    std::unique_ptr<VibratoMeter> meter_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VibratoSection)
};
