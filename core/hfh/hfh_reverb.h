#pragma once
/*
 *  HFH core — Haze reverb
 *  A lush, slowly-moving stereo reverb for ambient use:
 *    input -> 2 diffusing allpasses per side -> 8-line feedback delay network
 *  - Householder feedback matrix (lossless mixing; decay comes only from the
 *    per-line gains, so DECAY is accurate in seconds)
 *  - per-line damping (high frequencies die first, like a real room)
 *  - slow random modulation of four line lengths, so tails shimmer instead
 *    of ringing metallic
 *  - SIZE scales every line; changes glide smoothly
 *  - FREEZE: lossless loop, input muted, holds the tail forever
 *
 *  Memory: kBufferFloats floats from SDRAM (~280 KB).
 */

#include "hfh_math.h"
#include "hfh_filter.h"
#include "hfh_delay.h"
#include "hfh_mod.h"
#include "hfh_smooth.h"

namespace hfh {

// Schroeder allpass over external memory.
class Allpass {
 public:
  void init(float *buf, uint32_t sizePow2, uint32_t delay) { line_.init(buf, sizePow2); d_ = delay; }
  float process(float x, float g) {
    const float vd = line_.at((int32_t)d_ - 1);
    const float v = x + g * vd;
    line_.write(undenormal(v));
    return vd - g * v;
  }
 private:
  DelayLine line_;
  uint32_t d_ = 1;
};

class Haze {
 public:
  static constexpr int kLines = 8;
  static constexpr uint32_t kLineSize = 8192;   // 170 ms max per line
  static constexpr uint32_t kDiffSize = 1024;
  static constexpr uint32_t kBufferFloats = kLines * kLineSize + 4 * kDiffSize;

  void init(float *buffer) {
    float *p = buffer;
    for (int i = 0; i < kLines; ++i) { line_[i].init(p, kLineSize); p += kLineSize; }
    static const float kDiffMs[4] = {4.77f, 3.59f, 5.31f, 3.97f};
    for (int i = 0; i < 4; ++i) { diff_[i].init(p, kDiffSize, (uint32_t)msToSamples(kDiffMs[i])); p += kDiffSize; }
    for (int i = 0; i < 4; ++i) { mod_[i] = Drift(101u + 17u * i); mod_[i].setRate(0.13f + 0.07f * i); }
    for (int i = 0; i < kLines; ++i) { len_[i].setTime(250.f); }
    inHp_[0].setCutoff(60.f);
    inHp_[1].setCutoff(60.f);
    setSize(0.6f);
    for (int i = 0; i < kLines; ++i) len_[i].snap(len_[i].target());
    setDecay(4.f);
    setDamping(6000.f);
    setModulation(0.5f);
    freeze_ = false;
  }

  void setSize(float s) {  // 0..1
    size_ = linMap(s, 0.55f, 1.45f);
    for (int i = 0; i < kLines; ++i) len_[i].setTarget(msToSamples(baseMs(i) * size_));
    updateGains();
  }
  void setDecay(float sec) { rt60_ = clampf(sec, 0.2f, 60.f); updateGains(); }
  void setDamping(float hz) { for (int i = 0; i < kLines; ++i) damp_[i].setCutoff(clampf(hz, 500.f, 20000.f)); dampHz_ = hz; }
  void setModulation(float m) { modDepth_ = clampf(m, 0.f, 1.f) * 12.f; }   // up to +/-12 samples
  void setFreeze(bool f) {
    if (f == freeze_) return;
    freeze_ = f;
    for (int i = 0; i < kLines; ++i) damp_[i].setCutoff(f ? 20000.f : dampHz_);
    updateGains();
  }

  void process(float inL, float inR, float &outL, float &outR) {
    float l = inHp_[0].process(inL), r = inHp_[1].process(inR);
    l = diff_[1].process(diff_[0].process(l, 0.62f), 0.62f);
    r = diff_[3].process(diff_[2].process(r, 0.62f), 0.62f);
    const float inGain = freeze_ ? 0.f : 0.5f;

    float v[kLines];
    float sum = 0.f;
    for (int i = 0; i < kLines; ++i) {
      float d = len_[i].next();
      if (i < 4) d += mod_[i].next() * modDepth_;
      float y = line_[i].readHermite(d);
      y = damp_[i].process(y) * gain_[i];
      v[i] = y;
      sum += y;
    }
    const float h = sum * (2.f / kLines);  // Householder reflection
    for (int i = 0; i < kLines; ++i) {
      const float in = (i & 1) ? r : l;
      line_[i].write(undenormal(v[i] - h + in * inGain));
    }
    outL = 0.35f * (v[0] - v[2] + v[4] - v[6]);
    outR = 0.35f * (v[1] - v[3] + v[5] - v[7]);
  }

 private:
  void updateGains() {
    for (int i = 0; i < kLines; ++i) {
      const float sec = baseMs(i) * size_ * 0.001f;
      gain_[i] = freeze_ ? 1.f : fexp2(-3.f * sec / rt60_ * 3.3219281f);  // 10^(-3 t / rt60)
    }
  }

  static float baseMs(int i) {
    static const float k[kLines] = {31.7f, 37.9f, 41.3f, 45.7f, 53.1f, 59.9f, 67.3f, 73.7f};
    return k[i];
  }

  DelayLine line_[kLines];
  Allpass diff_[4];
  OnePoleLP damp_[kLines];
  OnePoleHP inHp_[2];
  Smoother len_[kLines];
  Drift mod_[4];
  float gain_[kLines] = {};
  float size_ = 1.f, rt60_ = 4.f, dampHz_ = 6000.f, modDepth_ = 6.f;
  bool freeze_ = false;
};

}  // namespace hfh
