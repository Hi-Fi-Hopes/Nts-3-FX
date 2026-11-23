#pragma once
/*
 *  HFH core — pitch shifter
 *  Classic delay-line shifter: two read taps sweep through a short window,
 *  half a window apart, each faded with a Hann window so they sum to unity.
 *  Cheap and smooth; the small "warble" it adds disappears inside reverb
 *  feedback, which is where shimmer effects use it.
 *
 *  Memory: kBufferFloats floats per shifter (mono).
 */

#include "hfh_math.h"
#include "hfh_delay.h"
#include "hfh_grain.h"

namespace hfh {

class PitchShifter {
 public:
  static constexpr uint32_t kBufferFloats = 8192;   // up to ~165 ms window

  void init(float *buffer) {
    line_.init(buffer, kBufferFloats);
    setWindowMs(80.f);
    setSemitones(12.f);
    phase_ = 0.f;
  }
  void setRatio(float r) { ratio_ = clampf(r, 0.25f, 4.f); }
  void setSemitones(float st) { setRatio(fexp2(st * (1.f / 12.f))); }
  void setWindowMs(float ms) { win_ = clampf(msToSamples(ms), 256.f, (float)kBufferFloats - 8.f); }

  float process(float x) {
    line_.write(x);
    // Delay shrinks (pitch up) or grows (pitch down) by (ratio - 1) per sample.
    phase_ += (1.f - ratio_) / win_;
    phase_ -= (float)floorToInt(phase_);
    float p2 = phase_ + 0.5f;
    if (p2 >= 1.f) p2 -= 1.f;
    const float a = line_.readHermite(2.f + phase_ * win_) * windowHann(phase_);
    const float b = line_.readHermite(2.f + p2 * win_) * windowHann(p2);
    return a + b;
  }

 private:
  DelayLine line_;
  float ratio_ = 2.f, win_ = 3840.f, phase_ = 0.f;
};

}  // namespace hfh
