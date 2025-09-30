
#pragma once
#include <cstddef>

namespace dsp {

class LoudnessEstimator {
public:
  LoudnessEstimator(int samplerate, int channels)
    : sr_(samplerate), ch_(channels) {}

  void set_channels(int ch) { ch_ = ch; }

  float block_lufs(float const* const* planes, int ch, size_t frames) const;

private:
  int sr_;
  int ch_;
};

} // namespace dsp
