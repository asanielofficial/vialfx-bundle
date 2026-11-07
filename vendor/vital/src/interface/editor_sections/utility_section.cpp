// VialFX - Utility section. Licensed under GPL-3.0-or-later.
#include "utility_section.h"

#include "fonts.h"
#include "open_gl_multi_quad.h"
#include "skin.h"
#include "synth_button.h"
#include "synth_gui_interface.h"
#include "synth_slider.h"
#include "utility_section.h"

#include <cmath>
#include <vector>

// Little LED dots at the left of each pill toggle; shows the toggles that are on (or, with on = false, off).
class UtilityLeds : public OpenGlMultiQuad {
  public:
    UtilityLeds(std::vector<SynthButton*> buttons, bool on) :
        OpenGlMultiQuad(8, Shaders::kCircleFragment), buttons_(buttons), on_(on) {
      setTargetComponent(this);
    }

    void render(OpenGlWrapper& open_gl, bool animate) override {
      const float w = getWidth(), h = getHeight();
      if (w <= 0.0f || h <= 0.0f)
        return;

      int count = 0;
      for (SynthButton* button : buttons_) {
        if (button->getToggleState() != on_)
          continue;
        Rectangle<int> bounds = button->getBounds();
        float led = std::round(bounds.getHeight() * 0.34f);
        float x = bounds.getX() - std::round(bounds.getHeight() * 0.57f);
        float y = bounds.getY() + std::round((bounds.getHeight() - led) * 0.5f);
        setQuad(count++, 2.0f * x / w - 1.0f, 1.0f - 2.0f * (y + led) / h, 2.0f * led / w, 2.0f * led / h);
      }
      if (count == 0)
        return;

      setNumQuads(count);
      setColor(on_ ? findColour(Skin::kBodyText, true) : findColour(Skin::kIconButtonOff, true));
      OpenGlMultiQuad::render(open_gl, animate);
    }

  private:
    std::vector<SynthButton*> buttons_;
    bool on_;
};

// Vertical output level meter (reads the module's peak meter status outputs).
class UtilityMeter : public OpenGlMultiQuad {
  public:
    UtilityMeter() : OpenGlMultiQuad(3, Shaders::kRoundedRectangleFragment), parent_(nullptr),
                     level_(nullptr), peak_(nullptr), smoothed_(0.0f) {
      setTargetComponent(this);
    }

    void init(OpenGlWrapper& open_gl) override {
      if (parent_ == nullptr)
        parent_ = findParentComponentOfClass<SynthGuiInterface>();
      if (parent_ && parent_->getSynth()) {
        level_ = parent_->getSynth()->getStatusOutput("utility_level");
        peak_ = parent_->getSynth()->getStatusOutput("utility_peak");
      }
      OpenGlMultiQuad::init(open_gl);
    }

    void render(OpenGlWrapper& open_gl, bool animate) override {
      float rounding = findValue(Skin::kWidgetRoundedCorner) * 0.5f;
      setRounding(rounding);

      float t = 0.0f;
      if (level_ && animate) {
        float peak = std::max(level_->value()[0], level_->value()[1]);
        float db = vital::utils::magnitudeToDb(peak);
        t = std::max((db + 60.0f) / 66.0f, 0.0f);     // -60 dB .. +6 dB
        t = std::min(t, 1.0f);
      }
      smoothed_ = t > smoothed_ ? t : smoothed_ + (t - smoothed_) * 0.25f;

      float h = getHeight();
      float fill = std::max(smoothed_ * h, 0.0f);
      setColor(findColour(Skin::kWidgetBackground, true));
      setQuad(0, -1.0f, -1.0f, 2.0f, 2.0f);
      setQuadVertical(0, -1.0f, 2.0f);
      OpenGlMultiQuad::setNumQuads(1);
      OpenGlMultiQuad::render(open_gl, animate);

      if (fill > 0.5f) {
        setColor(Colours::white);
        setQuad(0, -1.0f, -1.0f, 2.0f, 2.0f * fill / h);
        OpenGlMultiQuad::render(open_gl, animate);
      }
    }

  private:
    SynthGuiInterface* parent_;
    const vital::StatusOutput* level_;
    const vital::StatusOutput* peak_;
    float smoothed_;
};

