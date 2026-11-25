#pragma once
/*
 *  Glass Bells — a bell synth you play on the pad.
 *  Hi-Fi Hopes
 *
 *  Touch the pad to strike a bell. X picks the note (locked to a scale, so
 *  every note fits); sliding to a new note strikes it. Y sets the bell's
 *  character at the moment it's struck: soft celesta -> glass -> sparkle.
 *  Bells ring on after you let go (8 voices, oldest/quietest stolen), into a
 *  built-in Haze reverb. Whatever comes into the NTS-3 passes through
 *  untouched, so the bells play on top of it.
 *
 *  Each voice is 2-operator FM (the classic DX bell): a sine modulator at an
 *  inharmonic ratio bends a sine carrier. The brightness (FM index) dies
 *  faster than the volume, so each strike rings bright and then mellows.
 *  Two carriers a few cents apart give every bell its own stereo shimmer.
 */

#include "hfh.h"

namespace gb {

using namespace hfh;

constexpr int kVoices = 8;
constexpr int kScales = 6;
constexpr int kRootMidi = 60;   // KEY = C puts the lowest note at C4

struct Scale { int n; int steps[7]; const char *name; };
static const Scale kScaleTable[kScales] = {
  {5, {0, 2, 4, 7, 9, 0, 0}, "Penta Maj"},
  {5, {0, 3, 5, 7, 10, 0, 0}, "Penta Min"},
  {7, {0, 2, 4, 5, 7, 9, 11}, "Major"},
  {7, {0, 2, 3, 5, 7, 8, 10}, "Minor"},
  {7, {0, 2, 4, 6, 7, 9, 11}, "Lydian"},
  {7, {0, 2, 3, 5, 7, 9, 10}, "Dorian"},
};
static const char *const kNoteNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

inline float midiToHz(float m) { return 440.f * fexp2((m - 69.f) * (1.f / 12.f)); }

class BellVoice {
 public:
  void strike(float hz, float ratio, float index, float sparkle, float vel, float ringSec, float pan) {
    const float detune = fexp2(2.f / 1200.f);   // +/- 2 cents between the two carriers
    incL_ = hz * kSampleTime / detune;
    incR_ = hz * kSampleTime * detune;
    incM_ = hz * ratio * kSampleTime;
    incS_ = hz * 5.43f * kSampleTime;           // high inharmonic "glass" partial
    phL_ = phR_ = phM_ = phS_ = 0.f;
    amp_ = vel;
    idx_ = index;
    spk_ = sparkle * vel;
    atk_ = 0.f;
    // Amplitude falls 60 dB over RING; brightness and sparkle 4x and 10x faster.
    const float n = ringSec * kSampleRate;
    ampMul_ = fexp2(-3.f * 3.3219281f / n);
    idxMul_ = fexp2(-4.f * 3.3219281f / n);
    spkMul_ = fexp2(-10.f * 3.3219281f / n);
    gainL_ = fsqrt(0.5f - 0.5f * pan);
    gainR_ = fsqrt(0.5f + 0.5f * pan);
  }

  bool active() const { return amp_ > 1e-4f; }
  float level() const { return amp_; }

  void process(float &l, float &r) {
    if (!active()) return;
    atk_ += 0.015f * (1.f - atk_);   // ~1.4 ms strike attack, no click
    const float m = idx_ * (1.f / kTwoPi) * sinTurns(phM_);
    const float env = amp_ * atk_;
    const float s = spk_ * atk_ * sinTurns(phS_);
    l += gainL_ * (env * sinTurns(phL_ + m) + s);
    r += gainR_ * (env * sinTurns(phR_ + m) + s);
    phL_ = wrap(phL_ + incL_);
    phR_ = wrap(phR_ + incR_);
    phM_ = wrap(phM_ + incM_);
    phS_ = wrap(phS_ + incS_);
    amp_ *= ampMul_;
    idx_ *= idxMul_;
    spk_ *= spkMul_;
  }

