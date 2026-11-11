// VialFX - Licensed under GPL-3.0-or-later.
// Derived from Vital's FullInterface by Matt Tytel (GPL-3.0).
#include "full_interface.h"

#include "about_section.h"
#include "default_look_and_feel.h"
#include "delete_section.h"
#include "effect_info.h"
#include "effect_section_factory.h"
#if VITALFX_HAS_MORPH
#include "morph_effect_section.h"
#include "wavetable_edit_section.h"
#endif
#include "fonts.h"
#include "preset_browser.h"
#include "save_section.h"
#include "settings_section.h"
#include "load_save.h"
#include "skin.h"
#include "synth_gui_interface.h"

FullInterface::FullInterface(SynthGuiData* synth_data) : SynthSection("full_interface"), open_gl_(open_gl_context_) {
  Skin default_skin;
  setSkinValues(default_skin, true);
  default_skin.copyValuesToLookAndFeel(DefaultLookAndFeel::instance());
  markBundleSkinApplied();
  startTimer(1500);

#if VITALFX_HAS_MORPH
  {
    auto morph = std::make_unique<MorphEffectSection>(synth_data);
    morph_section_ = morph.get();
    effect_section_ = std::move(morph);
  }
#else
  effect_section_ = vitalfx::createEffectSection(static_cast<vitalfx::EffectKind>(VITALFX_EFFECT),
                                                 synth_data->mono_modulations);
#endif
  addSubSection(effect_section_.get());

#if VITALFX_HAS_MORPH
  if (morph_section_) {
    // The two table modes' wavetable editors (slot 0 = Distort Table, 1 = Spectral Table).
    for (int i = 0; i < vital::kNumWavetableSlots; ++i) {
      if (synth_data->wavetable_creators[i] == nullptr)
        continue;
      wavetable_edits_[i] = std::make_unique<WavetableEditSection>(i, synth_data->wavetable_creators[i]);
      addSubSection(wavetable_edits_[i].get());
      wavetable_edits_[i]->setVisible(false);
    }
    morph_section_->setLayoutCallback([this]() { morphLayoutChanged(); });
    morph_section_->setTableEditorCallback([this](int index) { showWavetableEditSection(index); });
    morph_section_->setTableImportCallback(
        [this](int index, String name, InputStream* stream, WavetableCreator::AudioFileLoadStyle style) {
          if (index < 0 || index >= vital::kNumWavetableSlots || wavetable_edits_[index] == nullptr) {
            delete stream;
            return false;
          }
          return wavetable_edits_[index]->loadAudioAsWavetable(name, stream, style);
        });
  }
#endif

  settings_section_ = std::make_unique<SettingsSection>();
  addSubSection(settings_section_.get());

  settings_section_->setVisible(false);

  header_ = std::make_unique<EffectHeader>();
  addSubSection(header_.get());
  header_->addListener(this);
  header_->setAudioMemory(synth_data->synth->getAudioMemory());

  preset_browser_ = std::make_unique<PresetBrowser>();
  addSubSection(preset_browser_.get());
  preset_browser_->setVisible(false);

  popup_browser_ = std::make_unique<PopupBrowser>();
  addSubSection(popup_browser_.get());
  popup_browser_->setVisible(false);

  popup_selector_ = std::make_unique<SinglePopupSelector>();
  addSubSection(popup_selector_.get());
  popup_selector_->setVisible(false);
  popup_selector_->setAlwaysOnTop(true);
  popup_selector_->setWantsKeyboardFocus(true);

  dual_popup_selector_ = std::make_unique<DualPopupSelector>();
  addSubSection(dual_popup_selector_.get());
  dual_popup_selector_->setVisible(false);
  dual_popup_selector_->setAlwaysOnTop(true);
  dual_popup_selector_->setWantsKeyboardFocus(true);

  popup_display_1_ = std::make_unique<PopupDisplay>();
  addSubSection(popup_display_1_.get());
  popup_display_1_->setVisible(false);
  popup_display_1_->setAlwaysOnTop(true);
  popup_display_1_->setWantsKeyboardFocus(false);

  popup_display_2_ = std::make_unique<PopupDisplay>();
  addSubSection(popup_display_2_.get());
  popup_display_2_->setVisible(false);
  popup_display_2_->setAlwaysOnTop(true);
  popup_display_2_->setWantsKeyboardFocus(false);

  save_section_ = std::make_unique<SaveSection>("save_section");
  addSubSection(save_section_.get(), false);
  addChildComponent(save_section_.get());
  preset_browser_->setSaveSection(save_section_.get());
  header_->setSaveSection(save_section_.get());
  header_->setBrowser(preset_browser_.get());

  delete_section_ = std::make_unique<DeleteSection>("delete_section");
  addSubSection(delete_section_.get(), false);
  addChildComponent(delete_section_.get());
  preset_browser_->setDeleteSection(delete_section_.get());

  about_section_ = std::make_unique<AboutSection>("about");
  addSubSection(about_section_.get(), false);
  addChildComponent(about_section_.get());

  effect_section_->toFront(true);
  settings_section_->toFront(true);
  preset_browser_->toFront(false);
  about_section_->toFront(true);
  save_section_->toFront(true);
  delete_section_->toFront(true);
  popup_browser_->toFront(true);
  popup_selector_->toFront(true);
  dual_popup_selector_->toFront(true);
  popup_display_1_->toFront(true);
  popup_display_2_->toFront(true);

  bin_highlight_ = std::make_unique<BinHighlight>();
  addSubSection(bin_highlight_.get());

  setAllValues(synth_data->controls);
  setOpaque(true);
  setSkinValues(default_skin, true);
  // Frequency knobs start in whichever units the user picked last (shared with Vital).
  setSkinValue(Skin::kFrequencyDisplay, LoadSave::displayHzFrequency() ? 1.0f : 0.0f);

  open_gl_context_.setContinuousRepainting(true);
  open_gl_context_.setOpenGLVersionRequired(OpenGLContext::openGL3_2);
  open_gl_context_.setSwapInterval(0);
  open_gl_context_.setRenderer(this);
  open_gl_context_.setComponentPaintingEnabled(false);
  open_gl_context_.attachTo(*this);
}

