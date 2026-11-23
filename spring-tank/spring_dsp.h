#pragma once
/*
 *  Spring Tank — spring reverb for the Korg NTS-3 kaoss pad kit
 *  Hi-Fi Hopes
 *
 *  Plain C++ DSP core with no SDK dependencies, so the same code runs on the
 *  NTS-3 and on a desktop computer (tools/render_demo.cc) for listening tests.
 *
 *  How it works
 *  ------------
 *  A real spring tank is a few coiled springs between a driver and a pickup.
 *  Sound travels down each spring, reflects at the ends and comes back again
 *  and again, and on every trip the springs smear it in time: higher
 *  frequencies arrive later than lower ones. That smear is the "drip" or
 *  "boing" chirp that makes a spring sound like a spring.
 *
 *  Each modelled spring is a feedback loop:
 *
 *      send -> (+) -> dispersion (stretched allpass chain) -> delay -> filters -> out
 *               ^                                                          |
 *               +------------------------- feedback gain <-----------------+
 *
 *  - Dispersion: kStages first-order allpass filters with every unit delay
 *    stretched to kStretch samples. Group delay rises with frequency up to
 *    about fs / (2 * kStretch) = 4.8 kHz, which gives the rising chirp. The
 *    DRIP parameter sets the allpass coefficient, which sets how big the smear is.
 *  - Delay: the spring's transit time (tens of ms), slowly wobbled so the
 *    tail stays alive.
 *  - Filters: a low-pass (springs are dull above ~4-5 kHz) and a high-pass
 *    (springs carry little bass).
 *  - Feedback gain sets how long the tank rings (DECAY).
 *
 *  DWELL works like the dwell knob on a tube spring unit: it drives the
 *  transducer harder, so the tank gets louder, splashier and saturates.
 *  CRASH injects a short noise burst, like bumping the tank.
 */

#include <stdint.h>
#include <math.h>
#include <string.h>

namespace st {

constexpr float kFs = 48000.f;
constexpr float kPi = 3.14159265358979f;

constexpr int kMaxSprings = 3;
constexpr int kStages = 48;          // dispersion allpasses per spring
constexpr int kStretch = 5;          // stretch factor: chirps live below fs/(2K) = 4.8 kHz
constexpr int kDelayLen = 4096;      // per-spring delay line, 85 ms, power of two
constexpr int kDelayMask = kDelayLen - 1;

// Cubic soft clipper: unity gain near zero, flat at +/-1 beyond |x| = 1.5.
inline float softclip(float x) {
  if (x > 1.5f) return 1.f;
  if (x < -1.5f) return -1.f;
  return x - (4.f / 27.f) * x * x * x;
}

inline float clampf(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }

// Fast 2^x (rel. error < 1e-4). Used instead of expf/powf/sinf so the unit
// stays small: the NTS-3 allows ~32 KB per unit and libm's versions alone
// take several KB. Only used for coefficients, never per sample.
inline float fexp2(float x) {
  x = clampf(x, -126.f, 126.f);
  int i = (int)x;
  if ((float)i > x) --i;              // floor
  const float f = x - (float)i;       // 0..1
  const float p = 1.f + f * (0.6931472f + f * (0.2402265f + f * (0.0555041f + f * (0.0096181f + f * 0.0013334f))));
  union { float f; int32_t i; } u;
  u.i = (i + 127) << 23;
  return p * u.f;
}
inline float fexp(float x) { return fexp2(x * 1.4426950f); }

// One-pole coefficient for a cutoff in Hz.
inline float onePoleCoef(float hz) { return 1.f - fexp(-2.f * kPi * hz / kFs); }

struct Character {
  float delayMs[kMaxSprings];  // transit time of each spring
  float loopLpHz;              // brightness of the springs at TONE = 50%
  float dripMin, dripMax;      // allpass coefficient range covered by DRIP
  float rtScale;               // decay time multiplier
  float driveMax;              // DWELL at 100%
  float sendHpHz;              // how much bass reaches the tank
  float wetGain;
};

// 0: SR-202W style — short tank like the one in a 70s receiver/amp: tighter,
//    darker, less drip.
// 1: SURF — long 3-spring tank like a 60s outboard tube unit: wet, bright, drippy.
static const Character kCharacters[2] = {
  {{23.0f, 29.0f, 26.3f}, 3200.f, 0.30f, 0.62f, 1.00f, 5.f, 180.f, 1.05f},
  {{33.0f, 41.0f, 37.3f}, 4600.f, 0.48f, 0.78f, 1.50f, 9.f, 120.f, 0.95f},
};

class SpringTank {
 public:
  // External memory needed for the delay lines, in floats.
  static constexpr uint32_t kBufferFloats = kMaxSprings * kDelayLen;

