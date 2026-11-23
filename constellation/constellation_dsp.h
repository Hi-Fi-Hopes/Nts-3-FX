#pragma once
/*
 *  Constellation — tiny, pitched-up grains of what you play, scattered
 *  across the stereo field like stars, then softened in a short reverb.
 *  Hi-Fi Hopes
 *
 *  The input is recorded continuously. Grains start at random moments
 *  (DENSITY), reach back a random distance into the recording (SCATTER),
 *  play at a chord-tone pitch above it (HEIGHT), land at a random pan
 *  position and fade in and out smoothly. Some play backwards (REVERSE).
 *  The dry sound always passes untouched; the sparkles are added on top.
 */

#include "hfh.h"

namespace constellation {

using namespace hfh;

constexpr int kHeights = 5;
// Pitch sets (semitones). Each grain picks one at random.
static const int8_t kHeightSets[kHeights][3] = {
  {0, 12, 12},     // unison + octave
  {12, 12, 12},    // octave
  {12, 19, 12},    // octave + fifth above
  {12, 19, 24},    // octave, fifth, two octaves
  {19, 24, 31},    // high: fifth, two octaves, two octaves + fifth
};

class Constellation {
 public:
  static constexpr uint32_t kRecSize = 65536;   // 1.36 s of history
  static constexpr uint32_t kBufferFloats = kRecSize + Haze::kBufferFloats;

  void init(float *buffer) {
    rec_.init(buffer, kRecSize);
    haze_.init(buffer + kRecSize);
    haze_.setDecay(3.5f);
    haze_.setSize(0.5f);
    haze_.setDamping(9000.f);
    haze_.setModulation(0.7f);
    cloud_.init(&rec_);
    cloud_.setTaper(0.7f);
    inHp_.setCutoff(150.f);
    aa_[0].setCutoff(5000.f);
    aa_[1].setCutoff(5000.f);
    level_.setTime(40.f);
    space_.setTime(40.f);
    setDensity(8.f);
    setHeight(0.4f);
    setLevel(0.6f);
    setSize(90.f);
    setScatter(600.f);
    setReverse(0.15f);
    setSpace(0.5f);
    setTone(9000.f);
  }

  // ---- parameters ----
  void setDensity(float perSec) { sched_.setDensity(clampf(perSec, 0.5f, 40.f)); }
  void setHeight(float h) { int z = (int)(clampf(h, 0.f, 0.999f) * kHeights); height_ = z; }
  void setHeightZone(int z) { height_ = z < 0 ? 0 : (z > kHeights - 1 ? kHeights - 1 : z); }
  void setLevel(float l) { level_.setTarget(clampf(l, 0.f, 1.f)); }
  void setSize(float ms) { sizeMs_ = clampf(ms, 20.f, 300.f); }
  void setScatter(float ms) { scatterMs_ = clampf(ms, 30.f, 1200.f); }
  void setReverse(float p) { reverse_ = clampf(p, 0.f, 1.f); }
  void setSpace(float s) { space_.setTarget(clampf(s, 0.f, 1.f)); }
  void setTone(float hz) { toneLp_[0].setCutoff(hz); toneLp_[1].setCutoff(hz); }
  static const char *heightName(int z) {
    static const char *k[kHeights] = {"Unison Oct", "Octave", "Oct 5th", "Oct 5th 2Oct", "High"};
    return k[z < 0 ? 0 : (z > kHeights - 1 ? kHeights - 1 : z)];
  }

  void process(const float *in, float *out, uint32_t frames) {
    for (uint32_t i = 0; i < frames; ++i) {
      const float inL = in[2 * i], inR = in[2 * i + 1];
      // Record band-limited (~5 kHz) so grains pitched up 2-3 octaves don't alias.
      float rx = inHp_.process(0.5f * (inL + inR));
      rx = aa_[1].process(aa_[0].process(rx));
      rec_.write(rx);

      if (sched_.tick()) spawnGrain();
      float gl, gr;
      cloud_.process(gl, gr);
      gl = toneLp_[0].process(gl);
      gr = toneLp_[1].process(gr);

      float rl, rr;
      haze_.process(gl, gr, rl, rr);
      const float sp = space_.next();
      const float sl = (1.f - sp) * gl + sp * rl * 2.f;
      const float sr = (1.f - sp) * gr + sp * rr * 2.f;

      const float lvl = level_.next() * 1.5f;
      out[2 * i] = softLimit(inL + lvl * sl);
      out[2 * i + 1] = softLimit(inR + lvl * sr);
    }
  }

 private:
  void spawnGrain() {
    Rng &r = sched_.rng();
    const int8_t st = kHeightSets[height_][r.nextU32() % 3];
    float rate = fexp2((float)st * (1.f / 12.f));
    if (r.unipolar() < reverse_) rate = -rate;
    // Shorter grains for higher pitches keep the sparkle tight.
    const float len = msToSamples(sizeMs_ * (0.6f + 0.8f * r.unipolar())) / maxf(1.f, absf(rate) * 0.7f);
    GrainSpec g;
    g.offset = msToSamples(30.f + scatterMs_ * r.unipolar());
    g.rate = rate;
    g.length = len;
    g.pan = 0.5f + 0.48f * r.bipolar();
    g.gain = 0.45f + 0.55f * r.unipolar();
    cloud_.spawn(g);
  }

  DelayLine rec_;
  GrainCloud cloud_;
  GrainScheduler sched_{1234u};
  Haze haze_;
  OnePoleHP inHp_;
  OnePoleLP toneLp_[2], aa_[2];
  Smoother level_, space_;
  float sizeMs_ = 90.f, scatterMs_ = 600.f, reverse_ = 0.15f;
  int height_ = 2;
};

}  // namespace constellation
