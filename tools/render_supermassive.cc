// Desktop listening test for Supermassive.
#include <functional>
#include "supermassive_dsp.h"
#include "demo_signals.h"

using namespace demo;

static std::vector<float> g_mem(sm::Supermassive::kBufferFloats);
static sm::Supermassive g_fx;

// Runs the effect in 64-frame blocks; `ctl(t)` can move parameters over time.
static Buf run(const Buf &in, const std::function<void(sm::Supermassive &)> &setup,
               const std::function<void(sm::Supermassive &, float)> &ctl = nullptr) {
  g_fx.init(g_mem.data());
  setup(g_fx);
  Buf out(in.size());
  const uint32_t frames = in.size() / 2;
  for (uint32_t f = 0; f < frames; f += 64) {
    if (ctl) ctl(g_fx, (float)f / kFs);
    g_fx.process(&in[2 * f], &out[2 * f], std::min<uint32_t>(64, frames - f));
  }
  return out;
}

int main(int argc, char **argv) {
  std::string dir = argc > 1 ? argv[1] : ".";

  Buf bells = silence(24.f);
  addBell(bells, 0.3f, 69, 0.35f);
  addBell(bells, 1.5f, 76, 0.3f);
  addBell(bells, 2.7f, 72, 0.3f);
  addBell(bells, 4.6f, 81, 0.25f);
  {
    Buf o = run(bells, [](sm::Supermassive &f) { f.setSize(0.75f); f.setDecay(20.f); f.setMix(0.5f); f.setTone(5500.f); });
    writeWav(dir + "/supermassive_bells_cathedral.wav", o);
    report("supermassive bells cathedral", o);
  }
  {
    Buf pad = silence(30.f);
    addPad(pad, 0.2f, 7.f, {57, 64, 69, 76}, 0.5f);
    Buf o = run(pad, [](sm::Supermassive &f) { f.setSize(1.f); f.setDecay(60.f); f.setMix(0.6f); f.setShimmer(0.5f); f.setTone(6000.f); });
    writeWav(dir + "/supermassive_pad_shimmer.wav", o);
    report("supermassive pad shimmer 60 s", o);
  }
  {
    // Dancing on the pad: X sweeps size back and forth (WARP 400 ms bends the
    // tail), then Y climbs to the top and freezes, holds, and lets go.
    Buf in = silence(32.f);
    addBell(in, 0.3f, 64, 0.35f);
    addBell(in, 1.1f, 71, 0.3f);
    addBell(in, 1.9f, 76, 0.3f);
    addPad(in, 2.5f, 5.f, {52, 59, 64}, 0.35f);
    addBell(in, 14.f, 83, 0.3f);   // played over the frozen tail
    Buf o = run(in, [](sm::Supermassive &f) { f.setSize(0.4f); f.setDecay(15.f); f.setMix(0.55f); f.setWarp(400.f); f.setModulation(0.7f); },
                [](sm::Supermassive &f, float t) {
                  if (t > 3.f && t < 11.f) f.setSize(0.5f + 0.5f * hfh::fsin(t * 1.3f));
                  if (t >= 9.f && t < 10.f) f.setDecay(15.f + 80.f * (t - 9.f));
                  if (t >= 10.f && t < 10.1f) { f.setFreeze(true); f.setDecay(100.f); }
                  if (t >= 22.f && t < 22.1f) { f.setFreeze(false); f.setDecay(8.f); }
                });
    writeWav(dir + "/supermassive_dance_and_freeze.wav", o);
    report("supermassive dance + freeze", o);
  }
  // Stress: loud noise burst, largest size, max shimmer, then freeze for 20 s.
  {
    Buf in = silence(30.f);
    for (int i = 0; i < kFs; ++i) in[2 * i] = in[2 * i + 1] = ((rand() / (float)RAND_MAX) * 2.f - 1.f) * 0.9f;
    Buf o = run(in, [](sm::Supermassive &f) { f.setSize(1.f); f.setDecay(99.f); f.setMix(1.f); f.setShimmer(1.f); f.setModulation(1.f); },
                [](sm::Supermassive &f, float t) { if (t > 5.f) f.setFreeze(true); });
    report("supermassive stress freeze+shimmer", o);
  }
  {
    Buf in = silence(20.f);
    for (int i = 0; i < kFs; ++i) in[2 * i] = in[2 * i + 1] = ((rand() / (float)RAND_MAX) * 2.f - 1.f) * 0.9f;
    Buf o = run(in, [](sm::Supermassive &f) { f.setSize(0.f); f.setDecay(1.f); f.setMix(1.f); f.setWarp(20.f); },
                [](sm::Supermassive &f, float t) { f.setSize(t - (float)(int)t); });   // size jumps every second
    report("supermassive stress size jumps", o);
  }
  return 0;
}
