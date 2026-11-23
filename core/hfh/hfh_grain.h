#pragma once
/*
 *  HFH core — grain windows and envelopes
 *  Window shape is most of what makes granular sound smooth or gritty.
 */

#include "hfh_math.h"

namespace hfh {

// Raised cosine (Hann): smoothest, the "cloud" default. p in 0..1.
inline float windowHann(float p) {
  const float s = fsin(clampf(p, 0.f, 1.f) * kPi);
  return s * s;
}

// Tukey: flat top with cosine edges. taper 0 = rectangle, 1 = Hann.
// Keeps more of the grain at full level, so dense clouds stay louder and clearer.
inline float windowTukey(float p, float taper) {
  p = clampf(p, 0.f, 1.f);
  const float edge = 0.5f * clampf(taper, 0.001f, 1.f);
  if (p < edge) { const float s = fsin(p / edge * kHalfPi); return s * s; }
  if (p > 1.f - edge) { const float s = fsin((1.f - p) / edge * kHalfPi); return s * s; }
  return 1.f;
}

// Attack/release envelope with exponential-feel curves, for gates and
// freeze/unfreeze fades.
class ArEnv {
 public:
  void set(float attackMs, float releaseMs) { a_ = timeCoef(attackMs); r_ = timeCoef(releaseMs); }
  void gate(bool on) { gate_ = on; }
  float next() { z_ += (gate_ ? a_ : r_) * ((gate_ ? 1.f : 0.f) - z_); return z_; }
  float value() const { return z_; }
 private:
  float a_ = 1.f, r_ = 1.f, z_ = 0.f;
  bool gate_ = false;
};

}  // namespace hfh
