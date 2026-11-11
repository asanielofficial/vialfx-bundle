// VialFX - Licensed under GPL-3.0-or-later.
// The Morph window: the spectral morph / distortion warp panel with the editors that dock below it. The curve editor
// (Spectral Filter, Phaser, Flanger) and the two table-settings menus open under the panel; the wavetable editors
// open over the whole window (see FullInterface). Adapted from VialFX's FxEffectSlot (Matt Tytel's Vital, GPL-3.0).
#pragma once

#include "JuceHeader.h"

#include "fx_morph_section.h"
#include "spectral_warp_editor.h"
#include "table_settings_editor.h"
#include "synth_gui_interface.h"

class MorphEffectSection : public SynthSection {
  public:
    // The docked editors are this many knob sections tall (the panel itself is FxMorphSection::heightInKnobSections()).
    static float attachmentHeightInKnobSections() { return 3.3f; }

    MorphEffectSection(SynthGuiData* data) : SynthSection("morph_effect") {
      morph_ = std::make_unique<FxMorphSection>("MORPH", "morph", 0);
      addSubSection(morph_.get());

      vital::SoundEngine* engine = data->synth->getEngine();
      curve_editor_ = std::make_unique<SpectralWarpEditor>(engine, 0, "morph");
      spectral_table_menu_ = std::make_unique<TableSettingsEditor>(
          TableSettingsEditor::kSpectral, "morph", vital::FxTables::morphSlot(0, true),
          data->mono_modulations, data->poly_modulations, "MORPH");
      distort_table_menu_ = std::make_unique<TableSettingsEditor>(
          TableSettingsEditor::kDistort, "morph", vital::FxTables::morphSlot(0, false),
          data->mono_modulations, data->poly_modulations, "MORPH");
      spectral_table_menu_->setFrameSlider(morph_->getSpectralSlider());
      distort_table_menu_->setFrameSlider(morph_->getDistortionSlider());

      for (SynthSection* attachment : attachments()) {
        addSubSection(attachment);
        attachment->setVisible(false);
        // (the editors tell us when they open or close; the window follows)
      }
      curve_editor_->setVisibilityCallback([this](bool) { attachmentVisibilityChanged(); });
      spectral_table_menu_->setVisibilityCallback([this](bool) { attachmentVisibilityChanged(); });
      distort_table_menu_->setVisibilityCallback([this](bool) { attachmentVisibilityChanged(); });
      spectral_table_menu_->setImportCallback(
          [this](String name, InputStream* stream, WavetableCreator::AudioFileLoadStyle style) {
            return import_callback_ ? import_callback_(vital::FxTables::morphSlot(0, true), name, stream, style)
                                    : false;
          });
      distort_table_menu_->setImportCallback(
          [this](String name, InputStream* stream, WavetableCreator::AudioFileLoadStyle style) {
            return import_callback_ ? import_callback_(vital::FxTables::morphSlot(0, false), name, stream, style)
                                    : false;
          });

      morph_->setAttachmentCallback([this](int kind) { toggleAttachment(kind); });
      morph_->setModesChangedCallback([this]() { refreshAttachments(); });
    }

    // Called whenever an editor docks or undocks (the window changes height).
    void setLayoutCallback(std::function<void()> callback) { layout_callback_ = callback; }
    // The pencils of the table modes open the wavetable editors: called with the slot.
    void setTableEditorCallback(std::function<void(int)> callback) { morph_->setTableEditorCallback(callback); }
    void setTableImportCallback(
        std::function<bool(int, String, InputStream*, WavetableCreator::AudioFileLoadStyle)> callback) {
      import_callback_ = callback;
    }

    bool anyAttachmentOpen() {
      for (SynthSection* attachment : attachments()) {
        if (attachment != nullptr && attachment->isVisible())
          return true;
      }
      return false;
    }

    // Height of the panel alone, and of the panel plus the docked editor, at the current scale.
    int moduleHeight() {
      return 2 * getKnobSectionHeight() - static_cast<int>(findValue(Skin::kWidgetMargin));
    }
    int attachmentsHeight() {
      return getPadding() + static_cast<int>(attachmentHeightInKnobSections() * getKnobSectionHeight());
    }

