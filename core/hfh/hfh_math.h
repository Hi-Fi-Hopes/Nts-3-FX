#pragma once
/*
 *  HFH core — math
 *  Fast, small replacements for libm. The NTS-3 allows ~32 KB per unit and
 *  libm's exp/pow/sin/log pull in several KB, so everything here is a short
 *  polynomial. Accuracy is checked in tools/test_core.cc.
 */

#include <stdint.h>

namespace hfh {

constexpr float kPi = 3.14159265358979f;
constexpr float kTwoPi = 6.28318530717959f;
constexpr float kHalfPi = 1.57079632679490f;

// The NTS-3 runs at 48 kHz.
constexpr float kSampleRate = 48000.f;
constexpr float kSampleTime = 1.f / kSampleRate;

inline float clampf(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline float minf(float a, float b) { return a < b ? a : b; }
inline float maxf(float a, float b) { return a > b ? a : b; }
inline float absf(float x) { return x < 0.f ? -x : x; }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }

inline int32_t floorToInt(float x) {
  int32_t i = (int32_t)x;
  return ((float)i > x) ? i - 1 : i;
}

union FloatBits { float f; int32_t i; uint32_t u; };

// 2^x. Relative error < 1e-4.
inline float fexp2(float x) {
  x = clampf(x, -126.f, 126.f);
  const int32_t i = floorToInt(x);
  const float f = x - (float)i;
  const float p = 1.f + f * (0.6931472f + f * (0.2402265f + f * (0.0555041f + f * (0.0096181f + f * 0.0013334f))));
  FloatBits b;
  b.i = (i + 127) << 23;
  return p * b.f;
}

// e^x
inline float fexp(float x) { return fexp2(x * 1.44269504f); }

// log2(x) for x > 0. Absolute error < 2e-5.
inline float flog2(float x) {
  FloatBits b;
  b.f = x;
  const float e = (float)((int32_t)((b.u >> 23) & 0xFF) - 127);
  b.u = (b.u & 0x007FFFFFu) | 0x3F800000u;  // mantissa in [1, 2)
  const float t = b.f - 1.f;  // 0..1
  const float p = 1.43541059e-05f + t * (1.44159271f + t * (-0.707256362f + t * (0.411566809f + t * (-0.189836406f + t * 0.0439295708f))));
  return e + p;
}

// a^b for a > 0
inline float fpow(float a, float b) { return fexp2(b * flog2(a)); }

inline float dbToGain(float db) { return fexp2(db * 0.16609640f); }     // 10^(db/20)
inline float gainToDb(float g) { return 6.0205999f * flog2(maxf(g, 1e-9f)); }

// sin(x) for any x (radians). Absolute error < 1e-5.
inline float fsin(float x) {
  // Wrap to [-pi, pi]
  x -= kTwoPi * (float)floorToInt(x * (1.f / kTwoPi) + 0.5f);
  // Fold to [-pi/2, pi/2]
  if (x > kHalfPi) x = kPi - x;
  else if (x < -kHalfPi) x = -kPi - x;
  const float x2 = x * x;
  return x * (0.99999660f - x2 * (0.16664824f - x2 * (0.00830629f - x2 * 0.00018363f)));
}
inline float fcos(float x) { return fsin(x + kHalfPi); }

// sin(2*pi*t) where t is a phase in cycles (turns). Handy for oscillators.
inline float sinTurns(float t) { return fsin(t * kTwoPi); }

// tan(x) for 0 <= x < ~1.5 (Pade approximant). Used for filter coefficients.
inline float ftan(float x) {
  const float x2 = x * x;
  return x * (945.f - 105.f * x2 + x2 * x2) / (945.f - 420.f * x2 + 15.f * x2 * x2);
}

// Smooth saturators. Both have unity gain near zero.
// Cubic: hard ceiling of +/-1 at |x| >= 1.5. Cheap, gentle.
inline float softclip(float x) {
  if (x > 1.5f) return 1.f;
  if (x < -1.5f) return -1.f;
  return x - (4.f / 27.f) * x * x * x;
}
// tanh approximation (within 0.004 of tanh), saturates at +/-1 beyond |x| = 3.
inline float ftanh(float x) {
  if (x > 3.f) return 1.f;
  if (x < -3.f) return -1.f;
  const float x2 = x * x;
  return x * (27.f + x2) / (27.f + 9.f * x2);
}

// Output safety: transparent below 0.8, then rounds off smoothly toward 1.0.
inline float softLimit(float x) {
  const float a = absf(x);
  if (a <= 0.8f) return x;
  const float y = 0.8f + 0.2f * ftanh((a - 0.8f) * 5.f);
  return x < 0.f ? -y : y;
}

// Square root. On the NTS-3 this is the single VSQRT instruction. Never call
// sqrtf(): it links libm's errno wrapper, which drags in abort/unwind code
// the NTS-3 can't resolve ("ELF error: Resolve Symbol" in the editor).
inline float fsqrt(float x) {
  if (x <= 0.f) return 0.f;
#if defined(__arm__) && defined(__ARM_FP)
  float r;
  __asm__("vsqrt.f32 %0, %1" : "=t"(r) : "t"(x));
  return r;
#else
  return __builtin_sqrtf(x);
#endif
}

// Equal-power crossfade. mix 0 = all dry, 1 = all wet.
inline void equalPowerGains(float mix, float &dry, float &wet) {
  wet = fsqrt(clampf(mix, 0.f, 1.f));
  dry = fsqrt(1.f - clampf(mix, 0.f, 1.f));
}

// Map 0..1 exponentially between lo and hi (both > 0): for times and frequencies,
// so equal knob travel gives equal musical change.
inline float expMap(float norm, float lo, float hi) { return lo * fexp2(clampf(norm, 0.f, 1.f) * flog2(hi / lo)); }
inline float linMap(float norm, float lo, float hi) { return lo + (hi - lo) * clampf(norm, 0.f, 1.f); }

inline float msToSamples(float ms) { return ms * (kSampleRate * 0.001f); }

// 1 - e^(-x) for x >= 0, accurate even for tiny x (where 1 - fexp() would
// lose most of its precision).
inline float oneMinusExpNeg(float x) {
  if (x < 0.05f) return x * (1.f - x * (0.5f - x * (0.16666667f - x * 0.04166667f)));
  return 1.f - fexp(-x);
}

// One-pole coefficient for a time constant in ms (time to reach ~63%).
inline float timeCoef(float ms) {
  if (ms <= 0.f) return 1.f;
  return oneMinusExpNeg(1.f / msToSamples(ms));
}

// One-pole coefficient for a cutoff in Hz.
inline float cutoffCoef(float hz) { return oneMinusExpNeg(kTwoPi * hz * kSampleTime); }

// Kill denormals in long feedback loops.
inline float undenormal(float x) { return (absf(x) < 1e-15f) ? 0.f : x; }

}  // namespace hfh
