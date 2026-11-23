// Desktop listening test for Memory Fog.
#include "fog_dsp.h"
#include "demo_signals.h"

using namespace demo;

struct Set { float timeMs = 450.f, age = 0.4f, fb = 0.5f, mix = 0.45f, spread = 0.5f, warble = 0.35f, hiss = 0.3f, tone = 0.5f; };

static Buf run(const Buf &in, const Set &s) {
  static std::vector<float> mem(fog::MemoryFog::kBufferFloats);
  static fog::MemoryFog fx;
  fx.init(mem.data());
  fx.setTime(s.timeMs); fx.setAge(s.age); fx.setFeedback(s.fb); fx.setMix(s.mix);
  fx.setSpread(s.spread); fx.setWarble(s.warble); fx.setHiss(s.hiss); fx.setTone(s.tone);
  Buf out(in.size());
  const uint32_t frames = in.size() / 2;
  for (uint32_t f = 0; f < frames; f += 64) fx.process(&in[2 * f], &out[2 * f], std::min<uint32_t>(64, frames - f));
  return out;
}

int main(int argc, char **argv) {
  std::string dir = argc > 1 ? argv[1] : ".";
  // 1. Three bell notes, then silence so the repeats can age.
  {
    Buf in = silence(10.f);
    addBell(in, 0.2f, 76, 0.4f, 1.5f);
    addBell(in, 0.9f, 79, 0.35f, 1.5f);
    addBell(in, 1.6f, 83, 0.3f, 1.5f);
    Set s; s.timeMs = 520.f; s.fb = 0.75f; s.age = 0.55f;
    Buf o = run(in, s);
    writeWav(dir + "/fog_bells.wav", o);
    report("fog bells", o);
  }
  // 2. Pad with heavy age and warble: repeats dissolve into fog.
  {
    Buf in = silence(14.f);
    addPad(in, 0.2f, 4.f, {60, 64, 67, 71}, 0.5f);
    Set s; s.timeMs = 800.f; s.fb = 0.85f; s.age = 0.85f; s.warble = 0.7f; s.hiss = 0.6f; s.mix = 0.55f;
    Buf o = run(in, s);
    writeWav(dir + "/fog_pad_deep.wav", o);
    report("fog pad deep", o);
  }
  // 3. Each repeat should be darker than the one before.
  {
    Buf in = silence(4.f);
    for (int i = 0; i < 480; ++i) in[2 * i] = in[2 * i + 1] = ((rand() / (float)RAND_MAX) * 2.f - 1.f) * 0.5f;
    Set s; s.timeMs = 500.f; s.fb = 0.8f; s.age = 0.6f; s.mix = 1.f; s.warble = 0.f; s.hiss = 0.f; s.spread = 0.f;
    Buf o = run(in, s);
    // Each repeat should be quieter and have less of its energy above ~2 kHz.
    printf("repeats (level dB / share above 2 kHz dB):");
    for (int k = 1; k <= 5; ++k) {
      size_t a = (size_t)(k * 0.5f * kFs) - 1200, b = a + 4800;
      double lo = 0, hi = 0, lp = 0;
      for (size_t i = a; i < b; ++i) { double x = o[2 * i]; lp += 0.23 * (x - lp); lo += lp * lp; hi += (x - lp) * (x - lp); }
      printf("  #%d %.1f/%.1f", k, 10 * std::log10((lo + hi) / 4800 + 1e-20), 10 * std::log10(hi / (lo + hi) + 1e-20));
    }
    printf("\n");
  }
  // 4. Stress: max feedback, max age, loud input. Must stay bounded.
  {
    Buf in = silence(12.f);
    for (int i = 0; i < 48000 * 2; ++i) in[2 * i] = in[2 * i + 1] = ((rand() / (float)RAND_MAX) * 2.f - 1.f) * 0.9f;
    Set s; s.timeMs = 60.f; s.fb = 1.f; s.age = 1.f; s.warble = 1.f; s.hiss = 1.f; s.mix = 1.f;
    report("fog stress", run(in, s));
  }
  return 0;
}
