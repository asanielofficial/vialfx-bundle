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
#include "synth_strings.h"
#include "tuning.h"
#include "open_gl_image_component.h"

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
      sync_button_->setButtonText("Sync all");
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
      const int frequency_height = (bottom - y - margin) / 2;
      frequency_display_->setBounds(margin, y, width, frequency_height);
      // Skin selector and the "Sync all" button share the second row.
      const int skin_y = y + frequency_height + margin;
      const int sync_width = width * 2 / 5;
      skin_->setBounds(margin, skin_y, width - sync_width - margin, bottom - skin_y);
      sync_button_->setBounds(skin_->getRight() + margin, skin_y, sync_width, bottom - skin_y);
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
      sync_button_->setButtonText("Synced!");
      sync_button_->setToggleState(false, dontSendNotification);
      Timer::callAfterDelay(1500, [safe = Component::SafePointer<SynthSection>(this)]() {
        if (safe != nullptr)
          static_cast<DisplayBox*>(safe.getComponent())->sync_button_->setButtonText("Sync all");
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

// ---- FFT size (Morph only): the size behind the spectral morph, with the latency it adds. The latency is reported
// to the host, so the DAW's delay compensation keeps the plugin in time; the readout follows the setting live.
class SettingsSection::FftBox : public SynthSection, private Timer {
  public:
    FftBox() : SynthSection("FFT SIZE") {
      setSidewaysHeading(false);

      for (int i = 0; i < kNumSizes; ++i) {
        buttons_[i] = std::make_unique<OpenGlToggleButton>("");
        buttons_[i]->addListener(this);
        buttons_[i]->setLookAndFeel(TextLookAndFeel::instance());
        buttons_[i]->setButtonText(String(256 << i));
        addAndMakeVisible(buttons_[i].get());
        addOpenGlComponent(buttons_[i]->getGlComponent());
      }

      latency_text_ = std::make_unique<PlainTextComponent>("Latency Text", "");
      addOpenGlComponent(latency_text_.get());
    }

    ~FftBox() { stopTimer(); }

    void setAllValues(vital::control_map& controls) override {
      SynthSection::setAllValues(controls);
      auto size = controls.find("fft_size");
      if (size != controls.end() && size->second)
        select(static_cast<int>(size->second->value()));
    }

    void paintBackground(Graphics& g) override {
      SynthSection::paintBackground(g);

      g.setColour(findColour(Skin::kTextComponentBackground, true));
      float rounding = findValue(Skin::kLabelBackgroundRounding);
      for (auto& button : buttons_)
        g.fillRoundedRectangle(button->getBounds().toFloat(), rounding);
    }

    void paintBackgroundShadow(Graphics& g) override { paintTabShadow(g); }

    void resized() override {
      SynthSection::resized();

      int widget_margin = getWidgetMargin();
      int title_width = getTitleWidth();
      int x = widget_margin;
      int y = title_width + widget_margin;
      int width = getWidth() - 2 * widget_margin;
      int text_height = std::max(1, static_cast<int>(findValue(Skin::kLabelBackgroundHeight)));
      // Two rows of four: 256 .. 2048 on top, the larger sizes below.
      int grid_height = std::max(1, getHeight() - y - text_height - 2 * widget_margin);
      int button_height = std::max(1, (grid_height - (kNumRows - 1) * widget_margin) / kNumRows);
      int button_width = (width - (kNumColumns - 1) * widget_margin) / kNumColumns;
      for (int i = 0; i < kNumSizes; ++i) {
        int column = i % kNumColumns, row = i / kNumColumns;
        buttons_[i]->setBounds(x + column * (button_width + widget_margin),
                               y + row * (button_height + widget_margin), button_width, button_height);
      }

      latency_text_->setBounds(x, y + grid_height + widget_margin, width, text_height);
      latency_text_->setTextSize(findValue(Skin::kLabelHeight));
      latency_text_->setColor(findColour(Skin::kBodyText, true));
    }

    void visibilityChanged() override {
      SynthSection::visibilityChanged();
      if (isShowing())
        startTimerHz(8);
      else
        stopTimer();
    }

    void buttonClicked(Button* clicked_button) override {
      for (int i = 0; i < kNumSizes; ++i) {
        if (clicked_button == buttons_[i].get()) {
          SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
          if (parent)
            parent->getSynth()->valueChangedInternal("fft_size", i);
          select(i);
          return;
        }
      }
    }

  private:
    static constexpr int kNumColumns = 4;
    static constexpr int kNumRows = 2;
    static constexpr int kNumSizes = kNumColumns * kNumRows;

    void timerCallback() override {
      SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
      if (parent == nullptr || parent->getSynth() == nullptr)
        return;

      auto& controls = parent->getSynth()->getControls();
      auto size = controls.find("fft_size");
      if (size != controls.end() && size->second)
        select(static_cast<int>(size->second->value() + 0.5f));
    }

    void select(int index) {
      index = std::max(0, std::min(index, kNumSizes - 1));
      for (int i = 0; i < kNumSizes; ++i)
        buttons_[i]->setToggleState(i == index, NotificationType::dontSendNotification);

      int samples = 256 << index;
      int sample_rate = 44100;
      SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
      if (parent && parent->getSynth() && parent->getSynth()->getSampleRate() > 0)
        sample_rate = parent->getSynth()->getSampleRate();

      String text = "Latency: " + String(samples) + " samples (" +
                    String(1000.0 * samples / sample_rate, 1) + " ms)";
      if (text != shown_text_) {
        shown_text_ = text;
        latency_text_->setText(text.toStdString());
      }
    }

    std::unique_ptr<OpenGlToggleButton> buttons_[kNumSizes];
    std::unique_ptr<PlainTextComponent> latency_text_;
    String shown_text_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FftBox)
};

// ---- Resonator: MIDI / MPE, tuning and expression. All controls are ordinary Vital parameters (saved with the
// preset and automatable by the host), except the scale file and the "follow MIDI" switch, which are plugin settings.
class SettingsSection::ResonatorBox : public SynthSection {
  public:
    enum Kind { kMidi, kTuning, kExpression };

    explicit ResonatorBox(Kind kind) :
        SynthSection(kind == kMidi ? "MIDI" : (kind == kTuning ? "TUNING" : "EXPRESSION")), kind_(kind) {
      setSidewaysHeading(false);
      if (kind == kMidi) {
        midi_follow_ = makeButton("Follow MIDI notes");
        mpe_ = makeSelector("resonator_mpe", strings::kResonatorMpeNames);
        bend_ = makeBar("resonator_bend_range");
        rows_.push_back({ midi_follow_.get(), "" });
        rows_.push_back({ mpe_.get(), "MPE" });
        rows_.push_back({ bend_.get(), "Bend" });
      }
      else if (kind == kTuning) {
        tuning_ = makeSelector("resonator_tuning", strings::kResonatorTuningNames);
        load_ = makeButton("Load scale...");
        ref_ = makeBar("resonator_ref_pitch");
        glide_ = makeBar("resonator_glide");
        rows_.push_back({ tuning_.get(), "Tuning" });
        rows_.push_back({ load_.get(), "" });
        rows_.push_back({ ref_.get(), "A4 ref" });
        rows_.push_back({ glide_.get(), "Glide" });
      }
      else {
        slide_to_ = makeSelector("resonator_slide_target", strings::kResonatorSlideTargetNames);
        pressure_to_ = makeSelector("resonator_pressure_target", strings::kResonatorPressureTargetNames);
        rows_.push_back({ slide_to_.get(), "Slide" });
        rows_.push_back({ pressure_to_.get(), "Pressure" });
      }
    }

    void parentHierarchyChanged() override {
      SynthSection::parentHierarchyChanged();
      if (EffectPlugin* plugin = getPlugin()) {
        if (midi_follow_)
          midi_follow_->setToggleState(plugin->getSettings().keytrack_from_midi, dontSendNotification);
        if (load_ && !plugin->getSettings().tuning_file.empty())
          load_->setButtonText(File(String(plugin->getSettings().tuning_file)).getFileNameWithoutExtension());
      }
    }

    void paintBackgroundShadow(Graphics& g) override { paintTabShadow(g); }

    void paintBackground(Graphics& g) override {
      SynthSection::paintBackground(g);
      setLabelFont(g);
      const float rounding = findValue(Skin::kLabelBackgroundRounding);
      for (const Row& row : rows_) {
        // The component's own look draws selectors, buttons and (through OpenGL) the bars' fill; the box only adds the
        // dark well behind it and the row's name to its left.
        g.setColour(findColour(Skin::kTextComponentBackground, true));
        g.fillRoundedRectangle(row.well.toFloat(), rounding);
        if (row.label.isNotEmpty()) {
          g.setColour(findColour(Skin::kBodyText, true));
          g.drawText(row.label, row.label_bounds, Justification::centredLeft);
        }
      }
    }

    void resized() override {
      SynthSection::resized();
      const int margin = getWidgetMargin();
      const int y = getTitleWidth() + margin;
      const int count = std::max(1, static_cast<int>(rows_.size()));
      const int each = (getHeight() - y) / (kind_ == kExpression ? 3 : count);
      const int label_width = static_cast<int>((getWidth() - 2 * margin) * 0.44f);
      for (size_t i = 0; i < rows_.size(); ++i) {
        Row& row = rows_[i];
        const int top = y + static_cast<int>(i) * each, height = each - margin, width = getWidth() - 2 * margin;
        Rectangle<int> well;
        if (row.label.isNotEmpty()) {
          row.label_bounds = Rectangle<int>(margin + 2, top, label_width - 2, height);
          well = Rectangle<int>(margin + label_width, top, width - label_width, height);
        }
        else {
          row.label_bounds = Rectangle<int>();
          well = Rectangle<int>(margin, top, width, height);
        }
        row.well = well;
        // Text selectors draw their value a little above centre (they expect a name pill below it); shift the
        // component down by that offset so the value sits in the middle of the well.
        int shift = 0;
        if (auto* selector = dynamic_cast<TextSelector*>(row.component))
          shift = static_cast<int>(-selector->findValue(Skin::kTextComponentOffset));
        row.component->setBounds(well.translated(0, shift));
      }
    }

    void sliderValueChanged(Slider* changed) override {
      SynthSection::sliderValueChanged(changed);
      repaintBackground();
    }

    void buttonClicked(Button* clicked) override {
      EffectPlugin* plugin = getPlugin();
      if (clicked == midi_follow_.get()) {
        if (plugin)
          plugin->setKeytrackFromMidi(midi_follow_->getToggleState());
      }
      else if (clicked == load_.get()) {
        chooser_ = std::make_unique<FileChooser>("Load a scale or keyboard mapping", File(), Tuning::allFileExtensions());
        chooser_->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                              [this](const FileChooser& chooser) {
          File file = chooser.getResult();
          EffectPlugin* p = getPlugin();
          if (file.existsAsFile() && p) {
            p->setTuningFile(file);
            load_->setButtonText(file.getFileNameWithoutExtension());
            if (tuning_)
              tuning_->setValue(1.0);
          }
        });
      }
    }

  private:
    struct Row {
      Component* component;
      String label;
      Rectangle<int> label_bounds;
      Rectangle<int> well;
    };

    EffectPlugin* getPlugin() {
      if (SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>())
        return dynamic_cast<EffectPlugin*>(parent->getSynth());
      return nullptr;
    }

    std::unique_ptr<OpenGlToggleButton> makeButton(const char* text) {
      auto button = std::make_unique<OpenGlToggleButton>("");
      button->addListener(this);
      button->setLookAndFeel(TextLookAndFeel::instance());
      button->setButtonText(text);
      addAndMakeVisible(button.get());
      addOpenGlComponent(button->getGlComponent());
      return button;
    }

    std::unique_ptr<TextSelector> makeSelector(const char* param, const std::string* names) {
      auto selector = std::make_unique<TextSelector>(param);
      addSlider(selector.get());
      selector->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
      selector->setLookAndFeel(TextLookAndFeel::instance());
      selector->setLongStringLookup(names);
      selector->setTextHeightPercentage(0.45f);
      return selector;
    }

    std::unique_ptr<SynthSlider> makeBar(const char* param) {
      auto slider = std::make_unique<SynthSlider>(param);
      addSlider(slider.get());
      slider->setSliderStyle(Slider::LinearBar);
      slider->setPopupPlacement(BubbleComponent::above);
      return slider;
    }

    Kind kind_;
    std::vector<Row> rows_;
    std::unique_ptr<OpenGlToggleButton> midi_follow_, load_;
    std::unique_ptr<TextSelector> mpe_, tuning_, slide_to_, pressure_to_;
    std::unique_ptr<SynthSlider> bend_, ref_, glide_;
    std::unique_ptr<FileChooser> chooser_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResonatorBox)
};

