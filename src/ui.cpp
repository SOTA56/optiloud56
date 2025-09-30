#include "ui.h"
#include <obs-module.h>
#include <string>

// expose current source for button callback (set in filter side)
obs_source_t* g_ui_current_source = nullptr;

// keys
static const char* K_MODE       = "mode";
static const char* K_NR         = "nr";
static const char* K_BGMTRIM    = "bgm_trim";
static const char* K_LEARN_HID  = "learn";       // hidden flag used internally
static const char* K_LEARN_INFO = "learn_info";  // info text

// refresh properties when Mode changes
static bool on_mode_changed(obs_properties_t*, obs_property_t*, void*) {
  return true; // tell OBS to re-fetch properties
}

// Learn button callback: set hidden flag, update to trigger flt_update()
static bool on_learn_button(obs_properties_t*, obs_property_t*, void*) {
  if (!g_ui_current_source) return true;
  obs_data_t* s = obs_source_get_settings(g_ui_current_source);
  obs_data_set_bool(s, K_LEARN_HID, true);
  obs_source_update(g_ui_current_source, s);
  obs_data_release(s);
  return true;
}

obs_properties_t* ui_properties(AutoParams* params)
{
  obs_properties_t* props = obs_properties_create();

  // Mode
  obs_property_t* p = obs_properties_add_list(props, K_MODE, "Mode",
                        OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
  obs_property_list_add_int(p, "Mic", (int)Mode::Mic);
  obs_property_list_add_int(p, "BGM", (int)Mode::BGM);
  obs_property_set_long_description(p, "Mic: マイク用。BGM: ミキシング済み音源向け。");
  obs_property_set_modified_callback(p, on_mode_changed);

  // NR (placeholder)
  p = obs_properties_add_bool(props, K_NR, "Noise Reduction (simple)");
  obs_property_set_long_description(p, "定常ノイズを軽く減衰（後フェーズで実装）。");

  // BGM trim (only visible on BGM)
  obs_property_t* pTrim = obs_properties_add_float_slider(props, K_BGMTRIM, "BGM Trim (dB)", -6.0, 6.0, 0.1);
  if (params) {
    bool visible = (params->mode == Mode::BGM);
    obs_property_set_visible(pTrim, visible);
  }

  // Learn button
  obs_property_t* pBtn = obs_properties_add_button(props, "learn_button", "Learn (3s + 5s)", on_learn_button);
  (void)pBtn;

  // Learn status text
  p = obs_properties_add_text(props, K_LEARN_INFO, "", OBS_TEXT_INFO);
  (void)p;
  return props;
}