  void init(float *buffer) {
    for (int s = 0; s < kMaxSprings; ++s) delay_[s] = buffer + s * kDelayLen;
    dwell_ = 0.4f; mix_ = 0.5f; decay_ = 0.45f; tone_ = 0.5f; drip_ = 0.55f;
    numSprings_ = 3; character_ = 0; crashEnabled_ = true;
    reset();
  }

  void reset() {
    for (int s = 0; s < kMaxSprings; ++s) {
      if (delay_[s]) memset(delay_[s], 0, sizeof(float) * kDelayLen);
      fb_[s] = 0.f; lp_[s] = 0.f; hp_[s] = 0.f;
      lfoPhase_[s] = 0.31f * s;
    }
    memset(apX_, 0, sizeof(apX_));
    memset(apY_, 0, sizeof(apY_));
    apPos_ = 0; writePos_ = 0;
    sendHp_ = 0.f; sendLp_ = 0.f; wetLpL_ = wetLpR_ = 0.f;
    crashEnv_ = 0.f; pendingCrash_ = false; touching_ = false; touchSamples_ = 0;
    driveSm_ = 1.f; drySm_ = 1.f; wetSm_ = 0.f;
    updateCoefs();
  }

  // ---- parameters (normalized 0..1 unless noted) ----
  void setDwell(float v) { dwell_ = clampf(v, 0.f, 1.f); }
  void setMix(float v) { mix_ = clampf(v, 0.f, 1.f); }
  void setDecay(float v) { decay_ = clampf(v, 0.f, 1.f); dirty_ = true; }
  void setTone(float v) { tone_ = clampf(v, 0.f, 1.f); dirty_ = true; }
  void setDrip(float v) { drip_ = clampf(v, 0.f, 1.f); dirty_ = true; }
  void setSprings(int n) { numSprings_ = n < 2 ? 2 : (n > 3 ? 3 : n); dirty_ = true; }
  void setCharacter(int c) { character_ = c ? 1 : 0; dirty_ = true; }
  void setCrashEnabled(bool on) { crashEnabled_ = on; }

  // ---- touch ----
  // A quick tap (finger down and up within ~0.2 s) crashes the springs.
  void touchBegan() { touching_ = true; touchSamples_ = 0; }
  void touchEnded() {
    if (touching_ && crashEnabled_ && touchSamples_ < (uint32_t)(0.2f * kFs)) pendingCrash_ = true;
    touching_ = false;
  }
  void touchCancelled() { touching_ = false; }
  void triggerCrash() { pendingCrash_ = true; }  // used by the desktop demo

