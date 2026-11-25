#pragma once
/*
 *  Supermassive — a huge, slowly moving reverb to dance around in space and time.
 *  Hi-Fi Hopes
 *
 *    in -> HP -> anti-alias -> (decimate to 24 kHz) -> pre-delay -> 4 diffusers/side
 *       -> 8-line FDN (Hadamard matrix, in-loop allpass, damping, low cut, drift)
 *       -> (interpolate back to 48 kHz) -> image filter -> Stability -> mix
 *
 *  The tail runs at half rate (24 kHz). That doubles how much delay fits in
 *  memory and halves the processor load, at the cost of air above ~10 kHz,
 *  which suits a dark, dreamy space anyway (old Lexicons did the same).
 *
 *  Built-in behaviour:
 *    - SIZE glides (WARP sets how fast); fast glides bend the pitch of the
 *      whole tail like a tape machine, slow glides just breathe.
 *    - Pre-delay grows with SIZE (0 - 120 ms).
 *    - Redshift: longer decays darken the tail, so it sinks into the distance.
 *    - Gravity: longer decays slow the modulation down, like time dilating.
 *    - Freeze (DECAY at max): lossless loop, input muted, holds forever.
 *    - SHIMMER: octave-up pitch shifter in the feedback, level-controlled so it
 *      blooms but never runs away.
 *
 *  Memory: kBufferFloats floats (~440 KB) of SDRAM.
 */

#include "hfh.h"

namespace sm {

using namespace hfh;

// Tail-rate helpers. hfh's coefficient helpers assume 48 kHz, so anything
// that runs inside the 24 kHz tail is given doubled frequencies / halved times.
constexpr float kTailRate = 24000.f;
inline float tailSamples(float ms) { return ms * (kTailRate * 0.001f); }

// Allpass with a smoothly variable (fractional) length, for diffusion that
// scales with room size.
class VarAllpass {
 public:
  void init(float *buf, uint32_t sizePow2) { line_.init(buf, sizePow2); }
  float process(float x, float delay, float g) {
    const float vd = line_.readLinear(delay - 1.f);
    const float v = x + g * vd;
    line_.write(undenormal(v));
    return vd - g * v;
  }
 private:
  DelayLine line_;
};

// Fast Walsh-Hadamard transform of 8 values, normalised (lossless).
inline void hadamard8(float *x) {
  for (int h = 1; h < 8; h <<= 1) {
    for (int i = 0; i < 8; i += h << 1) {
      for (int j = i; j < i + h; ++j) {
        const float a = x[j], b = x[j + h];
        x[j] = a + b;
        x[j + h] = a - b;
      }
    }
  }
  for (int i = 0; i < 8; ++i) x[i] *= 0.35355339f;  // 1/sqrt(8)
}

// Transparent below 1.0, then rounds off toward 1.5. Keeps the loop bounded
// in freeze and with heavy shimmer without colouring normal levels.
inline float loopSafety(float x) {
  const float a = absf(x);
  if (a <= 1.f) return x;
  const float y = 1.f + 0.5f * ftanh(2.f * (a - 1.f));
  return x < 0.f ? -y : y;
}

// FDN line lengths at size 1.0 (scaled 0.4x - 2.8x by SIZE), and the
// in-loop allpass that thickens each line's echoes.
static const float kLineMs[8] = {41.3f, 49.9f, 58.7f, 68.9f, 79.3f, 91.1f, 103.7f, 117.1f};
static const float kApMs[8] = {2.3f, 2.9f, 3.7f, 4.3f, 5.1f, 5.9f, 6.7f, 7.9f};

class Supermassive {
 public:
  static constexpr int kLines = 8;
  static constexpr uint32_t kLineSize = 8192;    // 341 ms per line at 24 kHz
  static constexpr uint32_t kDiffSize = 2048;
  static constexpr uint32_t kLoopApSize = 1024;
  static constexpr uint32_t kPreSize = 4096;     // 170 ms at 24 kHz
  static constexpr uint32_t kBufferFloats = kLines * kLineSize + 8 * kDiffSize + kLines * kLoopApSize +
                                            2 * kPreSize + PitchShifter::kBufferFloats + Stability::kBufferFloats;

  static constexpr float kScaleMin = 0.4f, kScaleMax = 2.8f;

