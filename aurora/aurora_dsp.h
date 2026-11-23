#pragma once
/*
 *  Aurora — a shimmer reverb whose shimmer follows a chord.
 *  Hi-Fi Hopes
 *
 *  Classic shimmer pitch-shifts the reverb's own output up an octave and feeds
 *  it back in, so the tail keeps climbing. Aurora uses two pitch voices at
 *  once, set to chord tones (octave, fifth, ninth, major or minor triad...),
 *  so the tail blooms into a chord instead of a single octave stack.
 *
 *    in -> Haze reverb -> out
 *            ^      |
 *            |      v
 *            +-- (voice A + voice B) -> high-pass -> low-pass -> x BLOOM
 */

#include "hfh.h"

namespace aurora {

using namespace hfh;

constexpr int kChords = 8;

struct Chord { float a, b; const char *name; };
static const Chord kChordTable[kChords] = {
  {12.f, 12.f, "Octave"},
  {7.f, 7.f, "Fifth"},
  {12.f, 7.f, "Oct 5th"},
  {12.f, 24.f, "2 Octaves"},
  {7.f, 14.f, "5th 9th"},
  {4.f, 7.f, "Major"},
  {3.f, 7.f, "Minor"},
  {-12.f, 12.f, "Sub Oct"},
};

class Aurora {
 public:
  static constexpr uint32_t kBufferFloats = Haze::kBufferFloats + 2 * PitchShifter::kBufferFloats + Stability::kBufferFloats;

  void init(float *buffer) {
    float *p = buffer;
    haze_.init(p); p += Haze::kBufferFloats;
    voiceA_.init(p); p += PitchShifter::kBufferFloats;
    voiceB_.init(p); p += PitchShifter::kBufferFloats;
    stab_.init(p);
    voiceA_.setWindowMs(73.f);   // different windows so the voices' warble doesn't line up
    voiceB_.setWindowMs(91.f);
    shimHp_.setCutoff(250.f);
    bloom_.setTime(60.f);
    mix_.setTime(40.f);
    haze_.setModulation(0.5f);
    setChord(0);
    setDecay(8.f);
    setTone(7000.f);
    setSize(0.8f);
    setBloom(0.4f);
    setMix(0.5f);
  }

  // ---- parameters ----
  void setChordPosition(int v) {   // 0..799, 100 per chord, with hysteresis
    const int zone = v / 100 > kChords - 1 ? kChords - 1 : v / 100;
    if (zone == chord_) return;
    const int edge = zone > chord_ ? zone * 100 : (zone + 1) * 100;
    if (v - edge > 15 || edge - v > 15) setChord(zone);
  }
  void setChord(int c) {
    chord_ = c < 0 ? 0 : (c >= kChords ? kChords - 1 : c);
    voiceA_.setSemitones(kChordTable[chord_].a);
    voiceB_.setSemitones(kChordTable[chord_].b);
  }
  void setBloom(float b) { bloom_.setTarget(clampf(b, 0.f, 1.f) * 0.55f); }
  void setDecay(float sec) { haze_.setDecay(sec); }
  void setMix(float m) { mix_.setTarget(clampf(m, 0.f, 1.f)); }
  void setTone(float hz) { haze_.setDamping(hz); shimLp_.setCutoff(clampf(hz * 1.2f, 1500.f, 14000.f)); }
  void setModulation(float m) { haze_.setModulation(m); }
  void setSize(float s) { haze_.setSize(s); }
  void setStability(float s) { stab_.setAmount(s); }
  static const char *chordName(int v) { int c = v / 100; return kChordTable[c < 0 ? 0 : (c > kChords - 1 ? kChords - 1 : c)].name; }

  void process(const float *in, float *out, uint32_t frames) {
    for (uint32_t i = 0; i < frames; ++i) {
      const float inL = in[2 * i], inR = in[2 * i + 1];
      const float shim = shim_ * bloom_.next();
      float rl, rr;
      haze_.process(inL + shim, inR + shim, rl, rr);

      // Shimmer path: the reverb's output, re-pitched to two chord tones.
      const float mono = 0.5f * (rl + rr);
      float s = 0.5f * (voiceA_.process(mono) + voiceB_.process(mono));
      s = shimLp_.process(shimHp_.process(s));
      shim_ = softclip(s * 1.4f);

      stab_.process(rl, rr);
      float dry, wet;
      equalPowerGains(mix_.next(), dry, wet);
      out[2 * i] = softLimit(dry * inL + wet * rl * 1.5f);
      out[2 * i + 1] = softLimit(dry * inR + wet * rr * 1.5f);
    }
  }

 private:
  Haze haze_;
  PitchShifter voiceA_, voiceB_;
  Stability stab_;
  OnePoleHP shimHp_;
  OnePoleLP shimLp_;
  Smoother bloom_, mix_;
  float shim_ = 0.f;
  int chord_ = -1;
};

}  // namespace aurora
