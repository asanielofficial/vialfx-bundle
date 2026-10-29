// VialFX - Licensed under GPL-3.0-or-later. Based on Vital by Matt Tytel (GPL-3.0).
#include "effect_state.h"

namespace vitalfx {

File getUserPresetDirectory(const EffectInfo& info) {
  File dir = File::getSpecialLocation(File::userDocumentsDirectory)
                 .getChildFile("VialFX")
                 .getChildFile("Bundle")
                 .getChildFile(info.preset_folder)
                 .getChildFile("User")
                 .getChildFile("Presets");
  if (!dir.exists())
    dir.createDirectory();
  return dir;
}

}  // namespace vitalfx
