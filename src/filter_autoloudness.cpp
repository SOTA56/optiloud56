#include "filter_autoloudness.h"
#include "ui.h"

static const char* S_ID = "optiloud56_filter";

/* OBS glue */
struct filter_context {
  obs_source_t*        self;
  AutoLoudnessFilter*  impl;
  AutoParams           params;
};

static const char* flt_get_name(void*) { return "OptiLoud56"; }

static void* flt_create(obs_data_t* settings, obs_source_t* source)
{
  auto* ctx = new filter_context();
  ctx->self = source;

  ctx->params.mode       = (Mode)obs_data_get_int(settings, "mode");
  ctx->params.targetLUFS = (float)obs_data_get_double(settings, "target_lufs");
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

static obs_properties_t* flt_get_properties(void* data)
{
  auto* ctx = (filter_context*)data;
  return ui_properties(ctx ? &ctx->params : nullptr);
}

static void flt_update(void* data, obs_data_t* settings)
{
  auto* ctx = (filter_context*)data;
  if (!ctx) return;

  ctx->params.mode       = (Mode)obs_data_get_int(settings, "mode");
  ctx->params.targetLUFS = (float)obs_data_get_double(settings, "target_lufs");
  ctx->params.nrOn       = obs_data_get_bool(settings, "nr");
  ctx->params.bgmTrimDb  = (float)obs_data_get_double(settings, "bgm_trim");

  ctx->impl->update(ctx->params);
}

static obs_audio_data* flt_filter_audio(void* data, obs_audio_data* audio)
{
  auto* ctx = (filter_context*)data;
  if (!ctx || !audio) return audio;
  return ctx->impl->process(audio); // pass-through
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
