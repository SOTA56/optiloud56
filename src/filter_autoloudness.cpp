#include "filter_autoloudness.h"
#include "ui.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdio>
#include <cmath>
#include <algorithm>

// ==== Config ====
static constexpr int   kCountdownSec = 3;   // 3-second countdown
static constexpr int   kMeasureSec   = 5;   // 5-second measuring
static constexpr float kTargetLUFS   = -16.0f; // fixed target

static inline float clampf(float x, float lo, float hi) { return std::max(lo, std::min(hi, x)); }

// Per-source Learn state
struct LearnState {
  int64_t samples_left = -1; // remaining samples for current phase
  int     phase = 0;         // 0 = countdown, 1 = measuring
  double  sum_lufs = 0.0;
  int     n_blocks = 0;
};

static std::unordered_map<obs_source_t*, LearnState> g_learn_states;

// provided by ui.cpp so button callback knows "current" source in props dialog
extern obs_source_t* g_ui_current_source;

static const char* S_ID = "optiloud56_filter";

AutoLoudnessFilter::AutoLoudnessFilter(obs_source_t* self, const AutoParams& p)
: self_(self), params_(p), loud_(p.samplerate, p.channels), comp_(p.samplerate), lim_(p.samplerate), nr_(p.samplerate, p.channels) {}

AutoLoudnessFilter::~AutoLoudnessFilter() {}

void AutoLoudnessFilter::update(const AutoParams& p) {
  params_ = p;
  loud_.set_channels(p.channels);
}

void AutoLoudnessFilter::request_learn() { learning_.store(true); }

// helper to compute LUFS for current block (symbol exists in dsp_loudness)
static inline float block_lufs_wrap(decltype(AutoLoudnessFilter::loud_)& L, float const* const* planes, int ch, size_t frames) {
  return L.block_lufs(planes, ch, frames);
}

obs_audio_data* AutoLoudnessFilter::process(obs_audio_data* audio) {
  if (!audio) return nullptr;
  const size_t frames = audio->frames;
  const int ch = params_.channels;

  std::vector<float*> planes(ch);
  for (int c = 0; c < ch; ++c) planes[c] = reinterpret_cast<float*>(audio->data[c]);

  // 1) Handle Learn trigger (from button -> flt_update -> request_learn())
  if (learning_.exchange(false)) {
    LearnState st;
    st.phase = 0; // countdown first
    st.samples_left = static_cast<int64_t>(params_.samplerate) * kCountdownSec;
    g_learn_states[self_] = st;

    // status text
    obs_data_t* s = obs_source_get_settings(self_);
    obs_data_set_string(s, "learn_info", "準備中… 3秒後に5秒測定します");
    obs_source_update(self_, s);
    obs_data_release(s);
  }

  // 2) Learn state machine
  auto it = g_learn_states.find(self_);
  if (it != g_learn_states.end()) {
    LearnState& st = it->second;

    if (st.phase == 0) {
      st.samples_left -= (int64_t)frames;
      if (st.samples_left <= 0) {
        st.phase = 1;
        st.samples_left = static_cast<int64_t>(params_.samplerate) * kMeasureSec;
        st.sum_lufs = 0.0;
        st.n_blocks = 0;

        obs_data_t* s = obs_source_get_settings(self_);
        obs_data_set_string(s, "learn_info", "測定中… 5秒お話しください");
        obs_source_update(self_, s);
        obs_data_release(s);
      }
    } else if (st.phase == 1) {
      const float lufs = block_lufs_wrap(loud_, (float const* const*)planes.data(), ch, frames);
      if (std::isfinite(lufs) && lufs > -70.0f) {
        st.sum_lufs += (double)lufs;
        st.n_blocks += 1;
      }
      st.samples_left -= (int64_t)frames;
      if (st.samples_left <= 0) {
        float measured = kTargetLUFS;
        if (st.n_blocks > 0) measured = (float)(st.sum_lufs / (double)st.n_blocks);

        const float diff = clampf(kTargetLUFS - measured, -24.0f, 24.0f);
        appliedGainDb_ = 0.7f * appliedGainDb_ + 0.3f * diff;

        char msg[128];
        std::snprintf(msg, sizeof(msg), "学習完了: 推定 %.1f LUFS → ゲイン %+0.1f dB 適用", measured, diff);

        obs_data_t* s = obs_source_get_settings(self_);
        obs_data_set_string(s, "learn_info", msg);
        // also clear hidden learn flag if any
        obs_data_set_bool(s, "learn", false);
        obs_source_update(self_, s);
        obs_data_release(s);

        g_learn_states.erase(it);
      }
    }
  }

  // 3) Micro-follow (very slow)
  const float instLUFS = block_lufs_wrap(loud_, (float const* const*)planes.data(), ch, frames);
  if (std::isfinite(instLUFS)) {
    float micro = 0.05f * (kTargetLUFS - instLUFS);
    micro = clampf(micro, -0.5f, 0.5f);
    appliedGainDb_ = 0.98f * appliedGainDb_ + 0.02f * (appliedGainDb_ + micro);
    appliedGainDb_ = clampf(appliedGainDb_, -24.0f, 24.0f);
  }

  // 4) Apply linear gain + BGM trim
  float lin = std::pow(10.0f, appliedGainDb_ / 20.0f);
  if (params_.mode == Mode::BGM) {
    lin *= std::pow(10.0f, params_.bgmTrimDb / 20.0f);
  }
  for (int c = 0; c < ch; ++c) {
    float* s = planes[c];
    for (size_t i = 0; i < frames; ++i) s[i] *= lin;
  }

  return audio;
}

