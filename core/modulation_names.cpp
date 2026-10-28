// VialFX - Licensed under GPL-3.0-or-later.
// These two helpers are copied unchanged from Vital's modulation_matrix.cpp (Matt Tytel, GPL-3.0).
// Vital's slider and modulation-button code still call them; the rest of the modulation matrix
// is not part of single-effect plugins.
#include "modulation_matrix.h"

String ModulationMatrix::getMenuSourceDisplayName(const String& original) {
  if (original == "aftertouch")
    return "After Touch";

  String modified = original.replaceFirstOccurrenceOf("control_", "");
  StringArray tokens;
  tokens.addTokens(modified, "_", "");
  String result;
  for (const String& token : tokens) {
    String capitalized = String(token.substring(0, 1)).toUpperCase() + token.substring(1);
    result += capitalized + " ";
  }

  return result.trim();
}

String ModulationMatrix::getUiSourceDisplayName(const String& original) {
  return getMenuSourceDisplayName(original).toUpperCase();
}
