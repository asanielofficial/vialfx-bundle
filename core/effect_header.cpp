// VialFX - Licensed under GPL-3.0-or-later. Based on Vital's HeaderSection by Matt Tytel (GPL-3.0).
#include "effect_header.h"

#include "skin.h"
#include "synth_button.h"

namespace {
  // Cogwheel icon (8 trapezoid teeth around a ring, centre hole), in the 0..1 unit square.
  Path makeGearPath() {
    constexpr int kTeeth = 8;
    const float pi = MathConstants<float>::pi;
    const float outer = 0.48f, root = 0.37f, hole = 0.16f;
    const float tip_half = 0.17f, base_half = 0.27f;

    Path gear;
    for (int i = 0; i < kTeeth; ++i) {
      float a = 2.0f * pi * i / kTeeth;
      const float radii[4] = { root, outer, outer, root };
      const float angles[4] = { a - base_half, a - tip_half, a + tip_half, a + base_half };
      for (int k = 0; k < 4; ++k) {
        Point<float> pt(0.5f + radii[k] * std::cos(angles[k]), 0.5f + radii[k] * std::sin(angles[k]));
        if (i == 0 && k == 0)
          gear.startNewSubPath(pt);
        else
          gear.lineTo(pt);
      }
    }
    gear.closeSubPath();
    gear.addEllipse(0.5f - hole, 0.5f - hole, 2.0f * hole, 2.0f * hole);
    gear.setUsingNonZeroWinding(false);  // even-odd keeps the centre hole open
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

  spectrogram_ = std::make_unique<Spectrogram>();
  addOpenGlComponent(spectrogram_.get());
  spectrogram_->paintBackgroundLines(false);
  spectrogram_->setMinDb(-40.0f);
  spectrogram_->setMaxDb(0.0f);
  spectrogram_->setInterceptsMouseClicks(false, false);

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
  const int widget_margin = findValue(Skin::kWidgetMargin);
  logo_section_->setBounds(0, 0, 0, height);

  // Left: small spectrum.  Centre: preset selector (centred on the window).  Right: volume + cog.
  const int side_width = std::max(1, width / 5);
  spectrogram_->setColour(Skin::kBody, findColour(Skin::kBackground, true));
  spectrogram_->setBounds(large_padding, widget_margin, side_width, height - 2 * widget_margin);

  const int settings_size = static_cast<int>(height * 0.55f);
  settings_button_->setBounds(width - large_padding - settings_size, (height - settings_size) / 2,
                              settings_size, settings_size);

  const int component_padding = 12 * size_ratio_;
  const int volume_x = settings_button_->getX() - component_padding - side_width;
  volume_section_->setBounds(volume_x, 0, side_width, height);

  const int selector_height = static_cast<int>(height * 0.6f);
  const int selector_width = std::max(1, std::min(width / 3, volume_x - spectrogram_->getRight() - 2 * component_padding));
  preset_selector_->setBounds((width - selector_width) / 2, (height - selector_height) / 2,
                              selector_width, selector_height);

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
