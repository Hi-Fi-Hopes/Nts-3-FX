// Desktop listening test for Bloom.
#include "bloom_dsp.h"
#include "demo_signals.h"

using namespace demo;

struct Set { float swellMs = 800.f, space = 0.5f, decay = 6.f, sens = 0.6f; int curve = 0; float tone = 6000.f, size = 0.7f, stab = 0.f, sustain = 0.5f; };

static Buf run(const Buf &in, const Set &s) {
  static std::vector<float> mem(bloom::Bloom::kBufferFloats);
  static bloom::Bloom fx;
  fx.init(mem.data());
  fx.setSwell(s.swellMs); fx.setSpace(s.space); fx.setDecay(s.decay); fx.setSens(s.sens);
  fx.setCurve(s.curve); fx.setTone(s.tone); fx.setSize(s.size); fx.setStability(s.stab); fx.setSustain(s.sustain);
  Buf out(in.size());
  const uint32_t frames = in.size() / 2;
  for (uint32_t f = 0; f < frames; f += 64) fx.process(&in[2 * f], &out[2 * f], std::min<uint32_t>(64, frames - f));
  return out;
}

int main(int argc, char **argv) {
  std::string dir = argc > 1 ? argv[1] : ".";

  // 1. Bell melody: dry, then bloomed ("bowed glass").
  Buf bells = silence(12.f);
  const float mel[] = {76, 79, 83, 81, 79, 76, 74, 76};
  for (int i = 0; i < 8; ++i) addBell(bells, 0.3f + i * 1.4f, mel[i], 0.35f);
  {
    Set s; s.swellMs = 700.f; s.space = 0.55f; s.decay = 7.f;
    Buf wet = run(bells, s);
    writeWav(dir + "/bloom_bells.wav", concat(bells, wet));
    report("bloom bells (dry then wet)", wet);
  }
  // 2. Same bells, long late swell into a big frozen-feeling space with tape warp.
  {
    Set s; s.swellMs = 2000.f; s.space = 0.75f; s.decay = 15.f; s.curve = 2; s.size = 1.f; s.stab = 0.45f; s.sustain = 0.9f;
    Buf wet = run(bells, s);
    writeWav(dir + "/bloom_bells_deep_warped.wav", wet);
    report("bloom bells deep + warped", wet);
  }
  // 3. Level check: swell finished, no reverb -> should match the dry level.
  {
    Buf tone = silence(4.f);
    for (size_t i = 0; i < tone.size() / 2; ++i) tone[2 * i] = tone[2 * i + 1] = 0.3f * std::sin(2 * M_PI * 330 * i / kFs);
    Set s; s.swellMs = 200.f; s.space = 0.f; s.sustain = 0.f;
    Buf wet = run(tone, s);
    double a = 0, b = 0;
    for (size_t i = kFs * 2; i < (size_t)kFs * 4; ++i) { a += tone[2 * i] * tone[2 * i]; b += wet[2 * i] * wet[2 * i]; }
    printf("bloom settled level vs dry: %.2f dB\n", 10 * std::log10(b / a));
    // Attack check: first 50 ms of a note should be much quieter than dry.
    a = b = 0;
    for (size_t i = 0; i < 2400; ++i) { a += tone[2 * i] * tone[2 * i]; b += wet[2 * i] * wet[2 * i]; }
    printf("bloom first 50 ms vs dry: %.1f dB\n", 10 * std::log10(b / a + 1e-20));
  }
  // 4. Stress
  {
    Buf in = silence(8.f);
    for (size_t i = 0; i < in.size(); ++i) in[i] = ((rand() / (float)RAND_MAX) * 2.f - 1.f) * 0.9f;
    Set s; s.swellMs = 20.f; s.space = 1.f; s.decay = 20.f; s.sens = 1.f; s.size = 1.f; s.stab = 1.f;
    report("bloom stress", run(in, s));
  }
  return 0;
}
