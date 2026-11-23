#pragma once
/*
 *  Memory Fog — NTS-3 parameter glue.
 */

#include "processor.h"
#include "unit_genericfx.h"
#include "fog_dsp.h"

class Effect : public Processor {
 public:
  enum { TIME = 0U, AGE, FEEDBACK, MIX, SPREAD, WARBLE, HISS, TONE, NUM_PARAMS };

  uint32_t getBufferSize() const override final { return fog::MemoryFog::kBufferFloats; }
  void init(float *buf) override final { fx_.init(buf); }
  void process(const float *__restrict in, float *__restrict out, uint32_t frames) override final { fx_.process(in, out, frames); }

  void setParameter(uint8_t index, int32_t value) override final {
    const float pct = value * 0.001f;
    switch (index) {
      case TIME: fx_.setTime((float)value); break;   // ms
      case AGE: fx_.setAge(pct); break;
      case FEEDBACK: fx_.setFeedback(pct); break;
      case MIX: fx_.setMix(pct); break;
      case SPREAD: fx_.setSpread(pct); break;
      case WARBLE: fx_.setWarble(pct); break;
      case HISS: fx_.setHiss(pct); break;
      case TONE: fx_.setTone(pct); break;
      default: break;
    }
  }

 private:
  fog::MemoryFog fx_;
};