  void init(float *buffer) {
    float *p = buffer;
    for (int i = 0; i < kLines; ++i) { line_[i].init(p, kLineSize); p += kLineSize; }
    for (int i = 0; i < 8; ++i) { diff_[i].init(p, kDiffSize); p += kDiffSize; }
    for (int i = 0; i < kLines; ++i) { loopAp_[i].init(p, kLoopApSize); p += kLoopApSize; }
    pre_[0].init(p, kPreSize); p += kPreSize;
    pre_[1].init(p, kPreSize); p += kPreSize;
    shim_.init(p); p += PitchShifter::kBufferFloats;
    stab_.init(p);

    shim_.setWindowMs(0.5f * 70.f);   // 70 ms window; times are halved at tail rate
    shim_.setSemitones(12.f);
    shimHp_.setCutoff(2.f * 300.f);
    shimLp_.setCutoff(2.f * 6000.f);

    for (int i = 0; i < 2; ++i) {
      inHp_[i].setCutoff(30.f);
      aa_[i].set(9500.f, 0.7071f);
      img_[i].set(10000.f, 0.7071f);
    }
    for (int i = 0; i < kLines; ++i) {
      mod_[i] = Drift(311u + 37u * (uint32_t)i);
      lowCut_[i].setCutoff(2.f * 4.f);
    }

    mix_.setTime(40.f);
    shimAmt_.setTime(0.5f * 80.f);
    inGain_.setTime(0.5f * 30.f);
    modDepth_.setTime(0.5f * 100.f);

    setWarp(600.f);
    setSize(0.6f);
    scale_.snap(scale_.target());
    setDecay(12.f);
    setTone(5000.f);
    setModulation(0.5f);
    setShimmer(0.f);
    setMix(0.5f);
    setFreeze(false);
    inGain_.snap(1.f);
    phase_ = 0;
    prevL_ = prevR_ = curL_ = curR_ = 0.f;
    shimFb_ = 0.f;
    env_ = 0.f;
  }

  // ---- parameters ----
  void setSize(float s) {   // 0..1, exponential: chamber -> supermassive
    scale_.setTarget(expMap(clampf(s, 0.f, 1.f), kScaleMin, kScaleMax));
  }
  void setWarp(float ms) { scale_.setTime(0.5f * clampf(ms, 20.f, 5000.f)); }   // tail runs at half rate
  void setDecay(float sec) { rt60_ = clampf(sec, 0.5f, 120.f); updateDamping(); updateGravity(); }
  void setFreeze(bool f) {
    freeze_ = f;
    inGain_.setTarget(f ? 0.f : 1.f);
    updateGravity();
  }
  void setTone(float hz) { toneHz_ = clampf(hz, 800.f, 10000.f); updateDamping(); }
  void setModulation(float m) { modDepth_.setTarget(clampf(m, 0.f, 1.f) * 22.f); }  // up to ~0.9 ms
  void setShimmer(float s) { shimTarget_ = clampf(s, 0.f, 1.f); }
  void setMix(float m) { mix_.setTarget(clampf(m, 0.f, 1.f)); }
  void setStability(float s) { stab_.setAmount(s); }

  bool frozen() const { return freeze_; }

  void process(const float *in, float *out, uint32_t frames) {
    // Per-block: feedback gains follow the gliding size.
    updateGains(scale_.value());
    shimAmt_.setTarget(freeze_ ? 0.f : shimTarget_);

    for (uint32_t i = 0; i < frames; ++i) {
      const float inL = in[2 * i], inR = in[2 * i + 1];
      float l = aa_[0].process(inHp_[0].process(inL), Svf::kLowPass);
      float r = aa_[1].process(inHp_[1].process(inR), Svf::kLowPass);

      float wl, wr;
      phase_ ^= 1;
      if (phase_) {
        prevL_ = curL_; prevR_ = curR_;
        tick(l, r, curL_, curR_);
        wl = 0.5f * (prevL_ + curL_);
        wr = 0.5f * (prevR_ + curR_);
      } else {
        wl = curL_;
        wr = curR_;
      }
      wl = img_[0].process(wl, Svf::kLowPass);
      wr = img_[1].process(wr, Svf::kLowPass);
      stab_.process(wl, wr);

      float dry, wet;
      equalPowerGains(mix_.next(), dry, wet);
      out[2 * i] = softLimit(dry * inL + wet * wl);
      out[2 * i + 1] = softLimit(dry * inR + wet * wr);
    }
  }

