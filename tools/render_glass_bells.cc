// Desktop listening test for Glass Bells: simulates touches on the pad.
#include <functional>
#include "bells_dsp.h"
#include "supermassive_dsp.h"
#include "demo_signals.h"

using namespace demo;

struct Touch { float at, len; int note; float tone; };   // note = NOTE param 0..1000

static std::vector<float> g_mem(gb::GlassBells::kBufferFloats);
static gb::GlassBells g_fx;

static Buf play(float sec, const std::vector<Touch> &touches, const std::function<void(gb::GlassBells &)> &setup) {
  g_fx.init(g_mem.data());
  setup(g_fx);
  Buf in = silence(sec), out(in.size());
  const uint32_t frames = in.size() / 2;
  std::vector<bool> began(touches.size(), false), ended(touches.size(), false);
  for (uint32_t f = 0; f < frames; f += 64) {
    const float t = (float)f / kFs;
    for (size_t k = 0; k < touches.size(); ++k) {
      const Touch &tc = touches[k];
      if (!began[k] && t >= tc.at) { began[k] = true; g_fx.touchBegan(); g_fx.setNoteParam(tc.note); g_fx.setTone(tc.tone); }
      if (began[k] && !ended[k] && t >= tc.at + tc.len) { ended[k] = true; g_fx.touchEnded(); }
    }
    g_fx.process(&in[2 * f], &out[2 * f], std::min<uint32_t>(64, frames - f));
  }
  return out;
}

// A slow melody in A minor pentatonic (zone width ~91 per note).
static std::vector<Touch> melody(float tone) {
  const int seq[] = {4, 6, 7, 9, 7, 6, 4, 2, 4, 1, 2, 4};
  std::vector<Touch> v;
  for (int i = 0; i < 12; ++i) v.push_back({0.3f + 0.9f * i, 0.3f, seq[i] * 91 + 45, tone});
  return v;
}

int main(int argc, char **argv) {
  std::string dir = argc > 1 ? argv[1] : ".";
  {
    Buf o = play(18.f, melody(0.5f), [](gb::GlassBells &b) { b.setKey(9); b.setScale(1); b.setRing(6.f); b.setSpace(0.5f); });
    writeWav(dir + "/glass_bells_melody.wav", o);
    report("glass bells melody (glass)", o);
  }
  {
    // Same note struck with Y moving from bottom to top: celesta -> glass -> sparkle.
    std::vector<Touch> v;
    for (int i = 0; i < 6; ++i) v.push_back({0.3f + 1.6f * i, 0.2f, 5 * 91 + 45, i / 5.f});
    Buf o = play(14.f, v, [](gb::GlassBells &b) { b.setKey(9); b.setScale(1); b.setRing(5.f); b.setSpace(0.35f); });
    writeWav(dir + "/glass_bells_tone_sweep.wav", o);
    report("glass bells tone sweep", o);
  }
  {
    // Hold and slide with REPEAT on: a mallet roll that follows the finger.
    g_fx.init(g_mem.data());
    g_fx.setKey(9); g_fx.setScale(1); g_fx.setRing(4.f); g_fx.setSpace(0.6f); g_fx.setRepeat(7.f);
    Buf in = silence(14.f), o(in.size());
    const uint32_t frames = in.size() / 2;
    for (uint32_t f = 0; f < frames; f += 64) {
      const float t = (float)f / kFs;
      if (f == 64 * 5) { g_fx.setNoteParam(100); g_fx.touchBegan(); }
      if (t > 0.1f && t < 6.f) g_fx.setNoteParam((int)(100 + 800 * t / 6.f));
      if (t >= 6.f && t < 6.01f) g_fx.touchEnded();
      g_fx.process(&in[2 * f], &o[2 * f], std::min<uint32_t>(64, frames - f));
    }
    writeWav(dir + "/glass_bells_roll.wav", o);
    report("glass bells repeat roll", o);
  }
  {
    // Bells into Supermassive: the two units chained.
    Buf bells = play(30.f, melody(0.55f), [](gb::GlassBells &b) { b.setKey(9); b.setScale(1); b.setRing(8.f); b.setSpace(0.2f); });
    static std::vector<float> mem(sm::Supermassive::kBufferFloats);
    static sm::Supermassive verb;
    verb.init(mem.data());
    verb.setSize(0.9f); verb.setDecay(30.f); verb.setMix(0.55f); verb.setShimmer(0.3f);
    Buf o(bells.size());
    const uint32_t frames = bells.size() / 2;
    for (uint32_t f = 0; f < frames; f += 64) verb.process(&bells[2 * f], &o[2 * f], std::min<uint32_t>(64, frames - f));
    writeWav(dir + "/glass_bells_into_supermassive.wav", o);
    report("glass bells -> supermassive", o);
  }
  // Stress: every voice busy, max level and sparkle, fast repeat, hold everything.
  {
    std::vector<Touch> v;
    for (int i = 0; i < 40; ++i) v.push_back({0.05f * i, 0.04f, (i * 137) % 1000, 1.f});
    Buf o = play(8.f, v, [](gb::GlassBells &b) { b.setLevel(1.f); b.setRing(30.f); b.setSpace(1.f); b.setRepeat(12.f); });
    report("glass bells stress", o);
  }
  return 0;
}
