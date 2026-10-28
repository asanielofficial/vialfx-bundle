// VialFX - Licensed under GPL-3.0-or-later. Based on Vital's HeaderSection by Matt Tytel (GPL-3.0).
#include "effect_header.h"

#include "skin.h"
#include "synth_button.h"

namespace {
  // Simple gear icon, drawn in the 0..1 unit square like Vital's other button shapes.
  Path makeGearPath() {
    Path gear;
    gear.addStar(Point<float>(0.5f, 0.5f), 8, 0.30f, 0.42f, 0.0f);
    gear.addEllipse(0.5f - 0.33f, 0.5f - 0.33f, 0.66f, 0.66f);
    gear.setUsingNonZeroWinding(true);

    Path hole;
    hole.addEllipse(0.5f - 0.13f, 0.5f - 0.13f, 0.26f, 0.26f);
    gear.addPath(hole);
    gear.setUsingNonZeroWinding(false);  // even-odd so the hole stays empty
    return gear;
  }
}

EffectHeader::EffectHeader() : SynthSection("header_section") {
  logo_section_ = std::make_unique<LogoSection>();
  addSubSection(logo_section_.get());
  logo_section_->addListener(this);

  preset_selector_ = std::make_unique<SynthPresetSelector>();
  addSubSection(preset_selector_.get());
  preset_selector_->addListener(this);

  volume_section_ = std::make_unique<VolumeSection>("VOLUME");
  addSubSection(volume_section_.get());

  settings_button_ = std::make_unique<OpenGlShapeButton>("settings");
  addAndMakeVisible(settings_button_.get());
  addOpenGlComponent(settings_button_->getGlComponent());
  settings_button_->addListener(this);
  settings_button_->setShape(makeGearPath());

  setSkinOverride(Skin::kHeader);
}

EffectHeader::~EffectHeader() = default;

Component* EffectHeader::getSettingsAnchor() {
  return settings_button_.get();
}

void EffectHeader::paintBackground(Graphics& g) {
  paintContainer(g);

  g.setColour(findColour(Skin::kBody, true));
  int logo_section_width = getStandardKnobSize() + getPadding();
  g.fillRect(0, 0, logo_section_width, getHeight());

  paintKnobShadows(g);
  paintChildrenBackgrounds(g);

  g.saveState();
  Rectangle<int> bounds = getLocalArea(preset_selector_.get(), preset_selector_->getLocalBounds());
  g.reduceClipRegion(bounds);
  g.setOrigin(bounds.getTopLeft());
  preset_selector_->paintBackground(g);
  g.restoreState();
}

void EffectHeader::resized() {
  const int height = getHeight();
  const int width = getWidth();
  const int large_padding = findValue(Skin::kLargePadding);
  const int logo_width = findValue(Skin::kModulationButtonWidth);
  logo_section_->setBounds(large_padding, 0, logo_width, height);

  // Settings button sits flush right, volume + meter to its left, preset selector in the middle.
  const int settings_size = static_cast<int>(height * 0.6f);
  const int settings_y = (height - settings_size) / 2;
  settings_button_->setBounds(width - large_padding - settings_size, settings_y, settings_size, settings_size);

  const int component_padding = 12 * size_ratio_;
  const int volume_width = std::max(1, width / 5);
  const int volume_x = settings_button_->getX() - component_padding - volume_width;
  volume_section_->setBounds(volume_x, 0, volume_width, height);

  const int selector_height = static_cast<int>(height * 0.6f);
  const int selector_y = (height - selector_height) / 2;
  const int left_limit = logo_section_->getRight() + component_padding;
  const int right_limit = volume_x - component_padding;
  const int selector_width = std::max(1, std::min(width / 3, right_limit - left_limit));
  const int selector_x = left_limit + (right_limit - left_limit - selector_width) / 2;
  preset_selector_->setBounds(selector_x, selector_y, selector_width, selector_height);

  SynthSection::resized();
}

void EffectHeader::reset() {
  if (preset_selector_)
    preset_selector_->resetText();
}

void EffectHeader::buttonClicked(Button* clicked_button) {
  if (clicked_button == settings_button_.get()) {
    for (Listener* listener : listeners_)
      listener->showSettings(settings_button_.get());
  }
  else
    SynthSection::buttonClicked(clicked_button);
}

void EffectHeader::setPresetBrowserVisibility(bool visible) {
  for (Listener* listener : listeners_)
    listener->setPresetBrowserVisibility(visible);
}

void EffectHeader::deleteRequested(File preset) {
  for (Listener* listener : listeners_)
    listener->deleteRequested(preset);
}

void EffectHeader::bankImported() {
  for (Listener* listener : listeners_)
    listener->bankImported();
}

void EffectHeader::showAboutSection() {
  for (Listener* listener : listeners_)
    listener->showAboutSection();
}

void EffectHeader::save(File) {
  preset_selector_->resetText();
  preset_selector_->setModified(false);
}

void EffectHeader::notifyChange() {
  preset_selector_->setModified(true);
}

void EffectHeader::notifyFresh() {
  preset_selector_->resetText();
  preset_selector_->setModified(false);
}
