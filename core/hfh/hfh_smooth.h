#pragma once
/*
 *  HFH core — parameter smoothing
 *  Every continuous control goes through one of these so nothing ever clicks
 *  or zippers, however fast the pad is swept.
 */

#include "hfh_math.h"

namespace hfh {

// One-pole (exponential) smoother. Good default for most controls.
class Smoother {
 public:
  explicit Smoother(float ms = 20.f) { setTime(ms); }
  void setTime(float ms) { c_ = timeCoef(ms); }
  void setTarget(float t) { target_ = t; }
  void snap(float v) { target_ = z_ = v; }
  float next() { z_ += c_ * (target_ - z_); return z_; }
  // Advance a whole block at once (for coefficients updated per block).
  float nextBlock(uint32_t frames) {
    // (1-c)^frames via exp: one call per block, not per sample.
    const float k = 1.f - fexp2((float)frames * flog2(1.f - c_ + 1e-12f));
    z_ += k * (target_ - z_);
    return z_;
  }
  float value() const { return z_; }
  float target() const { return target_; }

 private:
  float c_ = 1.f, z_ = 0.f, target_ = 0.f;
};

// Linear ramp that arrives exactly on time. Best for gains and crossfades.
class Ramp {
 public:
  explicit Ramp(float ms = 10.f) { setTime(ms); }
  void setTime(float ms) { samples_ = maxf(1.f, msToSamples(ms)); }
  void setTarget(float t) {
    if (t == target_) return;
    target_ = t;
    step_ = (target_ - z_) / samples_;
    left_ = (uint32_t)samples_;
  }
  void snap(float v) { z_ = target_ = v; left_ = 0; }
  float next() {
    if (left_) { z_ += step_; if (--left_ == 0) z_ = target_; }
    return z_;
  }
  bool busy() const { return left_ != 0; }
  float value() const { return z_; }

 private:
  float samples_ = 480.f, z_ = 0.f, target_ = 0.f, step_ = 0.f;
  uint32_t left_ = 0;
};

}  // namespace hfh
