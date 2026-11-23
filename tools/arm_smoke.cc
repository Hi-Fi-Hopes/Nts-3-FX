// Compiles every HFH core module with the NTS-3 compiler flags (CI check).
#include "hfh.h"
// Instantiate everything so the compiler checks and sizes it all.
using namespace hfh;
static float mem[Stability::kBufferFloats + 4096];
float smoke(const float* in, float* out, unsigned n) {
  static Smoother s; static Ramp r; static Svf f; static Svf24 f4; static OnePoleLP lp; static OnePoleHP hp; static DCBlocker dc;
  static DelayLine d; static Drift dr; static Lfo l; static Ramper rp; static ArEnv env; static TapeSat ts; static Clock c; static Stability st;
  static bool init = false;
  if (!init) { d.init(mem, 4096); st.init(mem + 4096); init = true; }
  f.set(expMap(in[0], 20.f, 20000.f), 0.7f); f4.set(1000.f, 1.f); c.setRate(quantizeClock(in[1], 4, 12), true); ts.setDrive(4.f); st.setAmount(0.5f);
  float acc = 0;
  for (unsigned i = 0; i < n; ++i) {
    float x = in[i];
    x = f.process(x, Svf::kLowPass) + f4.process(x, Svf::kHighPass) + lp.process(x) + hp.process(x) + dc.process(x);
    d.write(x); x += d.readHermite(100.5f + l.next() * dr.next()) * s.next() * r.next() * rp.next() * env.next();
    x = c.process(ts.process(x)) * windowHann(0.3f) * windowTukey(0.2f, 0.5f);
    float L = x, R = x; st.process(L, R); out[i] = L + R + fpow(2.f, in[i]) + gainToDb(x) + ftanh(x) + fsin(x);
    acc += out[i];
  }
  return acc;
}