FullInterface::~FullInterface() {
  stopTimer();
  open_gl_context_.detach();
  open_gl_context_.setRenderer(nullptr);
}

FullInterface::DesignMetrics FullInterface::metricsFor(const Skin& skin) {
  DesignMetrics metrics;
  metrics.knob_section = skin.getValue(Skin::kKnobSectionHeight);
  metrics.widget_margin = skin.getValue(Skin::kWidgetMargin);
  metrics.large_padding = skin.getValue(Skin::kLargePadding);
  metrics.padding = skin.getValue(Skin::kPadding);
  return metrics;
}

Point<int> FullInterface::designSizeFor(const DesignMetrics& m, int view) {
  if (view == 2)   // Morph's wavetable editor: a window of its own
    return Point<int>(kWavetableEditorWidth, kWavetableEditorHeight);

  float section_height = 2.0f * m.knob_section - m.widget_margin;
  if (vitalfx::thisEffect().kind == vitalfx::EffectKind::kMorph) {
    // The panel is as tall as any effect; the editor that docks below it while one is open adds to the window.
    if (view == 1)
      section_height += m.padding + kMorphAttachmentKnobSections * m.knob_section;
  }
  const float height = kTopHeight + m.padding + section_height + m.large_padding;
  // The Tremolo has two extra unit selectors under its knobs, so its window is a little taller than the others.
  const int extra = (vitalfx::thisEffect().kind == vitalfx::EffectKind::kTremolo ||
                       vitalfx::thisEffect().kind == vitalfx::EffectKind::kVibrato) ? kTremoloExtraHeight : 0;
  return Point<int>(static_cast<int>(kDesignWidth), static_cast<int>(std::ceil(height)) + extra);
}

Point<int> FullInterface::getDesignSize() {
  Skin skin;
  return designSizeFor(metricsFor(skin), 0);
}