  // Interleaved stereo in/out, as on the NTS-3.
  void process(const float *in, float *out, uint32_t frames) {
    if (dirty_) updateCoefs();
    const Character &ch = kCharacters[character_];

    if (touching_) touchSamples_ += frames;
    if (pendingCrash_) { pendingCrash_ = false; crashEnv_ = 1.f; crashKick_ = true; }

    // Block-rate targets, smoothed per sample to avoid zipper noise.
    const float driveT = 1.f + dwell_ * (ch.driveMax - 1.f);
    // Equal-power crossfade. VSQRT on the NTS-3; never libm's sqrtf (see fsqrt in hfh_math.h).
#if defined(__arm__) && defined(__ARM_FP)
    float wetT, dryT;
    { const float a = mix_, b = 1.f - mix_;
      __asm__("vsqrt.f32 %0, %1" : "=t"(wetT) : "t"(a));
      __asm__("vsqrt.f32 %0, %1" : "=t"(dryT) : "t"(b)); }
#else
    const float wetT = __builtin_sqrtf(mix_);
    const float dryT = __builtin_sqrtf(1.f - mix_);
#endif
    const float sm = 0.002f;

    const int n = numSprings_;
    const float springIn = n == 3 ? 0.577f : 0.707f;
    const float crashDecay = crashDecay_;

    for (uint32_t i = 0; i < frames; ++i) {
      driveSm_ += sm * (driveT - driveSm_);
      wetSm_ += sm * (wetT - wetSm_);
      drySm_ += sm * (dryT - drySm_);

      const float inL = in[2 * i];
      const float inR = in[2 * i + 1];
      float x = 0.5f * (inL + inR);

      // Band-limit what reaches the tank, then the dwell drive.
      sendHp_ += sendHpC_ * (x - sendHp_);
      x -= sendHp_;
      sendLp_ += sendLpC_ * (x - sendLp_);
      x = sendLp_;
      float send = softclip(x * driveSm_ * 0.6f) * 0.8f;

      // Crash: a short decaying noise burst, plus a hard first kick.
      if (crashEnv_ > 1e-4f) {
        rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
        const float noise = (float)(int32_t)rng_ * 4.6566e-10f;
        send += noise * crashEnv_ * 0.9f;
        if (crashKick_) { send += 1.2f; crashKick_ = false; }
        crashEnv_ *= crashDecay;
      }

      float springOut[kMaxSprings] = {0.f, 0.f, 0.f};
      const int ap = apPos_;
      for (int s = 0; s < n; ++s) {
        // Into the spring: new sound plus what came back from the last trip.
        float u = send * springIn + fb_[s] + 1e-20f;

        // Dispersion: stretched allpass chain. All stages share one ring
        // position because they all use the same stretch.
        const float a = apCoef_[s];
        float *xs = &apX_[s][0][0];
        float *ys = &apY_[s][0][0];
        for (int k = 0; k < kStages; ++k) {
          const int idx = k * kStretch + ap;
          const float y = a * (u - ys[idx]) + xs[idx];
          xs[idx] = u;
          ys[idx] = y;
          u = y;
        }

        // Transit delay, with a slow wobble.
        float *d = delay_[s];
        d[writePos_] = u;
        lfoPhase_[s] += lfoInc_[s];
        if (lfoPhase_[s] > 1.f) lfoPhase_[s] -= 1.f;
        const float tri = fabsf(2.f * lfoPhase_[s] - 1.f) * 2.f - 1.f;  // -1..1
        const float rd = delaySamps_[s] + tri * 3.f;
        const int ri = (int)rd;
        const float frac = rd - (float)ri;
        const float a0 = d[(writePos_ - ri) & kDelayMask];
        const float a1 = d[(writePos_ - ri - 1) & kDelayMask];
        float r = a0 + frac * (a1 - a0);

        // Spring colour: dull top, thin bottom.
        lp_[s] += loopLpC_ * (r - lp_[s]);
        r = lp_[s];
        hp_[s] += loopHpC_ * (r - hp_[s]);
        r -= hp_[s];

        springOut[s] = r;
        // Alternate the feedback polarity between springs to decorrelate them.
        fb_[s] = softclip(r * fbGain_[s]) * (s == 1 ? -1.f : 1.f);
      }
      apPos_ = (apPos_ + 1 == kStretch) ? 0 : apPos_ + 1;
      writePos_ = (writePos_ + 1) & kDelayMask;

      float wetL, wetR;
      if (n == 3) {
        wetL = springOut[0] + 0.6f * springOut[2];
        wetR = springOut[1] - 0.6f * springOut[2];
      } else {
        wetL = springOut[0] + 0.25f * springOut[1];
        wetR = springOut[1] + 0.25f * springOut[0];
      }
      wetL *= ch.wetGain * 1.4f;
      wetR *= ch.wetGain * 1.4f;
      wetLpL_ += outLpC_ * (wetL - wetLpL_);
      wetLpR_ += outLpC_ * (wetR - wetLpR_);
      wetL = softclip(wetLpL_);
      wetR = softclip(wetLpR_);

      out[2 * i] = drySm_ * inL + wetSm_ * wetL;
      out[2 * i + 1] = drySm_ * inR + wetSm_ * wetR;
    }
  }

