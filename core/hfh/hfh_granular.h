#pragma once
/*
 *  HFH core — grain engine
 *  Up to kMaxGrains playheads reading from a recording buffer (a DelayLine
 *  the effect writes into). Each grain has its own start point, speed
 *  (negative = reverse), length, pan and level, and is shaped by a Tukey
 *  window (taper 1 = Hann, smoothest; lower = fuller, more present grains).
 *
 *  The engine keeps fast grains from catching up with the record head, so
 *  pitched-up grains never read audio that hasn't been written yet.
 */

#include "hfh_math.h"
#include "hfh_delay.h"
#include "hfh_grain.h"
#include "hfh_mod.h"

namespace hfh {

struct GrainSpec {
  float offset;       // how far back in time to start, samples
  float rate;         // playback speed: 2 = octave up, -1 = reverse
  float length;       // samples
  float pan;          // 0 = left, 0.5 = centre, 1 = right
  float gain;
};

class GrainCloud {
 public:
  static constexpr int kMaxGrains = 16;

  void init(const DelayLine *source) {
    src_ = source;
    for (int i = 0; i < kMaxGrains; ++i) g_[i].active = false;
    taper_ = 1.f;
  }
  void setTaper(float t) { taper_ = clampf(t, 0.05f, 1.f); }

  int activeCount() const {
    int n = 0;
    for (int i = 0; i < kMaxGrains; ++i) n += g_[i].active ? 1 : 0;
    return n;
  }

  // Start a grain. Returns false if every voice is busy.
  bool spawn(const GrainSpec &s) {
    for (int i = 0; i < kMaxGrains; ++i) {
      Grain &g = g_[i];
      if (g.active) continue;
      const float len = maxf(s.length, 64.f);
      // Fast forward grains would overtake the record head; start them far enough back.
      const float minBack = (s.rate > 1.f ? (s.rate - 1.f) * len : 0.f) + 16.f;
      const float back = clampf(maxf(s.offset, minBack), 16.f, (float)src_->size() - 8.f);
      g.pos = (float)src_->writeIndex() - back;
      if (g.pos < 0.f) g.pos += (float)src_->size();
      g.rate = s.rate;
      g.inc = 1.f / len;
      g.age = 0.f;
      const float p = clampf(s.pan, 0.f, 1.f);
      g.gl = fsqrt(1.f - p) * s.gain;
      g.gr = fsqrt(p) * s.gain;
      g.active = true;
      return true;
    }
    return false;
  }

  void process(float &outL, float &outR) {
    float l = 0.f, r = 0.f;
    const float size = (float)src_->size();
    for (int i = 0; i < kMaxGrains; ++i) {
      Grain &g = g_[i];
      if (!g.active) continue;
      const float w = windowTukey(g.age, taper_);
      const float x = src_->readAbsHermite(g.pos) * w;
      l += x * g.gl;
      r += x * g.gr;
      g.pos += g.rate;
      if (g.pos >= size) g.pos -= size;
      else if (g.pos < 0.f) g.pos += size;
      g.age += g.inc;
      if (g.age >= 1.f) g.active = false;
    }
    outL = l;
    outR = r;
  }

 private:
  struct Grain {
    float pos, rate, inc, age, gl, gr;
    bool active;
  };
  const DelayLine *src_ = nullptr;
  Grain g_[kMaxGrains];
  float taper_ = 1.f;
};

// Random grain timing: average `perSecond` grains, jittered so they never
// fall into a mechanical rhythm.
class GrainScheduler {
 public:
  explicit GrainScheduler(uint32_t seed = 77u) : rng_(seed) {}
  void setDensity(float perSecond) { mean_ = kSampleRate / maxf(perSecond, 0.05f); }
  // Call once per sample; true when a grain should start.
  bool tick() {
    if (--count_ > 0) return false;
    count_ = (int32_t)(mean_ * (0.35f + 1.3f * rng_.unipolar())) + 1;
    return true;
  }
  Rng &rng() { return rng_; }
 private:
  Rng rng_;
  float mean_ = 4800.f;
  int32_t count_ = 1;
};

}  // namespace hfh
