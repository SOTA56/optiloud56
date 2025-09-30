
#include <obs-module.h>
#include "ui.h"

// We only need enums/struct fields from ui.h:
//   enum class Mode { Mic=0, BGM=1 };
//   struct AutoParams { bool nrOn; float bgmTrimDb; };

// ---- helpers ----

static bool on_mode_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
    UNUSED_PARAMETER(p);
    if (!props || !settings)
        return false;

    int mode = (int)obs_data_get_int(settings, "mode"); // 0: Mic, 1: BGM

    // BGM Trim は BGM のときだけ表示
    obs_property_t *bgm = obs_properties_get(props, "bgm_trim");
    if (bgm) {
        bool showBgmTrim = (mode == 1 /*BGM*/);
        obs_property_set_visible(bgm, showBgmTrim);
    }

    // Noise Reduction は両モードで表示（必要ならここで切替）
    return true;
}

// Learnボタン（3s+5s）押下時：
// props に仕込んだ param に obs_source_t* を入れておくので取り出して、
// settings の "learn" を true にして update → フィルタ側が受け取って実行。
static bool on_learn_clicked(obs_properties_t *props, obs_property_t *p, void *data)
{
    UNUSED_PARAMETER(p);
    obs_source_t *src = (obs_source_t*)obs_properties_get_param(props);
    if (!src) src = (obs_source_t*)data; // 念のため

    if (!src) return false;

    obs_data_t *s = obs_source_get_settings(src);
    if (!s) return false;
    obs_data_set_bool(s, "learn", true);
    obs_source_update(src, s);
    obs_data_release(s);
    return true;
}

obs_properties_t* ui_properties(struct AutoParams *initial, obs_source_t *self)
{
    obs_properties_t *props = obs_properties_create();

    // 後の callback で取れるように self を param に仕込む
    obs_properties_set_param(props, (void*)self, NULL);

    // --- Mode (Mic / BGM) ---
    obs_property_t *mode = obs_properties_add_list(props, "mode", obs_module_text("Mode"),
                                                   OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(mode, obs_module_text("Mic"), 0);
    obs_property_list_add_int(mode, obs_module_text("BGM"), 1);
    if (initial) {
        obs_data_t *tmp = obs_data_create();
        obs_data_set_int(tmp, "mode", (long long)0);
        obs_property_modified_t cb = on_mode_changed;
        obs_property_set_modified_callback(mode, cb);
        obs_data_release(tmp);
    } else {
        obs_property_set_modified_callback(mode, on_mode_changed);
    }

    // --- Noise Reduction ---
    obs_properties_add_bool(props, "nr", obs_module_text("Noise Reduction (simple)"));

    // --- BGM Trim (±6 dB) ---
    obs_property_t *bgm = obs_properties_add_float_slider(props, "bgm_trim",
                                                          obs_module_text("BGM Trim (dB)"),
                                                          -6.0, 6.0, 0.1);
    // Mic 初期表示では隠す (mode==0)
    obs_property_set_visible(bgm, false);

    // --- Learn Button ---
    obs_properties_add_button(props, "learn_btn",
                              obs_module_text("Learn (3s + 5s)"),
                              on_learn_clicked);

    // 互換のため hidden bool を用意（ボタンが true を入れて update する）
    obs_property_t *hidden = obs_properties_add_bool(props, "learn", "learn_internal");
    if (hidden) obs_property_set_visible(hidden, false);

    // 初期値反映（任意）
    if (initial) {
        // default NR off / trim 0dB
        (void)initial;
    }

    return props;
}
