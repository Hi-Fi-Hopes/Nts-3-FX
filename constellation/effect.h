#pragma once
/*
 *  Constellation — NTS-3 parameter glue.
 */

#include "processor.h"
#include "unit_genericfx.h"
#include "constellation_dsp.h"

class Effect : public Processor {
 public:
  enum { DENSITY = 0U, HEIGHT, SPARKLE, SIZE, SCATTER, REVERSE, SPACE, TONE, NUM_PARAMS };

  uint32_t getBufferSize() const override final { return constellation::Constellation::kBufferFloats; }
  void init(float *buf) override final { fx_.init(buf); }
  void process(const float *__restrict in, float *__restrict out, uint32_t frames) override final { fx_.process(in, out, frames); }

  void setParameter(uint8_t index, int32_t value) override final {
    const float pct = value * 0.001f;
    switch (index) {
      case DENSITY: fx_.setDensity(value * 0.1f); break;   // 5..400 -> 0.5..40.0 per second
      case HEIGHT: fx_.setHeightZone(value / 100); break;  // 0..499, 100 per pitch set
      case SPARKLE: fx_.setLevel(pct); break;
      case SIZE: fx_.setSize((float)value); break;         // ms
      case SCATTER: fx_.setScatter((float)value); break;   // ms
      case REVERSE: fx_.setReverse(pct); break;
      case SPACE: fx_.setSpace(pct); break;
      case TONE: fx_.setTone((float)value); break;         // Hz
      default: break;
    }
  }

  const char *getParameterStrValue(uint8_t index, int32_t value) const override final {
    if (index == HEIGHT) return constellation::Constellation::heightName(value / 100);
    return nullptr;
  }

 private:
  constellation::Constellation fx_;
};
