#pragma once
/*
 *  Sympathetic Strings — a bank of 12 tuned strings that ring along with
 *  whatever you play, like the sympathetic strings of a sitar or a piano with
 *  the dampers up. Notes that belong to the chosen chord set the strings
 *  ringing; notes outside it barely touch them.
 *  Hi-Fi Hopes
 *
 *  Each string is a digital waveguide: a delay line one period long with a
 *  damping filter in its feedback loop. The input excites all 12; each one
 *  resonates at its own pitch and harmonics.
 *
 *  Chords: 7 dreamy voicings per key (major or minor). Changing chord re-tunes
 *  the strings with a short glide, so ringing strings slide into the new chord.
 */

#include "hfh.h"

namespace sym {

using namespace hfh;

constexpr int kStrings = 12;
constexpr uint32_t kStringLen = 2048;   // longest period ~43 ms (~24 Hz), plenty of headroom
constexpr int kChords = 7;
constexpr int kMaxTones = 5;

// Chord tones in semitones above the key root. -1 = unused.
static const int8_t kMajorChords[kChords][kMaxTones] = {
  {0, 4, 7, 11, 14},   // I maj9
  {2, 5, 9, 12, 16},   // ii m9
  {4, 7, 11, 14, -1},  // iii m7
  {5, 9, 12, 16, 19},  // IV maj9
  {7, 12, 14, 17, -1}, // V 7sus4
  {9, 12, 16, 19, 23}, // vi m9
  {0, 7, 14, 19, -1},  // I open (5ths and 9ths)
};
static const int8_t kMinorChords[kChords][kMaxTones] = {
  {0, 3, 7, 10, 14},   // i m9
  {3, 7, 10, 14, -1},  // bIII maj7
  {5, 8, 12, 15, 19},  // iv m9
  {8, 12, 15, 19, 26}, // bVI maj9
  {10, 14, 17, 24, -1},// bVII add9
  {7, 10, 14, 17, -1}, // v m7
  {0, 7, 14, 19, -1},  // i open
};

inline const char *chordName(int scale, int chord) {
  static const char *kMaj[kChords] = {"I maj9", "ii m9", "iii m7", "IV maj9", "V sus", "vi m9", "I open"};
  static const char *kMin[kChords] = {"i m9", "bIII maj7", "iv m9", "bVI maj9", "bVII add9", "v m7", "i open"};
  if (chord < 0 || chord >= kChords) return "";
  return scale ? kMin[chord] : kMaj[chord];
}

class String {
 public:
  void init(float *buf, uint32_t seed) {
    line_.init(buf, kStringLen);
    len_.setTime(120.f);   // re-tune glide
    drift_ = Drift(seed);
    drift_.setRate(0.11f + 0.03f * (float)(seed % 7));
  }
  void setPitch(float hz, bool snap) {
    hz_ = hz;
    const float period = kSampleRate / hz;
    if (snap) len_.snap(period); else len_.setTarget(period);
  }
  // Loop gain and damping, recomputed at block rate.
  void setLoop(float ringSec, float dampCoef) {
    gain_ = fexp2(-3.f / (hz_ * ringSec) * 3.3219281f);   // 10^(-3 / (f * T60))
    dampC_ = dampCoef;
    // The damping filter delays the loop slightly; shorten the line to keep pitch true.
    comp_ = (1.f - dampC_) / dampC_;
  }
  // sqrt(1 - g): keeps the ringing energy roughly constant across RING times.
  float excitation() const { return __builtin_sqrtf(1.f - gain_); }

  // Slow detune drift, updated once per block (it moves far slower than that).
  void beginBlock(float detuneCents, uint32_t frames) {
    ratio_ = 1.f + detuneCents * drift_.nextBlock(frames) * 0.000577623f;  // cents -> ratio (small-angle)
  }

  float process(float exc) {
    const float d = clampf(len_.next() * ratio_ - comp_ - 1.f, 2.f, (float)kStringLen - 4.f);
    float r = line_.readHermite(d);
    lp_ += dampC_ * (r - lp_);
    const float y = exc + lp_ * gain_;
    line_.write(undenormal(softclip(y * 0.5f) * 2.f));
    return y;
  }

