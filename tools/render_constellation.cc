// Desktop listening test for Constellation.
#include "constellation_dsp.h"
#include "demo_signals.h"

using namespace demo;

static Buf run(const Buf &in, float density, float height, float level, float space) {
  static std::vector<float> mem(constellation::Constellation::kBufferFloats);
  static constellation::Constellation fx;
  fx.init(mem.data());
  fx.setDensity(density); fx.setHeight(height); fx.setLevel(level); fx.setSpace(space);
  Buf out(in.size());
  const uint32_t frames = in.size() / 2;
  for (uint32_t f = 0; f < frames; f += 64) fx.process(&in[2 * f], &out[2 * f], std::min<uint32_t>(64, frames - f));
  return out;
}

int main(int argc, char **argv) {
  std::string dir = argc > 1 ? argv[1] : ".";
  Buf bells = silence(12.f);
  const float mel[] = {72, 76, 79, 76, 74, 79, 81, 79};
  for (int i = 0; i < 8; ++i) addBell(bells, 0.2f + i * 1.3f, mel[i], 0.3f, 3.f);
  {
    Buf o = run(bells, 10.f, 0.5f, 0.6f, 0.5f);
    writeWav(dir + "/constellation_bells.wav", concat(bells, o));
    report("constellation bells (dry then wet)", o);
  }
  {
    Buf pad = silence(12.f);
    addPad(pad, 0.2f, 10.f, {60, 64, 67, 71}, 0.5f);
    Buf o = run(pad, 25.f, 0.9f, 0.7f, 0.7f);
    writeWav(dir + "/constellation_pad_dense_high.wav", o);
    report("constellation pad dense high", o);
  }
  {
    Buf in = silence(8.f);
    for (size_t i = 0; i < in.size(); ++i) in[i] = ((rand() / (float)RAND_MAX) * 2.f - 1.f) * 0.9f;
    report("constellation stress", run(in, 40.f, 1.f, 1.f, 1.f));
  }
  return 0;
}
