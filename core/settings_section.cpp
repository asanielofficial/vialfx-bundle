// VialFX - Licensed under GPL-3.0-or-later.
// Adapted from Vital's OversampleSettings and DisplaySettings (Matt Tytel, GPL-3.0).
#include "settings_section.h"

#include "effect_info.h"
#include "effect_plugin.h"
#include "full_interface.h"
#include "load_save.h"
#include "skin.h"
#include "synth_button.h"
#include "synth_gui_interface.h"
#include "synth_slider.h"
#include "text_look_and_feel.h"
#include "text_selector.h"

namespace {
  const std::string kFrequencyDisplayNames[] = { "Semitones", "Hz" };
}

// ---- Oversampling ----------------------------------------------------------------------------------
class SettingsSection::OversampleBox : public SynthSection {
  public:
    OversampleBox() : SynthSection("OVERSAMPLING") {
      setSidewaysHeading(false);
      const char* labels[4] = { "1x (Draft)", "2x (Recommended)", "4x (High CPU)", "8x (Ultra CPU)" };
      for (int i = 0; i < 4; ++i) {
        buttons_[i] = std::make_unique<OpenGlToggleButton>("");
        buttons_[i]->addListener(this);
        buttons_[i]->setLookAndFeel(TextLookAndFeel::instance());
        buttons_[i]->setButtonText(labels[i]);
        addAndMakeVisible(buttons_[i].get());
        addOpenGlComponent(buttons_[i]->getGlComponent());
      }
    }

    void setAllValues(vital::control_map& controls) override {
      SynthSection::setAllValues(controls);
      select(static_cast<int>(controls["oversampling"]->value()));
    }

    void paintBackground(Graphics& g) override {
      SynthSection::paintBackground(g);
      g.setColour(findColour(Skin::kTextComponentBackground, true));
      float rounding = findValue(Skin::kLabelBackgroundRounding);
      for (auto& b : buttons_)
        g.fillRoundedRectangle(b->getBounds().toFloat(), rounding);
    }

    void paintBackgroundShadow(Graphics& g) override { paintTabShadow(g); }

    void resized() override {
      SynthSection::resized();
      const int margin = getWidgetMargin();
      const int width = getWidth() - 2 * margin;
      const int y = getTitleWidth() + margin;
      const int each = (getHeight() - y) / 4;
      for (int i = 0; i < 4; ++i)
        buttons_[i]->setBounds(margin, y + i * each, width, each - margin);
    }

    void buttonClicked(Button* clicked) override {
      for (int i = 0; i < 4; ++i) {
        if (clicked == buttons_[i].get()) {
          select(i);
          if (SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>()) {
            parent->getSynth()->valueChangedInternal("oversampling", static_cast<vital::mono_float>(i));
            parent->getSynth()->notifyOversamplingChanged();
          }
        }
      }
    }

  private:
    void select(int index) {
      for (int i = 0; i < 4; ++i)
        buttons_[i]->setToggleState(i == index, dontSendNotification);
    }

    std::unique_ptr<OpenGlToggleButton> buttons_[4];
};

// ---- Display (frequency units + skin), same behaviour as Vital's Advanced tab --------------------------
class SettingsSection::DisplayBox : public SynthSection {
  public:
    DisplayBox() : SynthSection("DISPLAY") {
      setSidewaysHeading(false);

      frequency_display_ = std::make_unique<TextSelector>("frequency_display");
      frequency_display_->setRange(0.0, 1.0, 1.0);
      frequency_display_->setValue(LoadSave::displayHzFrequency());
      addSlider(frequency_display_.get());
      frequency_display_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
      frequency_display_->setLookAndFeel(TextLookAndFeel::instance());
      frequency_display_->setStringLookup(kFrequencyDisplayNames);
      frequency_display_->setLongStringLookup(kFrequencyDisplayNames);

      LoadSave::getAllSkins(skins_);
      File default_skin = LoadSave::getDefaultSkin();
      skin_value_ = 0;
      if (default_skin.exists()) {
        String skin_name = LoadSave::getLoadedSkin();
        for (int i = 0; i < skins_.size(); ++i)
          if (skins_[i].getFileNameWithoutExtension() == skin_name)
            skin_value_ = i + 1;
        if (skin_value_ == 0)
          skin_value_ = skins_.size() + 1;
      }

      short_skin_strings_ = std::make_unique<std::string[]>(skins_.size() + 2);
      long_skin_strings_ = std::make_unique<std::string[]>(skins_.size() + 2);
      short_skin_strings_[0] = "Default";
      long_skin_strings_[0] = "Default";
      short_skin_strings_[skins_.size() + 1] = "Custom";
      long_skin_strings_[skins_.size() + 1] = "Load Custom Skin...";
      for (int i = 0; i < skins_.size(); ++i) {
        short_skin_strings_[i + 1] = skins_[i].getFileNameWithoutExtension().toStdString();
        long_skin_strings_[i + 1] = skins_[i].getFileNameWithoutExtension().toStdString();
      }

      skin_ = std::make_unique<TextSelector>("skin");
      skin_->setRange(0.0, skins_.size() + 1, 1);
      skin_->setValue(skin_value_, dontSendNotification);
      skin_->setScrollEnabled(false);
      addSlider(skin_.get());
      skin_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
      skin_->setLookAndFeel(TextLookAndFeel::instance());
      skin_->setStringLookup(short_skin_strings_.get());
      skin_->setLongStringLookup(long_skin_strings_.get());

      sync_button_ = std::make_unique<OpenGlToggleButton>("");
      sync_button_->addListener(this);
      sync_button_->setLookAndFeel(TextLookAndFeel::instance());
      sync_button_->setButtonText("Sync skin to all VialFX plugins");
      addAndMakeVisible(sync_button_.get());
      addOpenGlComponent(sync_button_->getGlComponent());
    }