// -------- OBS glue (minimal) --------
static const char* flt_get_name(void*) { return "OptiLoud56"; }

struct filter_context {
  obs_source_t* self;
  AutoLoudnessFilter* impl;
  AutoParams params;
};

static void* flt_create(obs_data_t* settings, obs_source_t* source) {
  auto* ctx = new filter_context();
  ctx->self = source;
  ctx->params.mode       = (Mode)obs_data_get_int(settings, "mode");
  ctx->params.nrOn       = obs_data_get_bool(settings, "nr");
  ctx->params.bgmTrimDb  = (float)obs_data_get_double(settings, "bgm_trim");

  audio_t* ao = obs_get_audio();
  const audio_output_info* ai = audio_output_get_info(ao);
  ctx->params.samplerate = ai ? ai->samples_per_sec : 48000;
  ctx->params.channels   = ai ? (int)ai->speakers : 2;

  ctx->impl = new AutoLoudnessFilter(source, ctx->params);
  return ctx;
}

static void flt_destroy(void* data) {
  auto* ctx = (filter_context*)data;
  if (!ctx) return;
  delete ctx->impl;
  delete ctx;
}

static obs_properties_t* flt_get_properties(void* data) {
  auto* ctx = (filter_context*)data;
  // let UI know which source is active
  g_ui_current_source = ctx ? ctx->self : nullptr;
  return ui_properties(ctx ? &ctx->params : nullptr);
}

static void flt_update(void* data, obs_data_t* settings) {
  auto* ctx = (filter_context*)data;
  if (!ctx) return;
  ctx->params.mode       = (Mode)obs_data_get_int(settings, "mode");
  ctx->params.nrOn       = obs_data_get_bool(settings, "nr");
  ctx->params.bgmTrimDb  = (float)obs_data_get_double(settings, "bgm_trim");

  // Hidden flag set by Learn button
  if (obs_data_get_bool(settings, "learn")) {
    ctx->impl->request_learn();
    obs_data_set_bool(settings, "learn", false); // clear
  }
  ctx->impl->update(ctx->params);
}

static obs_audio_data* flt_filter_audio(void* data, obs_audio_data* audio) {
  auto* ctx = (filter_context*)data;
  if (!ctx || !audio) return audio;
  return ctx->impl->process(audio);
}

extern "C" {
struct obs_source_info optiloud56_filter_info = {
  .id            = S_ID,
  .type          = OBS_SOURCE_TYPE_FILTER,
  .output_flags  = OBS_SOURCE_AUDIO,
  .get_name      = flt_get_name,
  .create        = flt_create,
  .destroy       = flt_destroy,
  .get_properties= flt_get_properties,
  .update        = flt_update,
  .filter_audio  = flt_filter_audio,
};
}
