#pragma once
/*
 *  Bloom — removes the attack from every note and swells it in, then feeds it
 *  into a soft, slowly-moving reverb. Bells become bowed glass; plucks become
 *  pads.
 *  Hi-Fi Hopes
 *
 *  Sustain: before the swell, an optional leveller lifts decaying notes back
 *  up (up to +24 dB), so a bell that would fade away instead holds like a
 *  bowed note while the swell rises. That is the "bowed glass" sound.
 *
 *  Look-ahead: the audio runs 8 ms behind the detector, so the gain has
 *  already dipped to silence when a new attack arrives. Without this the
 *  first few ms of every note would tick through.
 *
 *  Onset detection: a fast and a slow envelope follower. A new note is either
 *  (a) sound arriving after quiet, or (b) a sudden jump of the fast envelope
 *  above the slow one. Either restarts the swell: the gain dips to silence in
 *  a few ms (no click), then rises over SWELL time along the chosen curve.
 */

#include "hfh.h"

namespace bloom {

using namespace hfh;

class Bloom {
 public:
  static constexpr uint32_t kLookLen = 512;
  static constexpr uint32_t kBufferFloats = Haze::kBufferFloats + Stability::kBufferFloats + 2 * kLookLen;
  enum Curve { kSoft = 0, kLinear, kLate };

  void init(float *buffer) {
    haze_.init(buffer);
    stab_.init(buffer + Haze::kBufferFloats);
    look_[0].init(buffer + Haze::kBufferFloats + Stability::kBufferFloats, kLookLen);
    look_[1].init(buffer + Haze::kBufferFloats + Stability::kBufferFloats + kLookLen, kLookLen);
    lookSamples_ = (int32_t)msToSamples(8.f);
    space_.setTime(40.f);
    setSwell(800.f);
    setSens(0.6f);
    setDecay(6.f);
    setTone(6000.f);
    setSize(0.7f);
    haze_.setModulation(0.6f);
    fastA_ = timeCoef(0.5f); fastR_ = timeCoef(40.f);
    slowA_ = timeCoef(60.f); slowR_ = timeCoef(500.f);
    duckStep_ = 1.f / msToSamples(6.f);
    levA_ = timeCoef(5.f); levR_ = timeCoef(250.f);
    susGain_.setTime(60.f);
    susGain_.snap(1.f);
    setSustain(0.5f);
  }

  // ---- parameters ----
  void setSwell(float ms) { swellInc_ = 1.f / msToSamples(clampf(ms, 20.f, 8000.f)); }
  void setSpace(float m) { space_.setTarget(clampf(m, 0.f, 1.f)); }
  void setDecay(float sec) { haze_.setDecay(sec); }
  void setSens(float s) {
    s = clampf(s, 0.f, 1.f);
    floor_ = dbToGain(-30.f - 30.f * s);   // -30 dB (insensitive) .. -60 dB (sensitive)
    jump_ = 4.f - 2.5f * s;                // fast/slow ratio needed for a new note: 4x .. 1.5x
  }
  void setCurve(int c) { curve_ = c < 0 ? 0 : (c > 2 ? 2 : c); }
  void setTone(float hz) { haze_.setDamping(hz); }
  void setSize(float s) { haze_.setSize(s); }
  void setSustain(float s) { sustain_ = clampf(s, 0.f, 1.f); }
  void setStability(float s) { stab_.setAmount(s); }

  void process(const float *in, float *out, uint32_t frames) {
    for (uint32_t i = 0; i < frames; ++i) {
      const float inL = in[2 * i], inR = in[2 * i + 1];
      const float lvl = 0.5f * (absf(inL) + absf(inR));

      // Envelope followers
      fast_ += (lvl > fast_ ? fastA_ : fastR_) * (lvl - fast_);
      slow_ += (lvl > slow_ ? slowA_ : slowR_) * (lvl - slow_);
      fast_ = undenormal(fast_);
      slow_ = undenormal(slow_);

      // Onset detection
      if (fast_ < floor_ * 0.5f) armed_ = true;
      if (holdoff_) --holdoff_;
      const bool afterQuiet = armed_ && fast_ > floor_;
      const bool jumped = !holdoff_ && fast_ > floor_ && fast_ > slow_ * jump_;
      if (afterQuiet || jumped) {
        armed_ = false;
        holdoff_ = (uint32_t)msToSamples(120.f);
        ducking_ = true;
        susGain_.snap(1.f);   // no boost on a fresh attack; it re-rises as the note decays
      }

      // Swell gain: dip to zero, then rise along the curve.
      if (ducking_) {
        gain_ -= duckStep_;
        if (gain_ <= 0.f) { gain_ = 0.f; ducking_ = false; pos_ = 0.f; }
      } else if (pos_ < 1.f) {
        pos_ = minf(1.f, pos_ + swellInc_);
        gain_ = shape(pos_);
      }

      // Sustain leveller: pull the average level toward about -16 dBFS, by up to +24 dB.
      lev_ += (lvl > lev_ ? levA_ : levR_) * (lvl - lev_);
      lev_ = undenormal(lev_);
      // The boost follows the smooth level only (a fast follower would ripple
      // at audio rate and distort). New attacks reset it above.
      float boost = 1.f;
      if (sustain_ > 0.f && lev_ > floor_) {
        const float want = 0.15f / lev_;   // gain that would reach the target
        boost = minf(fpow(maxf(want, 1.f), sustain_), 15.85f);   // only lifts, max +24 dB
      }
      if (boost < susGain_.value()) susGain_.snap(boost);
      else susGain_.setTarget(boost);
      const float g = gain_ * susGain_.next();
      look_[0].write(inL);
      look_[1].write(inR);
      const float sl = look_[0].at(lookSamples_) * g, sr = look_[1].at(lookSamples_) * g;

      // Reverb on the swelled signal
      float rl, rr;
      haze_.process(sl, sr, rl, rr);

      float dry, wet;
      equalPowerGains(space_.next(), dry, wet);
      float ol = dry * sl + wet * rl * 1.3f;
      float orr = dry * sr + wet * rr * 1.3f;
      stab_.process(ol, orr);
      out[2 * i] = softLimit(ol);
      out[2 * i + 1] = softLimit(orr);
    }
  }

 private:
  float shape(float p) const {
    switch (curve_) {
      case kLinear: return p;
      case kLate: return p * p * p;               // slow start, blooms at the end
      default: return 0.5f - 0.5f * fcos(p * kPi);  // smooth S-curve
    }
  }

  Haze haze_;
  Stability stab_;
  DelayLine look_[2];
  int32_t lookSamples_ = 384;
  Smoother space_;
  Smoother susGain_;
  float lev_ = 0.f, levA_ = 1.f, levR_ = 1.f, sustain_ = 0.5f;
  float fast_ = 0.f, slow_ = 0.f;
  float fastA_ = 1.f, fastR_ = 1.f, slowA_ = 1.f, slowR_ = 1.f;
  float floor_ = 0.001f, jump_ = 2.5f;
  float gain_ = 0.f, pos_ = 1.f, swellInc_ = 0.f, duckStep_ = 0.f;
  bool armed_ = true, ducking_ = false;
  uint32_t holdoff_ = 0;
  int curve_ = kSoft;
};

}  // namespace bloom