 private:
  static float wrap(float p) { return p >= 1.f ? p - 1.f : p; }
  float phL_ = 0.f, phR_ = 0.f, phM_ = 0.f, phS_ = 0.f;
  float incL_ = 0.f, incR_ = 0.f, incM_ = 0.f, incS_ = 0.f;
  float amp_ = 0.f, idx_ = 0.f, spk_ = 0.f, atk_ = 0.f;
  float ampMul_ = 1.f, idxMul_ = 1.f, spkMul_ = 1.f;
  float gainL_ = 0.707f, gainR_ = 0.707f;
};

class GlassBells {
 public:
  static constexpr uint32_t kBufferFloats = Haze::kBufferFloats;

  void init(float *buffer) {
    haze_.init(buffer);
    haze_.setSize(0.85f);
    haze_.setDamping(6500.f);
    haze_.setModulation(0.6f);
    level_.setTime(30.f);
    space_.setTime(60.f);
    level_.snap(0.7f);
    setKey(0);
    setScale(1);
    setTone(0.5f);
    setRing(6.f);
    setSpace(0.5f);
    space_.snap(space_.target());
    setRepeat(0.f);
    held_ = false;
    pending_ = 0;
    pendingCancelable_ = false;
    zone_ = 0;
    noteParam_ = 0;
    repeatCount_ = 0.f;
  }

  // ---- parameters ----
  void setNoteParam(int32_t v) {   // 0..1000 across the scale range
    noteParam_ = v;
    const int z = zoneWithHysteresis(v);
    if (z == zone_) return;
    zone_ = z;
    if (!held_) return;
    // A new note under the finger. Strike it after a short wait, so a param
    // jump that arrives just before "touch ended" doesn't ring a stray note.
    if (pending_ > 0) return;   // the first strike of this touch hasn't fired yet; it'll use this note
    pending_ = kSlideDelay;
    pendingCancelable_ = true;
  }
  void setTone(float t) { tone_ = clampf(t, 0.f, 1.f); }
  void setLevel(float l) { level_.setTarget(clampf(l, 0.f, 1.f)); }
  void setKey(int k) { key_ = k < 0 ? 0 : (k > 11 ? 11 : k); }
  void setScale(int s) { scale_ = s < 0 ? 0 : (s >= kScales ? kScales - 1 : s); zone_ = zoneFor(noteParam_); }
  void setRing(float sec) { ring_ = clampf(sec, 0.5f, 30.f); }
  void setSpace(float s) {
    s = clampf(s, 0.f, 1.f);
    space_.setTarget(s);
    haze_.setDecay(2.f + 10.f * s);   // more space, longer hall
  }
  void setRepeat(float hz) { repeatHz_ = hz; }   // 0 = off

  // ---- touch ----
  void touchBegan() {
    held_ = true;
    pending_ = kStrikeDelay;          // let the X position for this touch arrive first
    pendingCancelable_ = false;       // a quick tap still rings
    repeatCount_ = 0.f;
  }
  void touchEnded() {
    held_ = false;
    if (pendingCancelable_) pending_ = 0;
  }

  // ---- display ----
  int notesInRange() const { return 2 * kScaleTable[scale_].n + 1; }   // two octaves, top root included
  int midiForZone(int z) const {
    const Scale &sc = kScaleTable[scale_];
    return kRootMidi + key_ + 12 * (z / sc.n) + sc.steps[z % sc.n];
  }
  int zoneFor(int32_t v) const {
    const int n = notesInRange();
    int z = (int)((v * n) / 1001);
    return z < 0 ? 0 : (z >= n ? n - 1 : z);
  }
  static const char *scaleName(int s) { return kScaleTable[s < 0 ? 0 : (s >= kScales ? kScales - 1 : s)].name; }
  static const char *keyName(int k) { return kNoteNames[k < 0 ? 0 : (k > 11 ? 11 : k)]; }
  // Note name for a NOTE value, e.g. "D#5". Writes into buf (>= 5 chars).
  const char *noteName(int32_t v, char *buf) const {
    const int m = midiForZone(zoneFor(v));
    const char *n = kNoteNames[m % 12];
    int i = 0;
    while (*n) buf[i++] = *n++;
    buf[i++] = (char)('0' + (m / 12 - 1));
    buf[i] = 0;
    return buf;
  }

