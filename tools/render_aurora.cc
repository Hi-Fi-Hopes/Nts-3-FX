// Desktop listening test for Aurora.
#include "aurora_dsp.h"
#include "demo_signals.h"

using namespace demo;

static Buf run(const Buf &in, int chord, float bloom, float decay, float mix) {
  static std::vector<float> mem(aurora::Aurora::kBufferFloats);
  static aurora::Aurora fx;
  fx.init(mem.data());
  fx.setChord(chord); fx.setBloom(bloom); fx.setDecay(decay); fx.setMix(mix);
  Buf out(in.size());
  const uint32_t frames = in.size() / 2;
  for (uint32_t f = 0; f < frames; f += 64) fx.process(&in[2 * f], &out[2 * f], std::min<uint32_t>(64, frames - f));
  return out;
}

int main(int argc, char **argv) {
  std::string dir = argc > 1 ? argv[1] : ".";
  Buf bells = silence(14.f);
  addBell(bells, 0.2f, 69, 0.35f);
  addBell(bells, 1.4f, 76, 0.3f);
  addBell(bells, 2.6f, 72, 0.3f);
  {
    Buf o = run(bells, 2, 0.55f, 10.f, 0.55f);   // octave + fifth
    writeWav(dir + "/aurora_bells_oct5th.wav", o);
    report("aurora bells oct+5th", o);
  }
  {
    Buf o = run(bells, 5, 0.6f, 14.f, 0.6f);     // major triad glow
    writeWav(dir + "/aurora_bells_major.wav", o);
    report("aurora bells major", o);
  }
  {
    Buf pad = silence(16.f);
    addPad(pad, 0.2f, 6.f, {57, 64, 69}, 0.5f);
    Buf o = run(pad, 4, 0.5f, 12.f, 0.55f);       // 5th + 9th
    writeWav(dir + "/aurora_pad_5th9th.wav", o);
    report("aurora pad 5th+9th", o);
  }
  // Stress: max bloom, longest decay, every chord.
  for (int c = 0; c < aurora::kChords; ++c) {
    Buf in = silence(10.f);
    for (int i = 0; i < 48000; ++i) in[2 * i] = in[2 * i + 1] = ((rand() / (float)RAND_MAX) * 2.f - 1.f) * 0.9f;
    Buf o = run(in, c, 1.f, 30.f, 1.f);
    char n[64]; snprintf(n, sizeof n, "aurora stress chord %d", c);
    report(n, o);
  }
  return 0;
}
