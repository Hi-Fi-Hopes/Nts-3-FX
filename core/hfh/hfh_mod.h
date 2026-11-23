#pragma once
/*
 *  HFH core — modulation sources
 *  - Rng: fast xorshift noise
 *  - Drift: smooth random wander. The "nothing is ever static" ingredient.
 *  - Lfo: sine / triangle / square
 *  - Ramper: Chase Bliss-style ramping. A parameter either BOUNCEs
 *    continuously between its knob position and a second position, or RISEs /
 *    FALLs once and holds. Shapes: smooth (sine), linear (triangle), square.
 */

#include "hfh_math.h"

namespace hfh {

class Rng {
 public:
  explicit Rng(uint32_t seed = 0x9E3779B9u) : s_(seed ? seed : 1u) {}
  uint32_t nextU32() { s_ ^= s_ << 13; s_ ^= s_ >> 17; s_ ^= s_ << 5; return s_; }
  float bipolar() { return (float)(int32_t)nextU32() * 4.6566129e-10f; }   // -1..1
  float unipolar() { return (float)(nextU32() >> 8) * 5.9604645e-8f; }     // 0..1
 private:
  uint32_t s_;
};

// Smooth random wander in -1..1. New random target every 1/rate seconds,
// eased with a cosine curve so there are no corners.
class Drift {
 public:
  explicit Drift(uint32_t seed = 1u) : rng_(seed * 2654435761u + 1u) { next_ = rng_.bipolar(); }
  void setRate(float hz) { inc_ = clampf(hz, 0.001f, 100.f) * kSampleTime; }
  float next() {
    phase_ += inc_;
    if (phase_ >= 1.f) { phase_ -= 1.f; prev_ = next_; next_ = rng_.bipolar(); }
    const float ease = 0.5f - 0.5f * fcos(phase_ * kPi);
    return prev_ + (next_ - prev_) * ease;
  }
  // Advance a block, return the value at the end (for block-rate modulation).
  float nextBlock(uint32_t frames) {
    phase_ += inc_ * (float)(frames - 1);
    while (phase_ >= 1.f) { phase_ -= 1.f; prev_ = next_; next_ = rng_.bipolar(); }
    return next();
  }
 private:
  Rng rng_;
  float phase_ = 0.f, inc_ = 0.f, prev_ = 0.f, next_ = 0.f;
};

class Lfo {
 public:
  enum Shape { kSine = 0, kTriangle, kSquare };
  void setRate(float hz) { inc_ = hz * kSampleTime; }
  void setPhase(float p) { phase_ = p - (float)floorToInt(p); }
  float next(Shape s = kSine) {
    const float p = phase_;
    phase_ += inc_;
    if (phase_ >= 1.f) phase_ -= 1.f;
    switch (s) {
      case kTriangle: return 1.f - 4.f * absf(p - 0.5f);
      case kSquare: return p < 0.5f ? 1.f : -1.f;
      default: return sinTurns(p);
    }
  }
  float phase() const { return phase_; }
 private:
  float phase_ = 0.f, inc_ = 0.f;
};

class Ramper {
 public:
  enum Mode { kOff = 0, kBounce, kRise, kFall };
  enum Shape { kSmooth = 0, kLinear, kSquare };

  void setMode(Mode m) { if (m != mode_) { mode_ = m; pos_ = 0.f; dir_ = 1.f; } }
  void setShape(Shape s) { shape_ = s; }
  // Time for one full sweep (bounce: one way), in seconds.
  void setTime(float sec) { inc_ = kSampleTime / maxf(sec, 0.01f); }
  void retrigger() { pos_ = 0.f; dir_ = 1.f; }

  // Returns 0..1: how far between "home" (knob) and "away" (ramp target).
  float next() {
    switch (mode_) {
      case kBounce:
        pos_ += inc_ * dir_;
        if (pos_ >= 1.f) { pos_ = 1.f; dir_ = -1.f; }
        if (pos_ <= 0.f) { pos_ = 0.f; dir_ = 1.f; }
        break;
      case kRise:
      case kFall:
        pos_ = minf(1.f, pos_ + inc_);
        break;
      default:
        return 0.f;
    }
    const float t = (mode_ == kFall) ? 1.f - pos_ : pos_;
    switch (shape_) {
      case kLinear: return t;
      case kSquare: return t < 0.5f ? 0.f : 1.f;
      default: return 0.5f - 0.5f * fcos(t * kPi);
    }
  }
  // Apply: blend between home and away values.
  float apply(float home, float away) { return lerpf(home, away, next()); }

 private:
  Mode mode_ = kOff;
  Shape shape_ = kSmooth;
  float pos_ = 0.f, dir_ = 1.f, inc_ = 0.f;
};

}  // namespace hfh
