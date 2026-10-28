// VialFX - Licensed under GPL-3.0-or-later. Based on Vital's HeaderSection by Matt Tytel (GPL-3.0).
#pragma once

#include "JuceHeader.h"

#include "header_section.h"
#include "preset_browser.h"
#include "save_section.h"
#include "synth_preset_selector.h"
#include "synth_section.h"
#include "volume_section.h"

class OpenGlShapeButton;

// Top bar of every VialFX plugin: logo, preset selector (browse / save / previous / next),
// Vital's output volume slider with level meter, and a settings button on the right.
class EffectHeader : public SynthSection, public SaveSection::Listener,
                     public SynthPresetSelector::Listener, public LogoSection::Listener {
  public:
    class Listener {
      public:
        virtual ~Listener() = default;
        virtual void showAboutSection() = 0;
        virtual void deleteRequested(File preset) = 0;
        virtual void setPresetBrowserVisibility(bool visible) = 0;
        virtual void bankImported() = 0;
        virtual void showSettings(Component* anchor) = 0;
    };

    EffectHeader();
    ~EffectHeader() override;

    void paintBackground(Graphics& g) override;
    void resized() override;
    void reset() override;
    void buttonClicked(Button* clicked_button) override;

    // SynthPresetSelector::Listener
    void setPresetBrowserVisibility(bool visible) override;
    void setBankExporterVisibility(bool) override { }
    void deleteRequested(File preset) override;
    void bankImported() override;
    // SaveSection::Listener
    void save(File preset) override;
    // LogoSection::Listener
    void showAboutSection() override;

    void notifyChange();
    void notifyFresh();

    void setSaveSection(SaveSection* save_section) {
      preset_selector_->setSaveSection(save_section);
      save_section->addSaveListener(this);
    }
    void setBrowser(PresetBrowser* browser) { preset_selector_->setBrowser(browser); }
    void addListener(Listener* listener) { listeners_.push_back(listener); }

    Component* getSettingsAnchor();

  private:
    std::vector<Listener*> listeners_;
    std::unique_ptr<LogoSection> logo_section_;
    std::unique_ptr<SynthPresetSelector> preset_selector_;
    std::unique_ptr<VolumeSection> volume_section_;
    std::unique_ptr<OpenGlShapeButton> settings_button_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectHeader)
};