UtilitySection::UtilitySection(String name) : SynthSection(name) {
  auto add_toggle = [&](std::unique_ptr<SynthButton>& button, const char* param, const char* text) {
    button = std::make_unique<SynthButton>(param);
    addButton(button.get());
    button->setText(text);
    button->setNoBackground();
    button->setJustification(Justification::centred);
  };
  add_toggle(invert_left_, "utility_invert_left", "L");
  add_toggle(invert_right_, "utility_invert_right", "R");
  add_toggle(swap_, "utility_swap", "Swap L/R");
  add_toggle(dc_filter_, "utility_dc_filter", "DC Filter");
  add_toggle(mono_bass_, "utility_mono_bass", "Mono Bass");

  auto add_knob = [&](std::unique_ptr<SynthSlider>& slider, const char* param, bool bipolar) {
    slider = std::make_unique<SynthSlider>(param);
    addSlider(slider.get());
    slider->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
    if (bipolar) {
      slider->setBipolar();
      slider->snapToValue(true, 0.0);
    }
  };
  add_knob(gain_, "utility_gain", false);
  add_knob(pan_, "utility_pan", true);
  add_knob(spread_, "utility_spread", false);
  add_knob(haas_, "utility_haas", true);
  add_knob(frequency_, "utility_mono_frequency", false);
  gain_->snapToValue(true, 0.0);
  spread_->snapToValue(true, 1.0);

  std::vector<SynthButton*> toggles = { invert_left_.get(), invert_right_.get(), swap_.get(),
                                         dc_filter_.get(), mono_bass_.get() };
  leds_on_ = std::make_unique<UtilityLeds>(toggles, true);
  addOpenGlComponent(leds_on_.get());
  leds_off_ = std::make_unique<UtilityLeds>(toggles, false);
  addOpenGlComponent(leds_off_.get());

  meter_ = std::make_unique<UtilityMeter>();
  addOpenGlComponent(meter_.get());

  on_ = std::make_unique<SynthButton>("utility_on");
  addButton(on_.get());
  setActivator(on_.get());
  setSkinOverride(Skin::kUtility);
}

UtilitySection::~UtilitySection() { }

void UtilitySection::paintBackground(Graphics& g) {
  SynthSection::paintBackground(g);
  setLabelFont(g);

  for (SynthButton* button : { invert_left_.get(), invert_right_.get(), swap_.get(), dc_filter_.get(), mono_bass_.get() }) {
    Rectangle<float> bounds = button->getBounds().toFloat();
    bounds.setLeft(bounds.getX() - std::round(bounds.getHeight() * 0.95f));
    g.setColour(findColour(Skin::kLabelBackground, true));
    g.fillRoundedRectangle(bounds, bounds.getHeight() * 0.5f);
  }

  drawLabelForComponent(g, TRANS("GAIN"), gain_.get());
  drawLabelForComponent(g, TRANS("PAN"), pan_.get());
  drawLabelForComponent(g, TRANS("SPREAD"), spread_.get());
  drawLabelForComponent(g, TRANS("HAAS"), haas_.get());
  drawLabelForComponent(g, TRANS("FREQ"), frequency_.get());

  const float ratio_x = getWidth() / 580.0f, ratio_y = getHeight() / 136.5f;
  g.setColour(findColour(Skin::kBodyText, true).withAlpha(0.9f));
  g.setFont(Fonts::instance()->proportional_light().withPointHeight(14.0f * ratio_y));
  g.drawText(TRANS("Polarity Invert"), Rectangle<int>(std::round(42 * ratio_x), std::round(2 * ratio_y),
                                                        std::round(90 * ratio_x), std::round(26 * ratio_y)),
             Justification::centredLeft, false);
}

void UtilitySection::resized() {
  const float ratio_x = getWidth() / 580.0f, ratio_y = getHeight() / 136.5f;
  auto rect = [&](float x, float y, float w, float h) {
    return Rectangle<int>(std::round(x * ratio_x), std::round(y * ratio_y),
                          std::round(w * ratio_x), std::round(h * ratio_y));
  };

  // Each pill is the button plus an LED zone of ~22 px on its left.
  invert_left_->setBounds(rect(155.0f, 5.0f, 13.0f, 23.0f));
  invert_right_->setBounds(rect(203.0f, 5.0f, 13.0f, 23.0f));
  swap_->setBounds(rect(257.0f, 5.0f, 50.0f, 23.0f));
  dc_filter_->setBounds(rect(351.0f, 5.0f, 51.0f, 23.0f));
  mono_bass_->setBounds(rect(443.0f, 5.0f, 60.0f, 23.0f));

  gain_->setBounds(rect(32.0f, 58.0f, 84.0f, 66.0f));
  pan_->setBounds(rect(130.0f, 58.0f, 84.0f, 66.0f));
  spread_->setBounds(rect(224.0f, 58.0f, 84.0f, 66.0f));
  haas_->setBounds(rect(321.0f, 58.0f, 84.0f, 66.0f));
  frequency_->setBounds(rect(416.0f, 58.0f, 84.0f, 66.0f));
  meter_->setBounds(rect(529.0f, 15.0f, 13.0f, 96.0f));
  leds_on_->setBounds(getLocalBounds());
  leds_off_->setBounds(getLocalBounds());

  SynthSection::resized();
}
