#pragma once
#include <obs-module.h>
#include <atomic>

enum class Mode { Mic = 0, BGM = 1 };
enum class CompPreset { Soft = 0, Medium = 1, Strong = 2 };

struct AutoParams {
  Mode  mode {Mode::Mic};
  float targetLUFS {-16.0f};
  bool  nrOn {false};
  float bgmTrimDb {0.0f};
  int   samplerate {48000};
  int   channels {2};
};

class AutoLoudnessFilter {
public:
  AutoLoudnessFilter(obs_source_t* self, const AutoParams& p): self_(self), params_(p) {}
  ~AutoLoudnessFilter() = default;

  void update(const AutoParams& p) { params_ = p; }
  void request_learn() { learning_.store(true); }

  obs_audio_data* process(obs_audio_data* audio) {
    // Pass-through (minimum)
    (void)audio;
    return audio;
  }

private:
  obs_source_t* self_;
  AutoParams params_;
  std::atomic<bool> learning_{false};
};
