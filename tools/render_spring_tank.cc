// Desktop listening test for Spring Tank.
// Runs the exact DSP used on the NTS-3 and writes 48 kHz stereo WAV files.
//
//   g++ -O2 -std=c++17 -I core/hfh -I tools -I spring-tank tools/render_spring_tank.cc -o render_spring_tank
//   ./render_spring_tank out_dir
//
#include "spring_dsp.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

static const int kFs = 48000;

static void writeWav(const std::string &path, const std::vector<float> &lr) {
  FILE *f = fopen(path.c_str(), "wb");
  if (!f) { perror(path.c_str()); exit(1); }
  const uint32_t frames = lr.size() / 2;
  const uint32_t dataBytes = frames * 4;
  auto w32 = [&](uint32_t v) { fwrite(&v, 4, 1, f); };
  auto w16 = [&](uint16_t v) { fwrite(&v, 2, 1, f); };
  fwrite("RIFF", 1, 4, f); w32(36 + dataBytes); fwrite("WAVE", 1, 4, f);
  fwrite("fmt ", 1, 4, f); w32(16); w16(1); w16(2); w32(kFs); w32(kFs * 4); w16(4); w16(16);
  fwrite("data", 1, 4, f); w32(dataBytes);
  for (float s : lr) {
    float c = s > 1.f ? 1.f : (s < -1.f ? -1.f : s);
    int16_t v = (int16_t)lrintf(c * 32767.f);
    fwrite(&v, 2, 1, f);
  }
  fclose(f);
}

// --- test signals (mono, duplicated to stereo) ---

static std::vector<float> silence(float sec) { return std::vector<float>((size_t)(sec * kFs), 0.f); }

static void addClap(std::vector<float> &m, float atSec, float gain) {
  uint32_t r = 99;
  size_t start = (size_t)(atSec * kFs);
  for (size_t i = 0; i < (size_t)(0.03f * kFs) && start + i < m.size(); ++i) {
    r = r * 1664525u + 1013904223u;
    float n = ((r >> 9) / 8388608.f) * 2.f - 1.f;
    m[start + i] += gain * n * expf(-(float)i / (0.006f * kFs));
  }
}

// Two-operator FM pluck, roughly the kind of patch the FM-1 makes.
static void addFmNote(std::vector<float> &m, float atSec, float durSec, float hz, float gain) {
  size_t start = (size_t)(atSec * kFs);
  size_t len = (size_t)(durSec * kFs);
  for (size_t i = 0; i < len && start + i < m.size(); ++i) {
    float t = (float)i / kFs;
    float env = expf(-t * 3.5f) * (1.f - expf(-t * 400.f));
    float idx = 3.0f * expf(-t * 6.f);
    float mod = sinf(2.f * st::kPi * hz * 2.f * t);
    m[start + i] += gain * env * sinf(2.f * st::kPi * hz * t + idx * mod);
  }
}

static std::vector<float> toStereo(const std::vector<float> &m) {
  std::vector<float> s(m.size() * 2);
  for (size_t i = 0; i < m.size(); ++i) s[2 * i] = s[2 * i + 1] = m[i];
  return s;
}

struct Settings {
  int character = 0;
  float dwell = 0.4f, mix = 0.5f, decay = 0.45f, tone = 0.5f, drip = 0.55f;
  int springs = 3;
};

// Process in 64-frame blocks like the device. crashAt < 0 = no crash.
static std::vector<float> run(const std::vector<float> &inStereo, const Settings &st_,
                              float crashAtSec = -1.f) {
  static std::vector<float> mem(st::SpringTank::kBufferFloats);
  static st::SpringTank tank;
  tank.init(mem.data());
  tank.setCharacter(st_.character);
  tank.setDwell(st_.dwell);
  tank.setMix(st_.mix);
  tank.setDecay(st_.decay);
  tank.setTone(st_.tone);
  tank.setDrip(st_.drip);
  tank.setSprings(st_.springs);
  tank.reset();

  std::vector<float> out(inStereo.size());
  const uint32_t block = 64;
  const uint32_t frames = inStereo.size() / 2;
  size_t crashFrame = crashAtSec < 0 ? (size_t)-1 : (size_t)(crashAtSec * kFs);
  for (uint32_t f = 0; f < frames; f += block) {
    uint32_t nb = std::min(block, frames - f);
    if (crashFrame != (size_t)-1 && f <= crashFrame && crashFrame < f + nb) tank.triggerCrash();
    tank.process(&inStereo[2 * f], &out[2 * f], nb);
  }
  return out;
}

static void report(const char *name, const std::vector<float> &v) {
  float peak = 0.f; double sum = 0; bool bad = false;
  for (float s : v) {
    if (!std::isfinite(s)) bad = true;
    peak = std::max(peak, fabsf(s));
    sum += (double)s * s;
  }
  printf("%-34s peak %.3f  rms %.4f  %s\n", name, peak, sqrt(sum / v.size()), bad ? "NaN/Inf!" : "ok");
}

int main(int argc, char **argv) {
  std::string dir = argc > 1 ? argv[1] : ".";
  const char *charName[2] = {"sr202w", "surf"};

  for (int c = 0; c < 2; ++c) {
    // 1. Clap: the classic spring test.
    {
      auto m = silence(4.f);
      addClap(m, 0.1f, 0.8f);
      Settings s; s.character = c; s.mix = 0.6f;
      auto o = run(toStereo(m), s);
      std::string n = dir + "/1_clap_" + charName[c] + ".wav";
      writeWav(n, o); report(n.c_str(), o);
    }
    // 2. FM notes, dwell low then high.
    {
      auto m = silence(9.f);
      const float notes[] = {220.f, 261.6f, 329.6f, 392.f};
      for (int i = 0; i < 4; ++i) addFmNote(m, 0.2f + i * 0.45f, 1.2f, notes[i], 0.35f);
      for (int i = 0; i < 4; ++i) addFmNote(m, 4.6f + i * 0.45f, 1.2f, notes[i], 0.35f);
      auto in = toStereo(m);
      Settings lo; lo.character = c; lo.dwell = 0.15f; lo.mix = 0.5f;
      Settings hi = lo; hi.dwell = 0.9f;
      auto a = run(in, lo);
      auto b = run(in, hi);
      std::vector<float> o(in.size());
      size_t split = (size_t)(4.4f * kFs) * 2;
      for (size_t i = 0; i < o.size(); ++i) o[i] = i < split ? a[i] : b[i];
      std::string n = dir + "/2_fm_dwell_low_then_high_" + charName[c] + ".wav";
      writeWav(n, o); report(n.c_str(), o);
    }
    // 3. Crash: bump the tank, no input.
    {
      auto m = silence(4.f);
      Settings s; s.character = c; s.mix = 0.6f;
      auto o = run(toStereo(m), s, 0.1f);
      std::string n = dir + "/3_crash_" + charName[c] + ".wav";
      writeWav(n, o); report(n.c_str(), o);
    }
  }

  // 4. Stress test: max dwell, max decay, max drip, loud input. Must stay finite.
  {
    auto m = silence(10.f);
    for (int i = 0; i < 20; ++i) addFmNote(m, i * 0.25f, 1.f, 110.f * (1 + i % 5), 0.9f);
    for (int c = 0; c < 2; ++c) {
      Settings s; s.character = c; s.dwell = 1.f; s.decay = 1.f; s.drip = 1.f; s.mix = 1.f; s.tone = 1.f;
      auto o = run(toStereo(m), s, 2.0f);
      std::string n = dir + "/stress_" + std::string(charName[c]) + ".wav";
      report(n.c_str(), o);
    }
  }
  return 0;
}
