// Desktop tests for the HFH core library.
//   g++ -O2 -std=c++17 -I core/hfh tools/test_core.cc -o test_core && ./test_core
#include "hfh.h"

#include <cmath>
#include <cstdio>
#include <vector>

using namespace hfh;

static int g_fail = 0, g_pass = 0;
#define CHECK(cond, ...)                                   \
  do {                                                     \
    if (cond) { ++g_pass; }                                \
    else { ++g_fail; printf("FAIL %s:%d  ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } \
  } while (0)

static const float fs = kSampleRate;

// Steady-state gain of a filter at a frequency, by running a sine through it.
template <class F>
static float gainAt(F &&proc, float hz) {
  double in = 0, out = 0;
  const int n = (int)fs;
  for (int i = 0; i < n; ++i) {
    float x = sinf(2.f * (float)M_PI * hz * i / fs);
    float y = proc(x);
    if (i > n / 2) { in += x * x; out += y * y; }
  }
  return (float)std::sqrt(out / in);
}

static void testMath() {
  float worst = 0;
  for (float x = -20.f; x < 20.f; x += 0.01f) worst = std::max(worst, std::fabs(fexp2(x) / std::exp2(x) - 1.f));
  CHECK(worst < 1e-4f, "fexp2 rel err %g", worst);

  worst = 0;
  for (float x = 1e-4f; x < 1e4f; x *= 1.01f) worst = std::max(worst, std::fabs(flog2(x) - std::log2(x)));
  CHECK(worst < 2e-4f, "flog2 abs err %g", worst);

  worst = 0;
  for (float x = -50.f; x < 50.f; x += 0.003f) worst = std::max(worst, std::fabs(fsin(x) - std::sin(x)));
  CHECK(worst < 2e-5f, "fsin abs err %g", worst);

  worst = 0;
  for (float x = 0.001f; x < 1.42f; x += 0.001f) worst = std::max(worst, std::fabs(ftan(x) / std::tan(x + 1e-9f) - 1.f));
  CHECK(worst < 1e-3f, "ftan rel err %g (to 0.45 fs)", worst);

  CHECK(std::fabs(dbToGain(-6.0206f) - 0.5f) < 1e-3f, "dbToGain");
  CHECK(std::fabs(gainToDb(0.5f) + 6.0206f) < 0.01f, "gainToDb");
  CHECK(std::fabs(expMap(0.5f, 20.f, 20000.f) - 632.46f) < 1.f, "expMap midpoint %f", expMap(0.5f, 20.f, 20000.f));
  worst = 0;
  for (float x = -4.f; x < 4.f; x += 0.01f) worst = std::max(worst, std::fabs(ftanh(x) - std::tanh(x)));
  CHECK(worst < 0.025f, "ftanh abs err %g", worst);
  CHECK(ftanh(10.f) == 1.f && softclip(-10.f) == -1.f, "saturators bounded");

  float d, w;
  equalPowerGains(0.5f, d, w);
  CHECK(std::fabs(d * d + w * w - 1.f) < 1e-5f, "equal power");
}

static void testSmooth() {
  Smoother s(10.f);
  s.snap(0.f);
  s.setTarget(1.f);
  float v = 0;
  for (int i = 0; i < (int)msToSamples(10.f); ++i) v = s.next();
  CHECK(std::fabs(v - 0.632f) < 0.01f, "smoother 63%% at tau: %f", v);

  Smoother b(10.f);
  b.snap(0.f);
  b.setTarget(1.f);
  float vb = b.nextBlock((uint32_t)msToSamples(10.f));
  CHECK(std::fabs(vb - 0.632f) < 0.01f, "block smoother %f", vb);

  Ramp r(10.f);
  r.snap(0.f);
  r.setTarget(1.f);
  int n = 0;
  while (r.busy()) { r.next(); ++n; }
  CHECK(std::abs(n - 480) <= 1 && r.value() == 1.f, "ramp arrives on time (%d samples)", n);
}

static void testFilters() {
  Svf f;
  f.set(1000.f, 0.7071f);
  float g = gainAt([&](float x) { return f.process(x, Svf::kLowPass); }, 1000.f);
  CHECK(std::fabs(20 * std::log10(g) + 3.01f) < 0.3f, "SVF LP -3 dB at cutoff: %.2f dB", 20 * std::log10(g));
  f.reset();
  g = gainAt([&](float x) { return f.process(x, Svf::kLowPass); }, 100.f);
  CHECK(std::fabs(g - 1.f) < 0.01f, "SVF LP passband %f", g);
  f.reset();
  g = gainAt([&](float x) { return f.process(x, Svf::kHighPass); }, 100.f);
  CHECK(g < 0.02f, "SVF HP stopband %f", g);

  Svf24 f4;
  f4.set(1000.f, 0.7071f);
  g = gainAt([&](float x) { return f4.process(x, Svf::kLowPass); }, 1000.f);
  CHECK(std::fabs(20 * std::log10(g) + 3.01f) < 0.5f, "SVF24 Butterworth -3 dB: %.2f dB", 20 * std::log10(g));
  f4.reset();
  g = gainAt([&](float x) { return f4.process(x, Svf::kLowPass); }, 4000.f);
  CHECK(20 * std::log10(g) < -45.f, "SVF24 slope: %.1f dB two octaves up", 20 * std::log10(g));

  // Fast cutoff sweep at high resonance must stay finite and bounded.
  Svf s;
  Rng rng;
  float peak = 0;
  for (int i = 0; i < 48000 * 2; ++i) {
    s.set(expMap(0.5f + 0.5f * std::sin(i * 0.01f), 20.f, 20000.f), 15.f);
    float y = s.process(rng.bipolar() * 0.5f, Svf::kLowPass);
    peak = std::max(peak, std::fabs(y));
    if (!std::isfinite(y)) { peak = 1e9; break; }
  }
  CHECK(peak < 50.f, "SVF stable under fast resonant sweep (peak %.2f)", peak);

  OnePoleLP lp;
  lp.setCutoff(1000.f);
  g = gainAt([&](float x) { return lp.process(x); }, 1000.f);
  CHECK(std::fabs(20 * std::log10(g) + 3.f) < 0.5f, "one-pole -3 dB: %.2f", 20 * std::log10(g));

  DCBlocker dc;
  float y = 0;
  for (int i = 0; i < 48000; ++i) y = dc.process(1.f);
  CHECK(std::fabs(y) < 0.01f, "DC blocker removes DC (%f)", y);
}

static void testDelay() {
  std::vector<float> mem(1024);
  DelayLine d;
  d.init(mem.data(), 1024);
  for (int i = 0; i < 100; ++i) d.write((float)i);
  CHECK(d.at(0) == 99.f && d.at(10) == 89.f, "delay integer read");
  CHECK(std::fabs(d.readLinear(10.5f) - 88.5f) < 1e-4f, "delay linear read");
  CHECK(std::fabs(d.readHermite(10.25f) - 88.75f) < 1e-4f, "hermite exact on a ramp");
  // Hermite on a sine should be much closer than linear.
  DelayLine s;
  s.init(mem.data(), 1024);
  const float hz = 3000.f;
  for (int i = 0; i < 1000; ++i) s.write(std::sin(2 * M_PI * hz * i / fs));
  double eL = 0, eH = 0;
  for (float dd = 5.f; dd < 500.f; dd += 0.37f) {
    float truth = std::sin(2 * M_PI * hz * (999 - dd) / fs);
    eL = std::max(eL, (double)std::fabs(s.readLinear(dd) - truth));
    eH = std::max(eH, (double)std::fabs(s.readHermite(dd) - truth));
  }
  CHECK(eH < eL * 0.5, "hermite beats linear (%.4f vs %.4f)", eH, eL);
}

static void testMod() {
  Rng r;
  float lo = 1, hi = -1;
  for (int i = 0; i < 100000; ++i) { float v = r.bipolar(); lo = std::min(lo, v); hi = std::max(hi, v); }
  CHECK(lo >= -1.f && hi <= 1.f && lo < -0.99f && hi > 0.99f, "rng range %f..%f", lo, hi);

  Drift dr(5);
  dr.setRate(2.f);
  float maxStep = 0, prev = dr.next(), mx = 0;
  for (int i = 0; i < 48000 * 5; ++i) { float v = dr.next(); maxStep = std::max(maxStep, std::fabs(v - prev)); prev = v; mx = std::max(mx, std::fabs(v)); }
  CHECK(mx <= 1.f && maxStep < 0.001f, "drift smooth and bounded (step %g)", maxStep);

  Lfo l;
  l.setRate(5.f);
  int crossings = 0;
  float p = l.next();
  for (int i = 1; i < 48000; ++i) { float v = l.next(); if (p < 0 && v >= 0) ++crossings; p = v; }
  CHECK(crossings == 5 || crossings == 4, "lfo rate (%d up-crossings in 1 s)", crossings);

  Ramper rp;
  rp.setMode(Ramper::kBounce);
  rp.setShape(Ramper::kLinear);
  rp.setTime(0.5f);
  float top = 0;
  int topAt = 0;
  for (int i = 0; i < 48000; ++i) { float v = rp.next(); if (v > top) { top = v; topAt = i; } }
  CHECK(top > 0.999f && std::abs(topAt - 24000) < 50, "bounce reaches top at 0.5 s (%d)", topAt);
  Ramper rf;
  rf.setMode(Ramper::kFall);
  rf.setTime(0.1f);
  float v = 1;
  for (int i = 0; i < 9600; ++i) v = rf.next();
  CHECK(v < 1e-4f, "fall ramps to 0 and holds (%f)", v);
}

static void testGrain() {
  CHECK(windowHann(0.f) < 1e-6f && std::fabs(windowHann(0.5f) - 1.f) < 1e-4f && windowHann(1.f) < 1e-6f, "hann shape");
  // Overlapping Hann windows at 50% sum to 1 (no amplitude ripple in clouds).
  float worst = 0;
  for (float p = 0; p < 0.5f; p += 0.01f) worst = std::max(worst, std::fabs(windowHann(p) + windowHann(p + 0.5f) - 1.f));
  CHECK(worst < 1e-4f, "hann overlap-add flat (%g)", worst);
  CHECK(windowTukey(0.5f, 0.3f) == 1.f && windowTukey(0.f, 0.3f) < 1e-6f, "tukey shape");
}

static void testCharacter() {
  CHECK(std::fabs(quantizeClock(1.f, 4, 12) - 1.f) < 1e-4f, "clock top = 1");
  CHECK(std::fabs(quantizeClock(0.f, 4, 1) - 1.f / 16.f) < 1e-4f, "clock bottom 4 oct = 1/16");
  CHECK(std::fabs(quantizeClock(0.76f, 4, 1) - 0.5f) < 1e-3f, "clock snaps to octaves");

  TapeSat t;
  t.setDrive(1.f);
  float g = gainAt([&](float x) { return t.process(x * 0.01f) * 100.f; }, 1000.f);
  CHECK(std::fabs(g - 1.f) < 0.03f, "tape sat unity at low level (%f)", g);
  t.setDrive(8.f);
  float peak = 0;
  for (int i = 0; i < 48000; ++i) peak = std::max(peak, std::fabs(t.process(3.f * std::sin(i * 0.05f))));
  CHECK(peak < 0.6f, "tape sat compresses hot input (peak %f)", peak);

  Clock c;
  c.setRate(0.25f, true);
  g = gainAt([&](float x) { return c.process(x); }, 12000.f);
  CHECK(g < 0.1f, "clock 1/4 removes highs above its Nyquist (%f)", g);
  c.setRate(0.25f, true);
  g = gainAt([&](float x) { return c.process(x); }, 500.f);
  CHECK(g > 0.85f, "clock 1/4 keeps lows (%f)", g);

  std::vector<float> mem(Stability::kBufferFloats);
  Stability st;
  st.init(mem.data());
  st.setAmount(1.f);
  float mx = 0;
  bool finite = true;
  for (int i = 0; i < 48000 * 3; ++i) {
    float l = std::sin(i * 0.02f) * 0.5f, r = l;
    st.process(l, r);
    finite = finite && std::isfinite(l) && std::isfinite(r);
    mx = std::max(mx, std::fabs(l));
  }
  CHECK(finite && mx < 0.7f && mx > 0.3f, "stability full: stable, level kept (%f)", mx);
  // At 0 it should be (nearly) transparent after the smoother settles.
  Stability st0;
  st0.init(mem.data());
  st0.setAmount(0.f);
  double err = 0;
  for (int i = 0; i < 48000; ++i) {
    float x = std::sin(i * 0.02f) * 0.5f, l = x, r = x;
    st0.process(l, r);
    if (i > 24000) err = std::max(err, (double)std::fabs(l - x));
  }
  CHECK(err < 0.01, "stability 0 is transparent (%g)", err);
}

int main() {
  testMath();
  testSmooth();
  testFilters();
  testDelay();
  testMod();
  testGrain();
  testCharacter();
  printf("%d passed, %d failed\n", g_pass, g_fail);
  return g_fail ? 1 : 0;
}
