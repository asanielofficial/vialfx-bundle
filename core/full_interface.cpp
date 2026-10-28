// VialFX - Licensed under GPL-3.0-or-later.
// Derived from Vital's FullInterface by Matt Tytel (GPL-3.0).
#include "full_interface.h"

#include "about_section.h"
#include "default_look_and_feel.h"
#include "delete_section.h"
#include "effect_info.h"
#include "effect_section_factory.h"
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

  effect_section_ = vitalfx::createEffectSection(static_cast<vitalfx::EffectKind>(VITALFX_EFFECT),
                                                 synth_data->mono_modulations);
  addSubSection(effect_section_.get());

  settings_section_ = std::make_unique<SettingsSection>();
  addSubSection(settings_section_.get());
  settings_section_->setVisible(false);

  header_ = std::make_unique<EffectHeader>();
  addSubSection(header_.get());
  header_->addListener(this);

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
  open_gl_context_.detach();
  open_gl_context_.setRenderer(nullptr);
}

Point<int> FullInterface::getDesignSize() {
  Skin skin;
  const float knob_section = skin.getValue(Skin::kKnobSectionHeight);
  const float widget_margin = skin.getValue(Skin::kWidgetMargin);
  const float large_padding = skin.getValue(Skin::kLargePadding);
  const float padding = skin.getValue(Skin::kPadding);
  const float section_height = 2.0f * knob_section - widget_margin;
  const float height = kTopHeight + padding + section_height + large_padding;
  return Point<int>(static_cast<int>(kDesignWidth), static_cast<int>(std::ceil(height)));
}

void FullInterface::paintBackground(Graphics& g) {
  g.fillAll(findColour(Skin::kBackground, true));
  paintChildrenShadows(g);
  paintKnobShadows(g);
  paintChildrenBackgrounds(g);
}

void FullInterface::copySkinValues(const Skin& skin) {
  ScopedLock open_gl_lock(open_gl_critical_section_);
  skin.copyValuesToLookAndFeel(DefaultLookAndFeel::instance());
  setSkinValues(skin, true);
}

void FullInterface::reloadSkin(const Skin& skin) {
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

  const Point<int> design = getDesignSize();

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
  const int section_height = 2 * knob_section_height - widget_margin;
  Rectangle<int> section_bounds(left + large_padding, top + top_height + padding,
                                width - 2 * large_padding, section_height);
  effect_section_->setBounds(section_bounds);
  settings_section_->setBounds(section_bounds);

  about_section_->setBounds(bounds);
  save_section_->setBounds(bounds);
  delete_section_->setBounds(bounds);
  preset_browser_->setBounds(section_bounds.getX(), header_->getBottom(),
                             section_bounds.getWidth(), height - header_->getBottom());

  SynthSection::resized();

  if (getWidth() && getHeight())
    redoBackground();
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
}

void FullInterface::openGLContextClosing() {
  if (unsupported_)
    return;

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
  display->setContent(text, getLocalArea(source, source->getLocalBounds()), placement);
  display->setVisible(true);
}

void FullInterface::hideDisplay(bool primary) {
  PopupDisplay* display = primary ? popup_display_1_.get() : popup_display_2_.get();
  if (display)
    display->setVisible(false);
}
