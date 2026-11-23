#pragma once
/*
 *  HFH core — filters
 *  - OnePoleLP / OnePoleHP: cheap tone shaping
 *  - DCBlocker: for feedback loops and asymmetric saturation
 *  - Svf: zero-delay-feedback state-variable filter (Simper/Zavalishin "TPT").
 *    Stays stable and smooth under fast cutoff modulation, which matters on a
 *    touch pad. All four outputs come from one pass.
 */

#include "hfh_math.h"

namespace hfh {

class OnePoleLP {
 public:
  void setCutoff(float hz) { c_ = cutoffCoef(clampf(hz, 1.f, 20000.f)); }
  float process(float x) { z_ += c_ * (x - z_); return z_; }
  void reset(float v = 0.f) { z_ = v; }
 private:
  float c_ = 1.f, z_ = 0.f;
};

class OnePoleHP {
 public:
  void setCutoff(float hz) { lp_.setCutoff(hz); }
  float process(float x) { return x - lp_.process(x); }
  void reset() { lp_.reset(); }
 private:
  OnePoleLP lp_;
};

class DCBlocker {
 public:
  float process(float x) {
    const float y = x - x1_ + 0.9995f * y1_;
    x1_ = x;
    y1_ = undenormal(y);
    return y1_;
  }
  void reset() { x1_ = y1_ = 0.f; }
 private:
  float x1_ = 0.f, y1_ = 0.f;
};

class Svf {
 public:
  enum Mode { kLowPass = 0, kHighPass, kBandPass, kNotch, kPeak };

  // q: 0.5 (soft) .. ~20 (screaming). 0.707 = Butterworth.
  void set(float hz, float q) {
    hz = clampf(hz, 10.f, 0.45f * kSampleRate);
    q = maxf(q, 0.5f);
    const float g = ftan(kPi * hz * kSampleTime);
    k_ = 1.f / q;
    a1_ = 1.f / (1.f + g * (g + k_));
    a2_ = g * a1_;
    a3_ = g * a2_;
  }

  // Process one sample; all outputs available afterwards.
  void tick(float x) {
    const float v3 = x - ic2_;
    const float v1 = a1_ * ic1_ + a2_ * v3;
    const float v2 = ic2_ + a2_ * ic1_ + a3_ * v3;
    ic1_ = undenormal(2.f * v1 - ic1_);
    ic2_ = undenormal(2.f * v2 - ic2_);
    lp = v2;
    bp = v1;
    hp = x - k_ * v1 - v2;
  }

  float process(float x, Mode m) {
    tick(x);
    switch (m) {
      case kHighPass: return hp;
      case kBandPass: return bp;
      case kNotch: return lp + hp;
      case kPeak: return lp - hp;
      default: return lp;
    }
  }

  void reset() { ic1_ = ic2_ = 0.f; lp = bp = hp = 0.f; }

  float lp = 0.f, bp = 0.f, hp = 0.f;

 private:
  float k_ = 1.414f, a1_ = 0.f, a2_ = 0.f, a3_ = 0.f;
  float ic1_ = 0.f, ic2_ = 0.f;
};

// Two cascaded SVFs = 24 dB/oct, the "serious" filter slope.
class Svf24 {
 public:
  // q = 0.707 gives a 4-pole Butterworth; higher q adds resonance.
  void set(float hz, float q) { a_.set(hz, 0.5412f); b_.set(hz, 1.3066f * q * 1.41421f); }
  float process(float x, Svf::Mode m) { return b_.process(a_.process(x, m), m); }
  void reset() { a_.reset(); b_.reset(); }
 private:
  Svf a_, b_;
};

}  // namespace hfh