    void paintBackgroundShadow(Graphics& g) override { paintTabShadow(g); }

    void paintBackground(Graphics& g) override {
      SynthSection::paintBackground(g);
      drawTextComponentBackground(g, frequency_display_->getBounds(), true);
      drawTextComponentBackground(g, skin_->getBounds(), true);
      setLabelFont(g);
      drawLabelForComponent(g, "FREQUENCY UNITS", frequency_display_.get(), true);
      drawLabelForComponent(g, "SKIN", skin_.get(), true);
    }

    void resized() override {
      SynthSection::resized();
      const int margin = getWidgetMargin();
      const int width = getWidth() - 2 * margin;
      const int y = getTitleWidth() + margin;
      const int bottom = getHeight() - margin;
      const int row_height = (bottom - y - 2 * margin) / 3;
      frequency_display_->setBounds(margin, y, width, row_height);
      const int skin_y = y + row_height + margin;
      skin_->setBounds(margin, skin_y, width, row_height);
      const int sync_y = skin_y + row_height + margin;
      sync_button_->setBounds(margin, sync_y, width, bottom - sync_y);
    }

    void buttonClicked(Button* clicked) override {
      if (clicked != sync_button_.get()) {
        SynthSection::buttonClicked(clicked);
        return;
      }
      // Publish this plugin's current skin as the bundle skin; every open or newly opened VialFX plugin adopts it.
      File own = LoadSave::getDefaultSkin();
      if (!own.existsAsFile())
        LoadSave::writeBuiltInDefaultSkin(own);
      File bundle = LoadSave::getBundleSkin();
      if (LoadSave::installSkinFile(own, bundle))
        if (FullInterface* full = findParentComponentOfClass<FullInterface>())
          full->markBundleSkinApplied();
      sync_button_->setButtonText("Skin synced!");
      sync_button_->setToggleState(false, dontSendNotification);
      Timer::callAfterDelay(1500, [safe = Component::SafePointer<SynthSection>(this)]() {
        if (safe != nullptr)
          static_cast<DisplayBox*>(safe.getComponent())->sync_button_->setButtonText("Sync skin to all VialFX plugins");
      });
    }

    void pushFrequencyDisplay() {
      if (SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>())
        if (parent->getGui())
          parent->getGui()->setSkinValue(Skin::kFrequencyDisplay, frequency_display_->getValue());
    }

    void sliderValueChanged(Slider* changed) override {
      if (changed == frequency_display_.get()) {
        pushFrequencyDisplay();
        LoadSave::saveDisplayHzFrequency(frequency_display_->getValue() != 0.0f);
        // Hz/semitone labels are drawn into the cached background, so redraw it.
        if (FullInterface* full = findParentComponentOfClass<FullInterface>())
          full->redoBackground();
      }
      else if (changed == skin_.get()) {
        File default_skin = LoadSave::getDefaultSkin();
        if (skin_->getValue() == 0.0f) {
          // Keep a file (rather than deleting it) so the choice is newer than any synced bundle skin.
          LoadSave::saveLoadedSkin("");
          LoadSave::writeBuiltInDefaultSkin(default_skin);
          Skin skin;
          skin.loadDefaultSkin();
          loadSkin(skin);
        }
        else if (skin_->getValue() == skins_.size() + 1) {
          FileChooser open_box("Open Skin", File(), String("*.") + vital::kSkinExtension);
          if (open_box.browseForFileToOpen()) {
            File skin_file = open_box.getResult();
            LoadSave::installSkinFile(skin_file, LoadSave::getDefaultSkin());
            Skin skin;
            skin.loadFromFile(skin_file);
            loadSkin(skin);
          }
        }
        else {
          Skin skin;
          File skin_file = skins_[static_cast<int>(skin_->getValue()) - 1];
          if (!skin_file.exists())
            return;
          LoadSave::saveLoadedSkin(skin_file.getFileNameWithoutExtension().toStdString());
          LoadSave::installSkinFile(skin_file, default_skin);
          skin.loadFromFile(skin_file);
          loadSkin(skin);
        }
        skin_value_ = static_cast<int>(skin_->getValue());
      }
    }