// The window's proportions follow the live skin and what is open (Morph's editors dock below the panel, or take the
// whole window): the editor resizes itself when they change.
void FullInterface::updateDesignSize() {
  const Point<int> design = designSizeFor(metrics_, view_);
  if (design == design_size_)
    return;

  design_size_ = design;
  if (on_design_size_changed) {
    Component::SafePointer<FullInterface> safe(this);
    MessageManager::callAsync([safe, design]() {
      if (FullInterface* self = safe.getComponent())
        if (self->on_design_size_changed)
          self->on_design_size_changed(design);
    });
  }
}

#if VITALFX_HAS_MORPH
void FullInterface::morphLayoutChanged() {
  if (morph_section_ == nullptr || view_ == 2)
    return;
  view_ = morph_section_->anyAttachmentOpen() ? 1 : 0;
  updateDesignSize();
  resized();
  redoBackground();
}

void FullInterface::showWavetableEditSection(int index) {
  if (morph_section_ == nullptr)
    return;

  ScopedLock lock(open_gl_critical_section_);
  const bool show_editor = index >= 0 && index < vital::kNumWavetableSlots && wavetable_edits_[index] != nullptr;
  for (int i = 0; i < vital::kNumWavetableSlots; ++i) {
    if (wavetable_edits_[i])
      wavetable_edits_[i]->setVisible(show_editor && i == index);
  }

  open_table_editor_ = show_editor ? index : -1;
  header_->setVisible(!show_editor);
  effect_section_->setVisible(!show_editor);
  view_ = show_editor ? 2 : (morph_section_->anyAttachmentOpen() ? 1 : 0);
  updateDesignSize();
  resized();
  redoBackground();
}

void FullInterface::saveWavetable(int index) {
  if (index < 0 || index >= vital::kNumWavetableSlots || wavetable_edits_[index] == nullptr)
    return;

  save_section_->setIsPreset(false);
  save_section_->setSaveBounds();
  save_section_->setFileExtension(vital::kWavetableExtension);
  save_section_->setFileType("Wavetable");
  File destination = LoadSave::getUserWavetableDirectory();
  if (!destination.exists())
    destination.createDirectory();
  save_section_->setDirectory(destination);
  save_section_->setFileData(getWavetableJson(index));
  save_section_->setVisible(true);
}

json FullInterface::getWavetableJson(int index) {
  if (index >= 0 && index < vital::kNumWavetableSlots && wavetable_edits_[index])
    return wavetable_edits_[index]->getWavetableJson();
  return json();
}
#else
void FullInterface::morphLayoutChanged() { }
void FullInterface::showWavetableEditSection(int) { }
void FullInterface::saveWavetable(int) { }
json FullInterface::getWavetableJson(int) { return json(); }
#endif

void FullInterface::paintBackground(Graphics& g) {
  g.fillAll(findColour(Skin::kBackground, true));
  paintChildrenShadows(g);
  paintKnobShadows(g);
  paintChildrenBackgrounds(g);
}

void FullInterface::setSkinValues(const Skin& skin, bool top_level) {
  SynthSection::setSkinValues(skin, top_level);
  if (!top_level)
    return;

  updateGlow(skin);

  // The skin editor can change the spacing values the window's proportions come from (padding, knob section
  // height...). The layout follows the live skin, so the window's aspect ratio has to follow it too, otherwise the
  // interface is letterboxed and no longer scales with the window.
  metrics_ = metricsFor(skin);
  updateDesignSize();
}

void FullInterface::updateGlow(const Skin& skin) {
  std::vector<Skin::GlowEntry> glows;
  skin.collectGlows(glows);
  std::vector<GlowPass::Key> keys;
  for (const auto& glow : glows) {
    GlowPass::Key key = { glow.color.getFloatRed(), glow.color.getFloatGreen(), glow.color.getFloatBlue(),
                          glow.glow, glow.size };
    bool merged = false;
    for (GlowPass::Key& existing : keys) {
      float distance = std::abs(existing.red - key.red) + std::abs(existing.green - key.green) +
                       std::abs(existing.blue - key.blue);
      if (distance < 0.03f) {
        existing.glow = std::max(existing.glow, key.glow);
        existing.size = std::max(existing.size, key.size);
        merged = true;
        break;
      }
    }
    if (!merged && static_cast<int>(keys.size()) < GlowPass::kMaxKeys)
      keys.push_back(key);
  }

  {
    ScopedLock lock(open_gl_critical_section_);
    glow_pass_.setKeys(keys);
  }

  gradient_entries_.clear();
  skin.collectGradients(gradient_entries_);
  updateGradientRegions();
}

