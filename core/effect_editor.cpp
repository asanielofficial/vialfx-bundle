// VialFX - Licensed under GPL-3.0-or-later. Based on Vital's SynthEditor by Matt Tytel (GPL-3.0).
#include "effect_editor.h"

#include "default_look_and_feel.h"
#include "effect_info.h"

EffectEditor::EffectEditor(EffectPlugin& plugin) :
    AudioProcessorEditor(&plugin), SynthGuiInterface(&plugin), plugin_(plugin) {
  setLookAndFeel(DefaultLookAndFeel::instance());

  gui_->reset();
  gui_->animate(true);

  const Point<int> design = FullInterface::getDesignSize();
  constrainer_.setMinimumSize(design.x / 2, design.y / 2);
  constrainer_.setFixedAspectRatio(static_cast<double>(design.x) / design.y);
  constrainer_.setGui(gui_.get());
  setConstrainer(&constrainer_);

  addAndMakeVisible(gui_.get());
  setResizable(true, true);
  setSize(design.x, design.y);
}

void EffectEditor::resized() {
  AudioProcessorEditor::resized();
  gui_->setBounds(getLocalBounds());
}

void EffectEditor::setScaleFactor(float newScale) {
  AudioProcessorEditor::setScaleFactor(newScale);
  gui_->redoBackground();
}

void EffectEditor::updateFullGui() {
  SynthGuiInterface::updateFullGui();
  plugin_.updateHostDisplay();
}
