// Desktop listening test for Sympathetic Strings.
#include "strings_dsp.h"
#include "demo_signals.h"

using namespace demo;

static Buf run(const Buf &in, int key, int scale, std::vector<std::pair<float, int>> chords,
               float ring, float mix, float drift = 0.25f, float stability = 0.f) {
  static std::vector<float> mem(sym::SympatheticStrings::kBufferFloats);
  static sym::SympatheticStrings fx;
  fx.init(mem.data());
  fx.setKey(key); fx.setScale(scale); fx.setRing(ring); fx.setMix(mix);
  fx.setTone(5000.f); fx.setDrift(drift); fx.setStability(stability);
  fx.setChord(chords[0].second);
  Buf out(in.size());
  const uint32_t frames = in.size() / 2;
  size_t ci = 0;
  for (uint32_t f = 0; f < frames; f += 64) {
    const float t = (float)f / kFs;
    while (ci + 1 < chords.size() && t >= chords[ci + 1].first) fx.setChord(chords[++ci].second);
    uint32_t nb = std::min<uint32_t>(64, frames - f);
    fx.process(&in[2 * f], &out[2 * f], nb);
  }
  return out;
}

int main(int argc, char **argv) {
  std::string dir = argc > 1 ? argv[1] : ".";

  // 1. Bell melody in C major over I maj9 -> IV maj9 -> vi m9.
  {
    Buf in = silence(16.f);
    const float mel[] = {72, 76, 79, 83, 77, 81, 84, 88, 81, 84, 88, 86};
    for (int i = 0; i < 12; ++i) addBell(in, 0.3f + i * 1.1f, mel[i], 0.3f);
    Buf wet = run(in, 0, 0, {{0.f, 0}, {4.6f, 3}, {9.0f, 5}}, 8.f, 0.55f);
    writeWav(dir + "/strings_bells_C_major.wav", concat(in, wet));
    report("strings bells (dry then wet)", wet);
  }
  // 2. Pad in A minor, strings ringing long, with a little tape warp.
  {
    Buf in = silence(20.f);
    addPad(in, 0.2f, 8.f, {57, 60, 64, 67}, 0.5f);
    addPad(in, 8.5f, 8.f, {53, 57, 60, 64}, 0.5f);
    Buf wet = run(in, 9, 1, {{0.f, 0}, {8.3f, 3}}, 14.f, 0.6f, 0.4f, 0.3f);
    writeWav(dir + "/strings_pad_A_minor.wav", wet);
    report("strings pad A minor", wet);
  }
  // 3. Out-of-key notes should barely ring the strings.
  {
    Buf in = silence(6.f);
    addBell(in, 0.2f, 73, 0.3f);  // C# into C major strings
    addBell(in, 3.0f, 72, 0.3f);  // C, in key
    Buf wet = run(in, 0, 0, {{0.f, 0}}, 8.f, 1.f, 0.f);
    double outKey = 0, inKey = 0;
    for (size_t i = (size_t)(1.5f * kFs); i < (size_t)(2.9f * kFs); ++i) outKey += wet[2 * i] * wet[2 * i];
    for (size_t i = (size_t)(4.3f * kFs); i < (size_t)(5.7f * kFs); ++i) inKey += wet[2 * i] * wet[2 * i];
    printf("in-key vs out-of-key ring: %.1f dB\n", 10 * std::log10(inKey / (outKey + 1e-20)));
  }
  // 4. Stress: loud noise, max ring.
  {
    Buf in = silence(8.f);
    for (size_t i = 0; i < in.size(); ++i) in[i] = ((rand() / (float)RAND_MAX) * 2.f - 1.f) * 0.9f;
    Buf wet = run(in, 0, 0, {{0.f, 0}, {2.f, 6}, {4.f, 2}}, 30.f, 1.f, 1.f, 1.f);
    report("strings stress", wet);
  }
  return 0;
}
