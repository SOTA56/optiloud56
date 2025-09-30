
#include "dsp_loudness.h"
#include <cmath>

namespace dsp {

static inline float safe_log10(float x) {
  if (x <= 0.f) return -1e9f;
  return std::log10f(x);
}

float LoudnessEstimator::block_lufs(float const* const* planes, int ch, size_t frames) const
{
  const int C = ch > 0 ? ch : ch_;
  double sumsq = 0.0;
  size_t n = frames * (size_t)C;
  for (int c = 0; c < C; ++c) {
    const float* s = planes[c];
    for (size_t i = 0; i < frames; ++i) {
      const float v = s[i];
      sumsq += (double)v * (double)v;
    }
  }
  if (n == 0) return -1e9f;
  const double rms = std::sqrt(sumsq / (double)n);
  if (rms <= 3.162277660e-4) { // -70 dBFS gate
    return -1e9f;
  }
  const float lufs = 20.0f * (float)safe_log10((float)rms);
  return lufs;
}

} // namespace dsp