 private:
  DelayLine line_;
  Smoother len_;
  Drift drift_;
  float hz_ = 110.f, gain_ = 0.9f, dampC_ = 0.5f, comp_ = 1.f, lp_ = 0.f, ratio_ = 1.f;
};

class SympatheticStrings {
 public:
  static constexpr uint32_t kBufferFloats = kStrings * kStringLen + Stability::kBufferFloats;

  void init(float *buffer) {
    for (int i = 0; i < kStrings; ++i) strings_[i].init(buffer + i * kStringLen, 7u + 13u * i);
    stab_.init(buffer + kStrings * kStringLen);
    inHp_.setCutoff(120.f);
    mix_.setTime(30.f);
    retune(true);
    updateLoop();
  }

  // ---- parameters ----
  void setChord(int c) { c = c < 0 ? 0 : (c >= kChords ? kChords - 1 : c); if (c != chord_) { chord_ = c; retuneNeeded_ = true; } }
  void setKey(int k) { k = k < 0 ? 0 : (k > 11 ? 11 : k); if (k != key_) { key_ = k; retuneNeeded_ = true; } }
  void setScale(int s) { s = s ? 1 : 0; if (s != scale_) { scale_ = s; retuneNeeded_ = true; } }
  void setRing(float sec) { ring_ = clampf(sec, 0.5f, 30.f); loopDirty_ = true; }
  void setTone(float hz) { tone_ = clampf(hz, 800.f, 12000.f); loopDirty_ = true; }
  void setMix(float m) { mix_.setTarget(clampf(m, 0.f, 1.f)); }
  void setDrift(float d) { detune_ = clampf(d, 0.f, 1.f) * 12.f; }   // up to +/-12 cents
  void setStability(float s) { stab_.setAmount(s); }
  int scale() const { return scale_; }

  void process(const float *in, float *out, uint32_t frames) {
    if (retuneNeeded_) retune(false);
    if (loopDirty_) updateLoop();
    for (int s = 0; s < kStrings; ++s) strings_[s].beginBlock(detune_, frames);

    for (uint32_t i = 0; i < frames; ++i) {
      const float inL = in[2 * i], inR = in[2 * i + 1];
      const float x = inHp_.process(0.5f * (inL + inR));
      float wl = 0.f, wr = 0.f;
      for (int s = 0; s < kStrings; ++s) {
        // Excitation scaled so the wet level stays within a few dB from a
        // 2 s ring to a 30 s ring (house rule: level-matched).
        const float y = strings_[s].process(x * strings_[s].excitation() * 0.5f);
        // Spread strings across the stereo field, low strings nearer the centre.
        const float w = 0.5f + 0.4f * (float)(s % 2 ? 1 : -1) * (0.3f + 0.7f * (float)s / (kStrings - 1));
        wl += y * (1.f - w);
        wr += y * w;
      }
      wl = ftanh(wl * 0.5f);
      wr = ftanh(wr * 0.5f);
      stab_.process(wl, wr);
      float dry, wet;
      equalPowerGains(mix_.next(), dry, wet);
      out[2 * i] = dry * inL + wet * wl;
      out[2 * i + 1] = dry * inR + wet * wr;
    }
  }

 private:
  void retune(bool snap) {
    retuneNeeded_ = false;
    const int8_t *tones = scale_ ? kMinorChords[chord_] : kMajorChords[chord_];
    int n = 0;
    while (n < kMaxTones && tones[n] >= 0) ++n;
    for (int s = 0; s < kStrings; ++s) {
      // Stack the chord upward from the key root in octave 2 (C2 = MIDI 36).
      const int note = 36 + key_ + tones[s % n] + 12 * (s / n);
      const float hz = 440.f * fexp2(((float)note - 69.f) / 12.f);
      strings_[s].setPitch(hz, snap);
    }
    loopDirty_ = true;
  }
  void updateLoop() {
    loopDirty_ = false;
    const float c = cutoffCoef(tone_);
    for (int s = 0; s < kStrings; ++s) strings_[s].setLoop(ring_, c);
  }

  String strings_[kStrings];
  Stability stab_;
  OnePoleHP inHp_;
  Smoother mix_;
  int chord_ = 0, key_ = 0, scale_ = 0;
  float ring_ = 6.f, tone_ = 5000.f, detune_ = 3.f;
  volatile bool retuneNeeded_ = true, loopDirty_ = true;
};

}  // namespace sym
