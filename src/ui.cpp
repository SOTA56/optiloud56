
#include "ui.h"
#include <obs-module.h>

// Global current source provided by filter's get_properties
extern obs_source_t* g_ui_current_source;

static const char* K_MODE       = "mode";
static const char* K_NR         = "nr";
static const char* K_BGMTRIM    = "bgm_trim";
static const char* K_LEARN      = "learn";
static const char* K_LEARN_INFO = "learn_info";

// OBS expects 3-arg signature with obs_data_t*
static bool on_mode_changed(obs_properties_t* props, obs_property_t* prop, obs_data_t* settings)
{
  (void)prop;
  if (!props || !settings) return false;
  const int mode = (int)obs_data_get_int(settings, K_MODE);
  obs_property_t* pTrim = obs_properties_get(props, K_BGMTRIM);
  if (pTrim) {
    const bool visible = (mode == 1); // 1=BGM
    obs_property_set_visible(pTrim, visible);
  }
  return true;
}

static bool on_learn_button(obs_properties_t* props, obs_property_t* p, void* data)
{
  (void)props; (void)p; (void)data;
  if (!g_ui_current_source) return false;
  obs_data_t* s = obs_source_get_settings(g_ui_current_source);
  if (!s) return false;
  obs_data_set_bool(s, K_LEARN, true);
  obs_source_update(g_ui_current_source, s);
  obs_data_release(s);
  return true;
}

obs_properties_t* ui_properties(AutoParams* params)
{
  obs_properties_t* props = obs_properties_create();

  obs_property_t* mode = obs_properties_add_list(props, K_MODE, "Mode",
                            OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
  obs_property_list_add_int(mode, "Mic", 0);
  obs_property_list_add_int(mode, "BGM", 1);
  obs_property_set_modified_callback(mode, on_mode_changed);

  obs_properties_add_bool(props, K_NR, "Noise Reduction (simple)");

  obs_property_t* pTrim = obs_properties_add_float_slider(props, K_BGMTRIM, "BGM Trim (dB)", -6.0, 6.0, 0.1);
  if (params) {
    const bool visible = (params->mode == Mode::BGM);
    obs_property_set_visible(pTrim, visible);
  }

  obs_properties_add_button(props, "learn_button", "Learn (3s + 5s)", on_learn_button);
  // hidden bool as trigger storage
  obs_property_t* hidden = obs_properties_add_bool(props, K_LEARN, "learn_internal");
  if (hidden) obs_property_set_visible(hidden, false);

  obs_properties_add_text(props, K_LEARN_INFO, "", OBS_TEXT_INFO);
  return props;
}