// ---- The page ------------------------------------------------------------------------------------------
SettingsSection::SettingsSection() : SynthSection("settings") {
  setSkinOverride(Skin::kAdvanced);
  oversample_ = std::make_unique<OversampleBox>();
  addSubSection(oversample_.get());

  display_ = std::make_unique<DisplayBox>();
  addSubSection(display_.get());

  const bool resonator = vitalfx::thisEffect().kind == vitalfx::EffectKind::kResonator;
  if (vitalfx::thisEffect().uses_keytrack && !resonator) {
    keytrack_ = std::make_unique<KeytrackBox>();
    addSubSection(keytrack_.get());
  }

  if (resonator) {
    midi_box_ = std::make_unique<ResonatorBox>(ResonatorBox::kMidi);
    addSubSection(midi_box_.get());
    tuning_box_ = std::make_unique<ResonatorBox>(ResonatorBox::kTuning);
    addSubSection(tuning_box_.get());
    expression_box_ = std::make_unique<ResonatorBox>(ResonatorBox::kExpression);
    addSubSection(expression_box_.get());
  }

  if (vitalfx::thisEffect().kind == vitalfx::EffectKind::kMorph) {
    fft_ = std::make_unique<FftBox>();
    addSubSection(fft_.get());
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
  std::vector<SynthSection*> boxes_list = { oversample_.get(), display_.get() };
  for (SynthSection* extra : { static_cast<SynthSection*>(keytrack_.get()), static_cast<SynthSection*>(fft_.get()),
                               static_cast<SynthSection*>(midi_box_.get()), static_cast<SynthSection*>(tuning_box_.get()),
                               static_cast<SynthSection*>(expression_box_.get()) })
    if (extra)
      boxes_list.push_back(extra);
  const int boxes = static_cast<int>(boxes_list.size());
  const int box_width = (getWidth() - (boxes - 1) * padding) / boxes;

  int x = 0;
  for (int i = 0; i < boxes; ++i) {
    const bool last = i == boxes - 1;
    boxes_list[i]->setBounds(x, 0, last ? getWidth() - x : box_width, getHeight());
    x += box_width + padding;
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
