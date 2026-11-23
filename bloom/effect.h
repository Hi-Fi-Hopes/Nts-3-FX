#pragma once
/*
 *  Bloom — NTS-3 parameter glue.
 */

#include "processor.h"
#include "unit_genericfx.h"
#include "bloom_dsp.h"

class Effect : public Processor {
 public:
  enum { SWELL = 0U, SPACE, DECAY, SUSTAIN, SENS, CURVE, TONE, STABILITY, NUM_PARAMS };

  uint32_t getBufferSize() const override final { return bloom::Bloom::kBufferFloats; }
  void init(float *buf) override final { fx_.init(buf); }
  void process(const float *__restrict in, float *__restrict out, uint32_t frames) override final { fx_.process(in, out, frames); }

  void setParameter(uint8_t index, int32_t value) override final {
    switch (index) {
      case SWELL: fx_.setSwell((float)value); break;                       // ms
      case SPACE: fx_.setSpace(value * 0.001f); break;
      case DECAY: {
        const float sec = value * 0.1f;                                    // 5..200 -> 0.5..20.0 s
        fx_.setDecay(sec);
        fx_.setSize(hfh::clampf((sec - 0.5f) / 8.f, 0.f, 1.f));           // longer tails get a bigger room
        break;
      }
      case SUSTAIN: fx_.setSustain(value * 0.001f); break;
      case SENS: fx_.setSens(value * 0.001f); break;
      case CURVE: fx_.setCurve(value); break;
      case TONE: fx_.setTone((float)value); break;                         // Hz
      case STABILITY: fx_.setStability(value * 0.001f); break;
      default: break;
    }
  }

  const char *getParameterStrValue(uint8_t index, int32_t value) const override final {
    static const char *kCurves[3] = {"SOFT", "LINEAR", "LATE"};
    if (index == CURVE && value >= 0 && value < 3) return kCurves[value];
    return nullptr;
  }

 private:
  bloom::Bloom fx_;
};
