#pragma once
#include <obs-module.h>
#include "filter_autoloudness.h"

inline obs_properties_t* ui_properties(const AutoParams* /*cur*/)
{
  obs_properties_t* p = obs_properties_create();

  obs_property_t* mode = obs_properties_add_list(
      p, "mode", "Mode", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
  obs_property_list_add_int(mode, "Mic", (int)Mode::Mic);
  obs_property_list_add_int(mode, "BGM", (int)Mode::BGM);

  obs_properties_add_float_slider(p, "target_lufs", "Target LUFS", -30.0, -10.0, 0.5);
  obs_properties_add_bool(p, "nr", "Noise Reduction (simple)");
  obs_properties_add_float_slider(p, "bgm_trim", "BGM Trim (dB)", -6.0, 6.0, 0.5);

  return p;
}
