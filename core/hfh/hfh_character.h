#pragma once
/*
 *  HFH core — character ("the factory mods")
 *  The dreamy, worn-in layer that sits on top of an effect:
 *
 *  - Stability: wow + flutter + hiss + darkening in one control, from
 *    pristine to a warped old tape (in the spirit of Blooper's Stability and
 *    Generation Loss).
 *  - Clock: variable sample rate as a sound. Lower clock = darker, grainier,
 *    aliased. quantizeClock() snaps to musical steps (octaves/fifths), as
 *    MOOD's Clock does.
 *  - Tape saturation: soft, slightly asymmetric, with DC removal.
 *
 *  Wow/flutter need a short delay line; give Stability 2 x 2048 floats.
 */

#include "hfh_math.h"
#include "hfh_filter.h"
#include "hfh_delay.h"
#include "hfh_mod.h"

namespace hfh {

// Soft tape-style saturation. drive >= 1. Even harmonics from a small bias.
class TapeSat {
 public:
  // Unity gain for quiet signals; loud signals round off. drive 1 = gentle.
  void setDrive(float d) { k_ = maxf(d, 1.f) * 0.5f; inv_ = 1.f / k_; }
  float process(float x) {
    const float y = (ftanh(x * k_ + kBias) - kBiasOut) * inv_;
    return dc_.process(y);
  }
 private:
  static constexpr float kBias = 0.08f;
  static constexpr float kBiasOut = 0.0798486f;  // ftanh(0.08), the approximation used here
  float k_ = 0.5f, inv_ = 2.f;
  DCBlocker dc_;
};

// Snap a 0..1 control to musical clock ratios. steps per octave:
// 1 = octaves only, 2 = octaves + tritones, 12 = semitones.
// Returns a playback-rate ratio in (0, 1]: 1 = full 48 kHz.
inline float quantizeClock(float norm, int octaves, int stepsPerOctave) {
  const int total = octaves * stepsPerOctave;
  const int step = (int)(clampf(norm, 0.f, 1.f) * (float)total + 0.5f);
  return fexp2(-(float)(total - step) / (float)stepsPerOctave);
}

// Sample-rate reducer. rate 1 = untouched, 0.05 = ~2.4 kHz.
// smooth = true filters before and after for a dark, rounded lo-fi.
// smooth = false leaves the aliasing in for grit.
class Clock {
 public:
  void setRate(float ratio, bool smooth) {
    ratio_ = clampf(ratio, 0.02f, 1.f);
    smooth_ = smooth;
    const float nyq = 0.45f * kSampleRate * ratio_;
    pre_.set(nyq, 0.7071f);
    post_.set(nyq, 0.7071f);
  }
  float process(float x) {
    if (ratio_ >= 0.999f) return x;
    if (smooth_) x = pre_.process(x, Svf::kLowPass);
    phase_ += ratio_;
    if (phase_ >= 1.f) { phase_ -= 1.f; held_ = x; }
    return smooth_ ? post_.process(held_, Svf::kLowPass) : held_;
  }
 private:
  float ratio_ = 1.f, phase_ = 0.f, held_ = 0.f;
  bool smooth_ = true;
  Svf24 pre_, post_;
};

// Stability: 0 = pristine, 1 = warped tape. Stereo, with independent
// flutter per side and shared wow (like a real transport).
class Stability {
 public:
  static constexpr uint32_t kLineSize = 2048;               // 42 ms per side
  static constexpr uint32_t kBufferFloats = 2 * kLineSize;

  void init(float *buffer) {
    line_[0].init(buffer, kLineSize);
    line_[1].init(buffer + kLineSize, kLineSize);
    wowDrift_ = Drift(11u);
    flutDrift_[0] = Drift(23u);
    flutDrift_[1] = Drift(37u);
    wowDrift_.setRate(0.35f);
    wowLfo_.setRate(0.55f);
    flutLfo_[0].setRate(7.3f);
    flutLfo_[1].setRate(8.1f);
    flutDrift_[0].setRate(3.f);
    flutDrift_[1].setRate(3.4f);
    hissLp_[0].setCutoff(6000.f);
    hissLp_[1].setCutoff(6000.f);
    amount_.setTime(80.f);
    setAmount(0.f);
  }

  void setAmount(float a) {
    a = clampf(a, 0.f, 1.f);
    amount_.setTarget(a);
    // Darkening: 18 kHz clean -> 3.5 kHz at full instability.
    const float hz = expMap(1.f - a, 3500.f, 18000.f);
    tone_[0].setCutoff(hz);
    tone_[1].setCutoff(hz);
    hissGain_ = a * a * dbToGain(-54.f);
  }

  void process(float &l, float &r) {
    const float a = amount_.next();
    // Wow: slow, deep (up to ~6 ms). Flutter: fast, shallow (up to ~0.4 ms).
    const float wow = (0.6f * wowLfo_.next() + 0.4f * wowDrift_.next()) * a * a * msToSamples(6.f);
    const float base = msToSamples(8.f);
    float *ch[2] = {&l, &r};
    for (int i = 0; i < 2; ++i) {
      const float flut = (0.7f * flutLfo_[i].next() + 0.3f * flutDrift_[i].next()) * a * msToSamples(0.4f);
      line_[i].write(*ch[i]);
      float y = (a > 0.0005f) ? line_[i].readHermite(base + wow + flut) : *ch[i];
      y = tone_[i].process(y);
      y += hissGain_ * hissLp_[i].process(rng_.bipolar());
      *ch[i] = y;
    }
  }

 private:
  DelayLine line_[2];
  Lfo wowLfo_, flutLfo_[2];
  Drift wowDrift_, flutDrift_[2];
  OnePoleLP tone_[2];
  OnePoleLP hissLp_[2];
  Smoother amount_;
  Rng rng_{0xA5A5A5A5u};
  float hissGain_ = 0.f;
};

}  // namespace hfh