static void collectSectionRects(Component* component, Skin::SectionOverride override_id, FullInterface* root,
                                std::vector<Rectangle<int>>& rects) {
  for (int i = 0; i < component->getNumChildComponents(); ++i) {
    Component* child = component->getChildComponent(i);
    if (!child->isVisible())
      continue;

    SynthSection* section = dynamic_cast<SynthSection*>(child);
    if (section != nullptr && section->getSkinOverride() == override_id && child->getWidth() > 0) {
      rects.push_back(root->getLocalArea(child, child->getLocalBounds()));
      continue;
    }
    collectSectionRects(child, override_id, root, rects);
  }
}

void FullInterface::updateGradientRegions() {
  std::vector<GlowPass::GradientKey> keys;
  float width = static_cast<float>(getWidth());
  float height = static_cast<float>(getHeight());

  if (width > 0.0f && height > 0.0f) {
    for (const Skin::GradientEntry& entry : gradient_entries_) {
      if (static_cast<int>(keys.size()) >= GlowPass::kMaxGradients)
        break;

      Rectangle<int> region = getLocalBounds();
      if (entry.section != Skin::kNone) {
        std::vector<Rectangle<int>> rects;
        collectSectionRects(this, static_cast<Skin::SectionOverride>(entry.section), this, rects);
        if (rects.empty())
          continue;

        region = rects[0];
        for (const Rectangle<int>& rect : rects)
          region = region.getUnion(rect);
      }

      GlowPass::GradientKey key = { };
      key.base[0] = entry.color.getFloatRed();
      key.base[1] = entry.color.getFloatGreen();
      key.base[2] = entry.color.getFloatBlue();
      key.num_stops = entry.gradient.active() ? static_cast<int>(entry.gradient.stops.size()) + 1 : 1;
      key.rect[0] = region.getX() / width;
      key.rect[1] = 1.0f - region.getBottom() / height;
      key.rect[2] = region.getRight() / width;
      key.rect[3] = 1.0f - region.getY() / height;
      float radians = degreesToRadians(entry.gradient.angle);
      key.direction[0] = std::cos(radians);
      key.direction[1] = -std::sin(radians);
      Colour stop1 = entry.gradient.active() ? entry.gradient.stops[0] : entry.color;
      Colour stop2 = entry.gradient.stops.size() > 1 ? entry.gradient.stops[1] : stop1;
      key.stop1[0] = stop1.getFloatRed(); key.stop1[1] = stop1.getFloatGreen(); key.stop1[2] = stop1.getFloatBlue();
      key.stop2[0] = stop2.getFloatRed(); key.stop2[1] = stop2.getFloatGreen(); key.stop2[2] = stop2.getFloatBlue();
      if (entry.bevel.active()) {
        // Size 0 means "no bevel" to the shader: a bevel with only a gloss still needs it on.
        key.bevel_size = entry.bevel.size;
        key.bevel_depth = entry.bevel.depth;
        key.bevel_gloss = entry.bevel.gloss;
        float light = degreesToRadians(entry.bevel.angle);
        key.light[0] = std::cos(light);
        key.light[1] = std::sin(light);
      }
      keys.push_back(key);
    }
  }

  ScopedLock lock(open_gl_critical_section_);
  glow_pass_.setGradients(keys);
}

void FullInterface::copySkinValues(const Skin& skin) {
  ScopedLock open_gl_lock(open_gl_critical_section_);
  skin.copyValuesToLookAndFeel(DefaultLookAndFeel::instance());
  setSkinValues(skin, true);
}

void FullInterface::markBundleSkinApplied() {
  bundle_skin_applied_ = LoadSave::getBundleSkin().getLastModificationTime();
}