 private:
  void updateCoefs() {
    dirty_ = false;
    const Character &ch = kCharacters[character_];
    // TONE: +/- about 1.25 octaves around the character's brightness.
    const float toneMul = fexp2((tone_ - 0.5f) * 2.5f);
    loopLpC_ = onePoleCoef(clampf(ch.loopLpHz * toneMul, 800.f, 9000.f));
    outLpC_ = onePoleCoef(clampf(ch.loopLpHz * 1.4f * toneMul, 1200.f, 14000.f));
    loopHpC_ = onePoleCoef(90.f);
    sendHpC_ = onePoleCoef(ch.sendHpHz);
    sendLpC_ = onePoleCoef(5500.f);
    crashDecay_ = fexp(-1.f / (0.015f * kFs));

    // DECAY: 0.5 s .. 6 s (RT60), scaled by character.
    const float rt60 = 0.5f * fexp2(decay_ * 3.5849625f) * ch.rtScale;  // 0.5 * 12^decay
    const float drip = ch.dripMin + drip_ * (ch.dripMax - ch.dripMin);
    for (int s = 0; s < kMaxSprings; ++s) {
      delaySamps_[s] = ch.delayMs[s] * 0.001f * kFs;
      // Slightly different drip per spring so they don't chirp in lockstep.
      apCoef_[s] = clampf(drip + 0.02f * (float)(s - 1), 0.f, 0.9f);
      // Round trip = transit delay + average dispersion delay (an allpass
      // chain averages one stretched unit delay per stage across the band).
      const float dispSamps = (float)(kStages * kStretch);
      const float trip = (delaySamps_[s] + dispSamps) / kFs;
      fbGain_[s] = fexp2(-3.f * trip / rt60 * 3.3219281f);  // 10^(-3 trip / rt60)
      lfoInc_[s] = (0.23f + 0.17f * s) / kFs;
    }
  }

  // memory
  float *delay_[kMaxSprings] = {nullptr, nullptr, nullptr};
  float apX_[kMaxSprings][kStages][kStretch];
  float apY_[kMaxSprings][kStages][kStretch];
  int apPos_ = 0;
  int writePos_ = 0;

  // per-spring state
  float fb_[kMaxSprings], lp_[kMaxSprings], hp_[kMaxSprings];
  float lfoPhase_[kMaxSprings], lfoInc_[kMaxSprings];
  float delaySamps_[kMaxSprings], apCoef_[kMaxSprings], fbGain_[kMaxSprings];

  // filters
  float sendHp_ = 0.f, sendLp_ = 0.f, wetLpL_ = 0.f, wetLpR_ = 0.f;
  float loopLpC_ = 0.f, loopHpC_ = 0.f, outLpC_ = 0.f, sendHpC_ = 0.f, sendLpC_ = 0.f;

  // crash
  float crashEnv_ = 0.f, crashDecay_ = 0.f;
  bool crashKick_ = false;
  uint32_t rng_ = 0x1234567u;
  volatile bool pendingCrash_ = false;
  volatile bool touching_ = false;
  volatile uint32_t touchSamples_ = 0;

  // smoothed controls
  float driveSm_ = 1.f, drySm_ = 1.f, wetSm_ = 0.f;

  // parameters
  float dwell_ = 0.4f, mix_ = 0.5f, decay_ = 0.45f, tone_ = 0.5f, drip_ = 0.55f;
  int numSprings_ = 3, character_ = 0;
  bool crashEnabled_ = true;
  volatile bool dirty_ = true;
};

}  // namespace st