  void strikeNow(float vel) {
    const float t = tone_;
    // Y: 0 soft celesta (harmonic 2:1) -> 0.5 glass (3.5:1) -> 1 sparkle (4.77:1, bright)
    float ratio, index;
    if (t < 0.5f) { const float u = t * 2.f; ratio = lerpf(2.f, 3.5f, u); index = lerpf(0.9f, 2.4f, u); }
    else { const float u = (t - 0.5f) * 2.f; ratio = lerpf(3.5f, 4.77f, u); index = lerpf(2.4f, 4.f, u); }
    const float sparkle = 0.04f + 0.12f * t;
    const int z = zone_;
    const float hz = midiToHz((float)midiForZone(z));
    const float pan = 0.6f * ((float)z / (float)(notesInRange() - 1) - 0.5f) + 0.15f * rng_.bipolar();
    allocate().strike(hz, ratio, index, sparkle, vel, ring_, pan);
  }

  void process(const float *in, float *out, uint32_t frames) {
    const float repeatInc = repeatHz_ * kSampleTime;
    for (uint32_t i = 0; i < frames; ++i) {
      if (pending_ > 0 && --pending_ == 0) { strikeNow(1.f); repeatCount_ = 0.f; }
      if (held_ && repeatInc > 0.f && pending_ == 0) {
        repeatCount_ += repeatInc;
        if (repeatCount_ >= 1.f) { repeatCount_ -= 1.f; strikeNow(0.5f + 0.25f * rng_.unipolar()); }
      }

      float bl = 0.f, br = 0.f;
      for (int v = 0; v < kVoices; ++v) voice_[v].process(bl, br);
      bl *= 0.6f;
      br *= 0.6f;

      float rl, rr;
      haze_.process(bl, br, rl, rr);
      const float sp = space_.next();
      const float lv = level_.next();
      const float wl = (1.f - 0.5f * sp) * bl + 1.6f * sp * rl;
      const float wr = (1.f - 0.5f * sp) * br + 1.6f * sp * rr;
      out[2 * i] = softLimit(in[2 * i] + lv * wl);
      out[2 * i + 1] = softLimit(in[2 * i + 1] + lv * wr);
    }
  }

 private:
  static constexpr uint32_t kStrikeDelay = 240;   // 5 ms
  static constexpr uint32_t kSlideDelay = 384;    // 8 ms

  int zoneWithHysteresis(int32_t v) const {
    const int n = notesInRange();
    const float w = 1001.f / (float)n;
    const float lo = (float)zone_ * w - 0.2f * w, hi = (float)(zone_ + 1) * w + 0.2f * w;
    if ((float)v >= lo && (float)v < hi) return zone_;
    return zoneFor(v);
  }

  BellVoice &allocate() {
    int best = 0;
    float quiet = 1e9f;
    for (int v = 0; v < kVoices; ++v) {
      if (!voice_[v].active()) return voice_[v];
      if (voice_[v].level() < quiet) { quiet = voice_[v].level(); best = v; }
    }
    return voice_[best];
  }

  BellVoice voice_[kVoices];
  Haze haze_;
  Rng rng_{0xB311u};
  Smoother level_, space_;
  float tone_ = 0.5f, ring_ = 6.f, repeatHz_ = 0.f, repeatCount_ = 0.f;
  int key_ = 0, scale_ = 1, zone_ = 0;
  int32_t noteParam_ = 0;
  uint32_t pending_ = 0;
  bool held_ = false, pendingCancelable_ = false;
};

}  // namespace gb