void FullInterface::timerCallback() {
  File bundle = LoadSave::getBundleSkin();
  if (!bundle.existsAsFile())
    return;
  Time modified = bundle.getLastModificationTime();
  if (modified <= bundle_skin_applied_)
    return;
  bundle_skin_applied_ = modified;
  Skin skin;
  if (skin.loadFromFile(bundle)) {
    LoadSave::installSkinFile(bundle, LoadSave::getDefaultSkin());
    LoadSave::saveLoadedSkin("");
    reloadSkin(skin);
  }
}

void FullInterface::setBinHighlight(const std::vector<Rectangle<int>>& rects, const String& label) {
  if (bin_highlight_)
    bin_highlight_->setHighlight(rects, label);
}

void FullInterface::clearBinHighlight() {
  if (bin_highlight_)
    bin_highlight_->clearHighlight();
}

void FullInterface::reloadSkin(const Skin& skin) {
  // The shrink-and-restore forces every component to lay itself out again. The render thread is held off for
  // both steps, otherwise it can draw the tiny in-between layout and the window flashes on every skin edit.
  ScopedLock open_gl_lock(open_gl_critical_section_);
  copySkinValues(skin);
  Rectangle<int> bounds = getBounds();
  setBounds(0, 0, bounds.getWidth() / 4, bounds.getHeight() / 4);
  setBounds(bounds);
}

void FullInterface::repaintChildBackground(SynthSection* child) {
  if (!background_image_.isValid() || setting_all_values_)
    return;

  background_.lock();
  Graphics g(background_image_);
  paintChildBackground(g, child);
  background_.updateBackgroundImage(background_image_);
  background_.unlock();
}

void FullInterface::repaintOpenGlBackground(OpenGlComponent* component) {
  if (!background_image_.isValid())
    return;

  background_.lock();
  Graphics g(background_image_);
  paintOpenGlBackground(g, component);
  background_.updateBackgroundImage(background_image_);
  background_.unlock();
}

void FullInterface::redoBackground() {
  int width = std::ceil(display_scale_ * getWidth());
  int height = std::ceil(display_scale_ * getHeight());
  // Vital refuses to draw below its synth window minimum; single-effect windows are smaller.
  if (width <= 0 || height <= 0)
    return;

  ScopedLock open_gl_lock(open_gl_critical_section_);

  background_.lock();
  background_image_ = Image(Image::RGB, width, height, true);
  Graphics g(background_image_);
  paintBackground(g);
  background_.updateBackgroundImage(background_image_);
  background_.unlock();
}

void FullInterface::checkShouldReposition(bool resize) {
  float old_scale = display_scale_;
  int old_pixel_multiple = pixel_multiple_;
  display_scale_ = getDisplayScale();
  pixel_multiple_ = std::max<int>(1, display_scale_);

  if (resize && (old_scale != display_scale_ || old_pixel_multiple != pixel_multiple_))
    resized();
}