 private:
  // One step of the 24 kHz tail.
  void tick(float l, float r, float &outL, float &outR) {
    const float sc = scale_.next();
    const float ig = inGain_.next();

    // Pre-delay grows with size: 0 ms at the smallest, 120 ms at the largest.
    pre_[0].write(l);
    pre_[1].write(r);
    const float preMs = 120.f * clampf((sc - kScaleMin) / (kScaleMax - kScaleMin), 0.f, 1.f);
    float a = pre_[0].readLinear(tailSamples(preMs));
    float b = pre_[1].readLinear(tailSamples(preMs));

    // Shimmer: the tail's own sound an octave up, fed back in. Its gain
    // backs off as the tail gets loud, so it blooms but can't run away.
    const float sAmt = shimAmt_.next();
    const float shim = shimFb_ * sAmt * 0.45f / (1.f + 6.f * env_);
    a += shim;
    b += shim;

    // Input diffusion, scaled with size so big rooms smear more.
    static const float kDiffMs[8] = {3.1f, 4.7f, 7.3f, 11.3f, 3.7f, 5.3f, 8.1f, 12.1f};
    const float ds = 0.5f + 0.5f * sc;
    for (int k = 0; k < 4; ++k) a = diff_[k].process(a, tailSamples(kDiffMs[k] * ds), 0.68f);
    for (int k = 4; k < 8; ++k) b = diff_[k].process(b, tailSamples(kDiffMs[k] * ds), 0.68f);
    a *= ig;
    b *= ig;

    // Feedback delay network.
    const float md = modDepth_.next();
    float v[kLines];
    for (int k = 0; k < kLines; ++k) {
      const float d = tailSamples(kLineMs[k] * sc) + md * mod_[k].next() + 2.f;
      float y = line_[k].readHermite(d);
      y = loopAp_[k].process(y, tailSamples(kApMs[k] * sc), 0.5f);
      if (!freeze_) {
        // High shelf: below TONE the tail decays in exactly DECAY seconds;
        // above it, faster. Then a DC/sub blocker.
        const float lo = damp_[k].process(y);
        y = lo + hfMul_[k] * (y - lo);
        y = lowCut_[k].process(y);
      }
      v[k] = y * gain_[k];
    }
    outL = 0.42f * (v[0] - v[2] + v[4] - v[6]);
    outR = 0.42f * (v[1] - v[3] + v[5] - v[7]);

    hadamard8(v);
    for (int k = 0; k < kLines; ++k) {
      const float inj = (k & 1) ? b : a;
      line_[k].write(undenormal(loopSafety(v[k] + 0.6f * inj)));
    }

    // Shimmer source + level follower for its gain control.
    const float mono = 0.5f * (outL + outR);
    env_ += 0.0005f * (absf(mono) - env_);
    float s = shim_.process(mono);
    s = shimLp_.process(shimHp_.process(s));
    shimFb_ = softclip(s * 1.2f);
  }

  void updateGains(float sc) {
    for (int k = 0; k < kLines; ++k) {
      if (freeze_) { gain_[k] = 1.f; hfMul_[k] = 1.f; continue; }
      const float sec = (kLineMs[k] + kApMs[k]) * sc * 0.001f;
      gain_[k] = fexp2(-3.f * 3.3219281f * sec / rt60_);   // 10^(-3 t / rt60)
      // Extra loss above TONE so highs ring for rt60 * hfRatio instead.
      hfMul_[k] = fexp2(-3.f * 3.3219281f * sec * (1.f / (rt60_ * hfRatio_) - 1.f / rt60_));
    }
  }

  // Redshift: past 4 s the tail darkens: the shelf corner drops up to 1.3
  // octaves and the highs die relatively sooner (45% -> 20% of DECAY) by 100 s.
  void updateDamping() {
    const float n = clampf(flog2(rt60_ / 4.f) / flog2(25.f), 0.f, 1.f);
    const float hz = toneHz_ * fexp2(-1.3f * n);
    hfRatio_ = lerpf(0.45f, 0.2f, n);
    for (int k = 0; k < kLines; ++k) damp_[k].setCutoff(2.f * hz);
  }

  // Gravity: the longer the decay, the slower the tail moves.
  void updateGravity() {
    const float n = freeze_ ? 1.f : clampf(flog2(rt60_ / 2.f) / flog2(50.f), 0.f, 1.f);
    const float slow = 1.f / (1.f + 2.5f * n);
    for (int k = 0; k < kLines; ++k) mod_[k].setRate(2.f * (0.09f + 0.045f * (float)k) * slow);
  }

  DelayLine line_[kLines];
  VarAllpass diff_[8];
  VarAllpass loopAp_[kLines];
  DelayLine pre_[2];
  OnePoleLP damp_[kLines];
  OnePoleHP lowCut_[kLines];
  OnePoleHP inHp_[2];
  Svf24 aa_[2], img_[2];
  Drift mod_[kLines];
  PitchShifter shim_;
  OnePoleHP shimHp_;
  OnePoleLP shimLp_;
  Stability stab_;
  Smoother scale_, mix_, shimAmt_, inGain_, modDepth_;
  float gain_[kLines] = {}, hfMul_[kLines] = {};
  float rt60_ = 12.f, toneHz_ = 5000.f, shimTarget_ = 0.f, hfRatio_ = 0.45f;
  float prevL_ = 0.f, prevR_ = 0.f, curL_ = 0.f, curR_ = 0.f;
  float shimFb_ = 0.f, env_ = 0.f;
  int phase_ = 0;
  bool freeze_ = false;
};

}  // namespace sm
