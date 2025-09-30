// UI_BUILD: BUTTON-V2
#include "ui.h"
#include <obs-module.h>

extern "C" {
    obs_properties_t* ui_properties(const AutoParams* init);
}

static inline int get_init_mode(const AutoParams* init) {
    if (!init) return 0; // default Mic
    return (int)init->mode; // Mode::Mic==0, Mode::BGM==1 (as defined in your headers)
}

obs_properties_t* ui_properties(const AutoParams* init)
{
    obs_properties_t* props = obs_properties_create();

    // Mode
    obs_property_t* pmode = obs_properties_add_list(
        props, "mode", "Mode", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(pmode, "Mic", 0);
    obs_property_list_add_int(pmode, "BGM", 1);

    // Noise Reduction
    obs_properties_add_bool(props, "nr", "Noise Reduction (simple)");

    // Show BGM Trim only when initial mode is BGM
    if (get_init_mode(init) == 1) {
        obs_properties_add_float_slider(props, "bgm_trim", "BGM Trim (dB)", -6.0, 6.0, 0.1);
    }

    // Learn (checkbox that triggers countdown+5s measure in the filter)
    obs_properties_add_bool(props, "learn", "Learn (3s countdown + 5s measure)");

    // Verification banner
    obs_properties_add_text(props, "__ui_build", "UI Build: BUTTON-V2", OBS_TEXT_INFO);

    return props;
}