void FullInterface::resized() {
  checkShouldReposition(false);

  width_ = getWidth();
  if (!enable_redo_background_)
    return;

  resized_width_ = width_;

  ScopedLock lock(open_gl_critical_section_);

  const Point<int> design = design_size_;

  int left = 0;
  int top = 0;
  int width = std::ceil(getWidth() * display_scale_);
  int height = std::ceil(getHeight() * display_scale_);
  Rectangle<int> bounds(0, 0, width, height);

  // Same fitting rule Vital uses: keep the design aspect ratio and centre what is left over.
  float width_ratio = getWidth() / (1.0f * design.x);
  float ratio = width_ratio * display_scale_;
  float height_ratio = getHeight() / (1.0f * design.y);
  if (width_ratio > height_ratio + 1.0f / design.y) {
    ratio = height_ratio * display_scale_;
    width = height_ratio * design.x * display_scale_;
    left = (getWidth() - width) / 2;
  }
  if (height_ratio > width_ratio + 1.0f / design.y) {
    ratio = width_ratio * display_scale_;
    height = width_ratio * design.y * display_scale_;
    top = (getHeight() - height) / 2;
  }

  setSizeRatio(ratio);

  popup_browser_->setBounds(bounds);

  const int padding = getPadding();
  const int large_padding = findValue(Skin::kLargePadding);
  const int top_height = kTopHeight * ratio;
  header_->setBounds(left, top, width, top_height);

  const int knob_section_height = getKnobSectionHeight();
  const int widget_margin = findValue(Skin::kWidgetMargin);
  const int extra_height = (vitalfx::thisEffect().kind == vitalfx::EffectKind::kTremolo ||
                       vitalfx::thisEffect().kind == vitalfx::EffectKind::kVibrato) ? kTremoloExtraHeight * ratio : 0;
  int section_height = 2 * knob_section_height - widget_margin + extra_height;
  if (vitalfx::thisEffect().kind == vitalfx::EffectKind::kMorph) {
    if (view_ == 1)
      section_height += padding + static_cast<int>(kMorphAttachmentKnobSections * knob_section_height);
  }
  Rectangle<int> section_bounds(left + large_padding, top + top_height + padding,
                                width - 2 * large_padding, section_height);
  effect_section_->setBounds(section_bounds);
  settings_section_->setBounds(section_bounds.withHeight(2 * knob_section_height - widget_margin + extra_height));
#if VITALFX_HAS_MORPH
  for (auto& editor : wavetable_edits_) {
    if (editor)
      editor->setBounds(left, top, width, height - 2 * top);
  }
#endif

  about_section_->setBounds(bounds);
  save_section_->setBounds(bounds);
  delete_section_->setBounds(bounds);
  if (bin_highlight_)
    bin_highlight_->setBounds(getLocalBounds());
  preset_browser_->setBounds(section_bounds.getX(), header_->getBottom(),
                             section_bounds.getWidth(), height - header_->getBottom());

  SynthSection::resized();

  if (getWidth() && getHeight())
    redoBackground();
  if (!gradient_entries_.empty())
    updateGradientRegions();
}

void FullInterface::animate(bool animate) {
  if (animate_ != animate)
    open_gl_context_.setContinuousRepainting(animate);

  animate_ = animate;
  SynthSection::animate(animate);
}

void FullInterface::reset() {
  ScopedLock lock(open_gl_critical_section_);

  setting_all_values_ = true;
  SynthSection::reset();
  setting_all_values_ = false;
  redoBackground();
}

void FullInterface::setAllValues(vital::control_map& controls) {
  ScopedLock lock(open_gl_critical_section_);
  setting_all_values_ = true;
  SynthSection::setAllValues(controls);
  setting_all_values_ = false;
}

void FullInterface::newOpenGLContextCreated() {
  double version_supported = OpenGLShaderProgram::getLanguageVersion();
  unsupported_ = version_supported < kMinOpenGlVersion;
  if (unsupported_) {
    NativeMessageBox::showMessageBoxAsync(AlertWindow::WarningIcon, "Unsupported OpenGL Version",
                                          String("VialFX requires OpenGL version: ") + String(kMinOpenGlVersion) +
                                          String("\nSupported version: ") + String(version_supported));
    return;
  }

  shaders_ = std::make_unique<Shaders>(open_gl_context_);
  open_gl_.shaders = shaders_.get();
  open_gl_.display_scale = display_scale_;
  last_render_scale_ = display_scale_;

  background_.init(open_gl_);
  initOpenGlComponents(open_gl_);
}

void FullInterface::renderOpenGL() {
  if (unsupported_)
    return;

  float render_scale = open_gl_.context.getRenderingScale();
  if (render_scale != last_render_scale_) {
    last_render_scale_ = render_scale;
    MessageManager::callAsync([=] { checkShouldReposition(true); });
  }

  ScopedLock lock(open_gl_critical_section_);
  open_gl_.display_scale = display_scale_;
  background_.render(open_gl_);
  renderOpenGlComponents(open_gl_, animate_);
  glow_pass_.render(open_gl_.context, roundToInt(render_scale * getWidth()), roundToInt(render_scale * getHeight()),
                    render_scale);
}

void FullInterface::openGLContextClosing() {
  if (unsupported_)
    return;

  glow_pass_.destroy();
  background_.destroy(open_gl_);
  destroyOpenGlComponents(open_gl_);
  open_gl_.shaders = nullptr;
  shaders_ = nullptr;
}

void FullInterface::showAboutSection() {
  ScopedLock lock(open_gl_critical_section_);
  about_section_->setVisible(true);
}

