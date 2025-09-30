
#include "filter_autoloudness.h"
#include "ui.h"
#include <vector>
#include <algorithm>
#include <cmath>

static inline float clampf(float x, float lo, float hi){ return std::max(lo, std::min(hi, x)); }
static const char* S_ID = "optiloud56_filter";

struct filter_context {
  obs_source_t*        self;
  AutoLoudnessFilter*  impl;
  AutoParams           params;
};

obs_audio_data* AutoLoudnessFilter::process(obs_audio_data* audio)
{
  if (!audio) return nullptr;
  const size_t frames = audio->frames;
  const int ch = params_.channels;

  std::vector<float*> planes(ch);
  for (int c = 0; c < ch; ++c) planes[c] = reinterpret_cast<float*>(audio->data[c]);

  if (learning_.load()) {
    const float mLUFS = loud_.block_lufs((float const* const*)planes.data(), ch, frames);
    if (mLUFS > -1e8f && std::isfinite(mLUFS)) {
      const float diff = clampf(params_.targetLUFS - mLUFS, -24.0f, 24.0f);
      appliedGainDb_ = 0.7f * appliedGainDb_ + 0.3f * diff;
    }
    learning_.store(false);
  }

  const float instLUFS = loud_.block_lufs((float const* const*)planes.data(), ch, frames);
  if (instLUFS > -1e8f && std::isfinite(instLUFS)) {
    float micro = 0.05f * (params_.targetLUFS - instLUFS);
    micro = clampf(micro, -0.5f, 0.5f);
    appliedGainDb_ = 0.98f * appliedGainDb_ + 0.02f * (appliedGainDb_ + micro);
    appliedGainDb_ = clampf(appliedGainDb_, -24.0f, 24.0f);
  }

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

static const char* flt_get_name(void*) { return "OptiLoud56 (ButtonUI)"; }

static void* flt_create(obs_data_t* settings, obs_source_t* source)
{
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

static void flt_destroy(void* data)
{
  auto* ctx = (filter_context*)data;
  if (!ctx) return;
  delete ctx->impl;
  delete ctx;
}

// expose current source to UI (button callback)
obs_source_t* g_ui_current_source = nullptr;

static obs_properties_t* flt_get_properties(void* data)
{
  auto* ctx = (filter_context*)data;
  g_ui_current_source = ctx ? ctx->self : nullptr;
  return ui_properties(ctx ? &ctx->params : nullptr);
}

static void flt_update(void* data, obs_data_t* settings)
{
  auto* ctx = (filter_context*)data;
  if (!ctx) return;

  ctx->params.mode      = (Mode)obs_data_get_int(settings, "mode");
  ctx->params.nrOn      = obs_data_get_bool(settings, "nr");
  ctx->params.bgmTrimDb = (float)obs_data_get_double(settings, "bgm_trim");

  if (obs_data_get_bool(settings, "learn")) {
    ctx->impl->request_learn();
    obs_data_set_bool(settings, "learn", false);
  }
  ctx->impl->update(ctx->params);
}

static obs_audio_data* flt_filter_audio(void* data, obs_audio_data* audio)
{
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
