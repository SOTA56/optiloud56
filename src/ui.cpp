#include <obs-module.h>
#include "ui.h"

// Keep keys stable to match filter_autoloudness.cpp
#define KEY_MODE        "mode"
#define KEY_TARGET_LUFS "target_lufs"
#define KEY_NR          "nr"
#define KEY_BGM_TRIM    "bgm_trim"
#define KEY_LEARN       "learn"

// Build a very safe, dependency-free UI. Dynamic show/hide is skipped for compatibility.
extern "C" obs_properties_t* optiloud56_build_properties(const AutoParams* /*init*/)
{
    obs_properties_t* props = obs_properties_create();

    // Mode
    obs_property_t* p_mode = obs_properties_add_list(
        props, KEY_MODE, "Mode",
        OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(p_mode, "Mic", 0);
    obs_property_list_add_int(p_mode, "BGM", 1);

    // Target LUFS (will be ignored for BGM; kept for now)
    obs_properties_add_float_slider(props, KEY_TARGET_LUFS, "Target LUFS", -20.0, -14.0, 0.1);

    // Noise reduction toggle
    obs_properties_add_bool(props, KEY_NR, "Noise Reduction (simple)");

    // BGM Trim
    obs_properties_add_float_slider(props, KEY_BGM_TRIM, "BGM Trim (dB)", -6.0, 6.0, 0.1);

    // Learn (checkbox acts like a button; processing side resets it to false)
    obs_properties_add_bool(props, KEY_LEARN, "Learn (analyze short section)");

    // Footer tag to verify the UI build is loaded
    obs_properties_add_text(props, "ui_tag", "UI Build: BUTTON-V3", OBS_TEXT_INFO);

    return props;
}