void FullInterface::deleteRequested(File preset) {
  delete_section_->setFileToDelete(preset);
  delete_section_->setVisible(true);
}

void FullInterface::setPresetBrowserVisibility(bool visible) {
  ScopedLock lock(open_gl_critical_section_);
  settings_section_->setVisible(false);
  preset_browser_->setVisible(visible);
  effect_section_->setVisible(!visible);

  if (visible) {
    preset_browser_->repaintBackground();
    preset_browser_->grabKeyboardFocus();
  }
  redoBackground();
}

void FullInterface::bankImported() {
  preset_browser_->loadPresets();
}

void FullInterface::showSettings(Component*) {
  ScopedLock lock(open_gl_critical_section_);
  const bool show = !settings_section_->isVisible();
  preset_browser_->setVisible(false);
  settings_section_->setVisible(show);
  effect_section_->setVisible(!show);
  if (show)
    settings_section_->repaintBackground();
  redoBackground();
}

void FullInterface::notifyChange() {
  if (header_)
    header_->notifyChange();
}

void FullInterface::notifyFresh() {
  if (header_)
    header_->notifyFresh();
}

void FullInterface::externalPresetLoaded(const File& preset) {
  if (preset_browser_)
    preset_browser_->externalPresetLoaded(preset);
}

void FullInterface::openSaveDialog() {
  save_section_->setIsPreset(true);
  save_section_->setVisible(true);
}

void FullInterface::popupBrowser(SynthSection* owner, Rectangle<int> bounds, std::vector<File> directories,
                                 String extensions, std::string passthrough_name,
                                 std::string additional_folders_name) {
  popup_browser_->setIgnoreBounds(getLocalArea(owner, owner->getLocalBounds()));
  popup_browser_->setBrowserBounds(getLocalArea(owner, bounds));
  popup_browser_->setVisible(true);
  popup_browser_->grabKeyboardFocus();
  popup_browser_->setOwner(owner);
  popup_browser_->loadPresets(directories, extensions, passthrough_name, additional_folders_name);
}

void FullInterface::popupBrowserUpdate(SynthSection* owner) {
  if (popup_browser_)
    popup_browser_->setOwner(owner);
}

void FullInterface::popupSelector(Component* source, Point<int> position, const PopupItems& options,
                                  std::function<void(int)> callback, std::function<void()> cancel) {
  popup_selector_->setCallback(callback);
  popup_selector_->setCancelCallback(cancel);
  popup_selector_->showSelections(options);
  Rectangle<int> bounds(0, 0, std::ceil(display_scale_ * getWidth()), std::ceil(display_scale_ * getHeight()));
  popup_selector_->setPosition(getLocalPoint(source, position), bounds);
  popup_selector_->setVisible(true);
}

void FullInterface::dualPopupSelector(Component* source, Point<int> position, int width,
                                      const PopupItems& options, std::function<void(int)> callback) {
  dual_popup_selector_->setCallback(callback);
  dual_popup_selector_->showSelections(options);
  Rectangle<int> bounds(0, 0, std::ceil(display_scale_ * getWidth()), std::ceil(display_scale_ * getHeight()));
  dual_popup_selector_->setPosition(getLocalPoint(source, position), width, bounds);
  dual_popup_selector_->setVisible(true);
}

void FullInterface::popupDisplay(Component* source, const std::string& text,
                                 BubbleComponent::BubblePlacement placement, bool primary) {
  PopupDisplay* display = primary ? popup_display_1_.get() : popup_display_2_.get();
  Rectangle<int> source_bounds = getLocalArea(source, source->getLocalBounds());
  // Controls in the lower half would have their value bubble clipped by the window edge: show it above instead.
  if (placement == BubbleComponent::below && source_bounds.getCentreY() > getHeight() / 2)
    placement = BubbleComponent::above;
  display->setContent(text, source_bounds, placement);
  display->setVisible(true);
}

void FullInterface::hideDisplay(bool primary) {
  PopupDisplay* display = primary ? popup_display_1_.get() : popup_display_2_.get();
  if (display)
    display->setVisible(false);
}