  private:
    void loadSkin(Skin& skin) {
      if (FullInterface* full = findParentComponentOfClass<FullInterface>())
        full->reloadSkin(skin);
    }

    std::unique_ptr<TextSelector> frequency_display_;
    std::unique_ptr<TextSelector> skin_;
    std::unique_ptr<OpenGlToggleButton> sync_button_;
    int skin_value_ = 0;
    Array<File> skins_;
    std::unique_ptr<std::string[]> short_skin_strings_;
    std::unique_ptr<std::string[]> long_skin_strings_;
};

// ---- Keytrack (filter plugin only) -----------------------------------------------------------------------
class SettingsSection::KeytrackBox : public SynthSection {
  public:
    KeytrackBox() : SynthSection("KEYTRACK") {
      setSidewaysHeading(false);
      midi_follow_ = std::make_unique<OpenGlToggleButton>("");
      midi_follow_->addListener(this);
      midi_follow_->setLookAndFeel(TextLookAndFeel::instance());
      midi_follow_->setButtonText("Follow MIDI notes");
      addAndMakeVisible(midi_follow_.get());
      addOpenGlComponent(midi_follow_->getGlComponent());
    }

    void parentHierarchyChanged() override {
      SynthSection::parentHierarchyChanged();
      if (EffectPlugin* plugin = getPlugin())
        midi_follow_->setToggleState(plugin->getSettings().keytrack_from_midi, dontSendNotification);
    }

    void paintBackgroundShadow(Graphics& g) override { paintTabShadow(g); }

    void paintBackground(Graphics& g) override {
      SynthSection::paintBackground(g);
      g.setColour(findColour(Skin::kTextComponentBackground, true));
      g.fillRoundedRectangle(midi_follow_->getBounds().toFloat(), findValue(Skin::kLabelBackgroundRounding));
    }

    void resized() override {
      SynthSection::resized();
      const int margin = getWidgetMargin();
      const int y = getTitleWidth() + margin;
      midi_follow_->setBounds(margin, y, getWidth() - 2 * margin, (getHeight() - y) / 4 - margin);
    }

    void buttonClicked(Button* clicked) override {
      if (clicked != midi_follow_.get())
        return;
      if (EffectPlugin* plugin = getPlugin())
        plugin->setKeytrackFromMidi(midi_follow_->getToggleState());
    }

  private:
    EffectPlugin* getPlugin() {
      if (SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>())
        return dynamic_cast<EffectPlugin*>(parent->getSynth());
      return nullptr;
    }

    std::unique_ptr<OpenGlToggleButton> midi_follow_;
};

// ---- The page ------------------------------------------------------------------------------------------
SettingsSection::SettingsSection() : SynthSection("settings") {
  oversample_ = std::make_unique<OversampleBox>();
  addSubSection(oversample_.get());

  display_ = std::make_unique<DisplayBox>();
  addSubSection(display_.get());

  if (vitalfx::thisEffect().is_filter) {
    keytrack_ = std::make_unique<KeytrackBox>();
    addSubSection(keytrack_.get());
  }
}

SettingsSection::~SettingsSection() = default;

void SettingsSection::paintBackground(Graphics& g) {
  g.fillAll(findColour(Skin::kBackground, true));
  paintChildrenShadows(g);
  paintKnobShadows(g);
  paintChildrenBackgrounds(g);
}

void SettingsSection::resized() {
  const int padding = getPadding();
  const int boxes = keytrack_ ? 3 : 2;
  const int box_width = (getWidth() - (boxes - 1) * padding) / boxes;

  int x = 0;
  oversample_->setBounds(x, 0, box_width, getHeight());
  x += box_width + padding;
  display_->setBounds(x, 0, box_width, getHeight());
  if (keytrack_) {
    x += box_width + padding;
    keytrack_->setBounds(x, 0, getWidth() - x, getHeight());
  }

  SynthSection::resized();
}

void SettingsSection::setAllValues(vital::control_map& controls) {
  SynthSection::setAllValues(controls);
}

void SettingsSection::buttonClicked(Button* clicked_button) {
  SynthSection::buttonClicked(clicked_button);
}

void SettingsSection::sliderValueChanged(Slider* changed_slider) {
  SynthSection::sliderValueChanged(changed_slider);
}

void SettingsSection::parentHierarchyChanged() {
  SynthSection::parentHierarchyChanged();
  if (display_)
    display_->pushFrequencyDisplay();
}
