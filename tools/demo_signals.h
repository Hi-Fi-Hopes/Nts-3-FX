#pragma once
// Shared test material for desktop listening renders: a WAV writer, FM bells
// and a soft detuned pad. Everything is 48 kHz stereo interleaved.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace demo {

constexpr int kFs = 48000;
using Buf = std::vector<float>;  // interleaved stereo

inline Buf silence(float sec) { return Buf((size_t)(sec * kFs) * 2, 0.f); }

inline void writeWav(const std::string &path, const Buf &lr) {
  FILE *f = fopen(path.c_str(), "wb");
  if (!f) { perror(path.c_str()); exit(1); }
  const uint32_t frames = lr.size() / 2, dataBytes = frames * 4;
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

inline float midiHz(float n) { return 440.f * std::pow(2.f, (n - 69.f) / 12.f); }

// DX-style bell: modulator at 3.5x, bright decaying index.
inline void addBell(Buf &b, float at, float note, float gain, float len = 4.f) {
  const float hz = midiHz(note);
  const size_t s0 = (size_t)(at * kFs);
  for (size_t i = 0; i < (size_t)(len * kFs) && (s0 + i) * 2 + 1 < b.size(); ++i) {
    float t = (float)i / kFs;
    float env = std::exp(-t * 1.6f) * (1.f - std::exp(-t * 900.f));
    float idx = 2.2f * std::exp(-t * 2.5f);
    float m = std::sin(2.f * (float)M_PI * hz * 3.5f * t);
    float v = gain * env * std::sin(2.f * (float)M_PI * hz * t + idx * m);
    b[(s0 + i) * 2] += v;
    b[(s0 + i) * 2 + 1] += v;
  }
}

// Soft pad chord: detuned saws, gentle low-pass, slow attack/release.
inline void addPad(Buf &b, float at, float len, const std::vector<float> &notes, float gain) {
  const size_t s0 = (size_t)(at * kFs);
  const size_t n = (size_t)(len * kFs);
  std::vector<double> ph(notes.size() * 3, 0.0);
  float lpL = 0, lpR = 0;
  for (size_t i = 0; i < n && (s0 + i) * 2 + 1 < b.size(); ++i) {
    float t = (float)i / kFs;
    float env = std::min(1.f, t / 1.2f) * std::min(1.f, (len - t) / 1.5f);
    float l = 0, r = 0;
    for (size_t k = 0; k < notes.size(); ++k) {
      for (int d = 0; d < 3; ++d) {
        double &p = ph[k * 3 + d];
        p += midiHz(notes[k]) * (1.0 + (d - 1) * 0.004) / kFs;
        p -= std::floor(p);
        float saw = (float)(2.0 * p - 1.0);
        (d == 0 ? l : (d == 2 ? r : l)) += saw * 0.6f;
        if (d == 1) r += saw * 0.6f;
      }
    }
    lpL += 0.04f * (l - lpL);
    lpR += 0.04f * (r - lpR);
    b[(s0 + i) * 2] += gain * env * lpL / notes.size();
    b[(s0 + i) * 2 + 1] += gain * env * lpR / notes.size();
  }
}

inline Buf concat(const Buf &a, const Buf &b) { Buf c(a); c.insert(c.end(), b.begin(), b.end()); return c; }

inline void report(const char *name, const Buf &v) {
  float peak = 0.f; double sum = 0; bool bad = false;
  for (float s : v) {
    if (!std::isfinite(s)) bad = true;
    peak = std::max(peak, std::fabs(s));
    sum += (double)s * s;
  }
  printf("%-44s peak %.3f  rms %.4f  %s\n", name, peak, std::sqrt(sum / v.size()), bad ? "NaN/Inf!" : "ok");
}

}  // namespace demo