    void resized() override {
      SynthSection::resized();
      const int module_height = std::min(moduleHeight(), getHeight());
      morph_->setBounds(0, 0, getWidth(), module_height);
      layoutAttachments(module_height);
    }

    void closeAttachments() {
      suppress_layout_ = true;
      for (SynthSection* attachment : attachments()) {
        if (attachment->isVisible())
          attachment->setVisible(false);
      }
      suppress_layout_ = false;
    }

  private:
    std::vector<SynthSection*> attachments() const {
      return { curve_editor_.get(), spectral_table_menu_.get(), distort_table_menu_.get() };
    }

    // The spectral side (curve editor or Spectral Table menu) takes the left, the Distort Table menu the right; one
    // alone takes the full width.
    void layoutAttachments(int module_height) {
      SynthSection* left = nullptr;
      if (curve_editor_->isVisible())
        left = curve_editor_.get();
      else if (spectral_table_menu_->isVisible())
        left = spectral_table_menu_.get();
      SynthSection* right = distort_table_menu_->isVisible() ? distort_table_menu_.get() : nullptr;

      const int padding = getPadding();
      const int y = module_height + padding;
      const int height = std::max(1, getHeight() - y);
      if (left && right) {
        const int half = (getWidth() - padding) / 2;
        left->setBounds(0, y, half, height);
        right->setBounds(half + padding, y, getWidth() - half - padding, height);
      }
      else if (left)
        left->setBounds(0, y, getWidth(), height);
      else if (right)
        right->setBounds(0, y, getWidth(), height);
    }

    void attachmentVisibilityChanged() {
      if (suppress_layout_)
        return;
      if (layout_callback_)
        layout_callback_();
    }

    void toggleAttachment(int kind) {
      SynthSection* target = distort_table_menu_.get();
      if (kind == FxMorphSection::kCurveEditor)
        target = curve_editor_.get();
      else if (kind == FxMorphSection::kSpectralTableMenu)
        target = spectral_table_menu_.get();

      if (target->isVisible()) {
        target->setVisible(false);
        return;
      }

      suppress_layout_ = true;
      if (kind == FxMorphSection::kCurveEditor) {
        // The curve editor and the Spectral Table menu are both for the spectral side: one at a time.
        if (spectral_table_menu_->isVisible())
          spectral_table_menu_->setVisible(false);
        curve_editor_->showCurve(0, morph_->curveIndex());
      }
      else {
        if (kind == FxMorphSection::kSpectralTableMenu && curve_editor_->isVisible())
          curve_editor_->setVisible(false);
        target->setVisible(true);
      }
      suppress_layout_ = false;
      attachmentVisibilityChanged();
    }

    // The modes changed: an editor whose mode is gone closes; the curve editor follows a switch between the three
    // drawable modes.
    void refreshAttachments() {
      bool changed = false;
      suppress_layout_ = true;
      if (curve_editor_->isVisible()) {
        if (morph_->isDrawableMode())
          curve_editor_->showCurve(0, morph_->curveIndex());
        else {
          curve_editor_->setVisible(false);
          changed = true;
        }
      }
      if (spectral_table_menu_->isVisible() && !morph_->isSpectralTableMode()) {
        spectral_table_menu_->setVisible(false);
        changed = true;
      }
      if (distort_table_menu_->isVisible() && !morph_->isWarpTableMode()) {
        distort_table_menu_->setVisible(false);
        changed = true;
      }
      suppress_layout_ = false;
      if (changed)
        attachmentVisibilityChanged();
    }

    std::unique_ptr<FxMorphSection> morph_;
    std::unique_ptr<SpectralWarpEditor> curve_editor_;
    std::unique_ptr<TableSettingsEditor> spectral_table_menu_;
    std::unique_ptr<TableSettingsEditor> distort_table_menu_;
    std::function<void()> layout_callback_;
    std::function<bool(int, String, InputStream*, WavetableCreator::AudioFileLoadStyle)> import_callback_;
    bool suppress_layout_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MorphEffectSection)
};
