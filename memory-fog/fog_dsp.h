#pragma once
/*
 *  Memory Fog — a delay where every repeat is older than the last.
 *  Hi-Fi Hopes
 *
 *  Each pass around the feedback loop goes through the same "aging" chain, so
 *  the damage compounds: the first echo is nearly clean, the fourth is a
 *  dark, wobbly, hissy ghost, and the tail dissolves into fog.
 *
 *    read (with wow/flutter) -> low-pass (darkens) -> high-pass (thins)
 *      -> clock (lower sample rate, grit) -> tape saturation -> + hiss
 *      -> feedback (with ping-pong spread) -> write
 *
 *  Moving TIME glides the delay like a tape machine changing speed.
 */

#include "hfh.h"

namespace fog {

using namespace hfh;

class MemoryFog {
 public:
  static constexpr uint32_t kLineSize = 65536;   // 1.36 s per side
  static constexpr uint32_t kBufferFloats = 2 * kLineSize;

  void init(float *buffer) {
    line_[0].init(buffer, kLineSize);
    line_[1].init(buffer + kLineSize, kLineSize);
    time_.setTime(350.f);              // tape-like glide when TIME moves
    time_.snap(msToSamples(450.f));
    mix_.setTime(30.f);
    fb_.setTime(30.f);
    wowLfo_.setRate(0.6f);
    flutLfo_.setRate(6.7f);
    wowDrift_ = Drift(5u);
    wowDrift_.setRate(0.4f);
    hissLp_[0].setCutoff(7000.f);
    hissLp_[1].setCutoff(7000.f);
    setTime(450.f);
    setAge(0.4f);
    setFeedback(0.5f);
    setMix(0.4f);
    setSpread(0.5f);
    setWarble(0.35f);
    setHiss(0.3f);
    setTone(0.5f);
  }

  // ---- parameters ----
  void setTime(float ms) { time_.setTarget(msToSamples(clampf(ms, 20.f, 1300.f))); }
  void setFeedback(float f) { fb_.setTarget(clampf(f, 0.f, 1.f) * 1.02f); }   // >1 at the top: self-sustaining, held by saturation
  void setMix(float m) { mix_.setTarget(clampf(m, 0.f, 1.f)); }
  void setSpread(float s) { spread_ = clampf(s, 0.f, 1.f); }
  void setWarble(float w) { warble_ = clampf(w, 0.f, 1.f); }
  void setHiss(float h) { hiss_ = clampf(h, 0.f, 1.f) * dbToGain(-46.f); }
  void setTone(float t) { tone_ = clampf(t, 0.f, 1.f); ageDirty_ = true; }
  void setAge(float a) { age_ = clampf(a, 0.f, 1.f); ageDirty_ = true; }

  void process(const float *in, float *out, uint32_t frames) {
    if (ageDirty_) updateAge();
    for (uint32_t i = 0; i < frames; ++i) {
      const float inL = in[2 * i], inR = in[2 * i + 1];

      // Wow (slow, deep) + flutter (fast, shallow), scaled by WARBLE.
      const float w = warble_ * warble_;
      const float mod = w * (msToSamples(3.5f) * (0.6f * wowLfo_.next() + 0.4f * wowDrift_.next())
                             + msToSamples(0.25f) * flutLfo_.next());
      const float d = clampf(time_.next() + mod, 4.f, (float)kLineSize - 8.f);

      float r[2];
      for (int c = 0; c < 2; ++c) {
        float x = line_[c].readHermite(d);
        x = lp_[c].process(x);
        x = hp_[c].process(x);
        x = clock_[c].process(x);
        x = sat_[c].process(x);
        x += hiss_ * hissLp_[c].process(rng_.bipolar());
        r[c] = x;
      }

      // Feedback with ping-pong spread: 0 = each side to itself, 1 = fully crossed.
      const float fb = fb_.next();
      const float fl = (1.f - spread_) * r[0] + spread_ * r[1];
      const float fr = (1.f - spread_) * r[1] + spread_ * r[0];
      line_[0].write(undenormal(softclip(inL + fb * fl)));
      line_[1].write(undenormal(softclip(inR + fb * fr)));

      float dry, wet;
      equalPowerGains(mix_.next(), dry, wet);
      out[2 * i] = softLimit(dry * inL + wet * r[0]);
      out[2 * i + 1] = softLimit(dry * inR + wet * r[1]);
    }
  }

 private:
  // AGE sets how much damage each pass does.
  void updateAge() {
    ageDirty_ = false;
    const float a = age_;
    const float bright = expMap(tone_, 0.5f, 2.f);                  // TONE: darker/brighter start
    const float lpHz = clampf(expMap(1.f - a * 0.85f, 1100.f, 16000.f) * bright, 600.f, 18000.f);
    const float hpHz = linMap(a, 25.f, 320.f);
    const float clock = 1.f - 0.7f * a * a;                           // down to ~0.3 x 48 kHz per pass
    for (int c = 0; c < 2; ++c) {
      lp_[c].setCutoff(lpHz);
      hp_[c].setCutoff(hpHz);
      clock_[c].setRate(clock, true);
      sat_[c].setDrive(1.f + 5.f * a);
    }
  }

  DelayLine line_[2];
  OnePoleLP lp_[2], hissLp_[2];
  OnePoleHP hp_[2];
  Clock clock_[2];
  TapeSat sat_[2];
  Smoother time_, mix_, fb_;
  Lfo wowLfo_, flutLfo_;
  Drift wowDrift_;
  Rng rng_{0xF06F06u};
  float spread_ = 0.5f, warble_ = 0.35f, hiss_ = 0.f, tone_ = 0.5f, age_ = 0.4f;
  volatile bool ageDirty_ = true;
};

}  // namespace fog
