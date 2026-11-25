#pragma once
/*
 *  Supermassive — NTS-3 parameter glue.
 */

#include "processor.h"
#include "unit_genericfx.h"
#include "supermassive_dsp.h"

class Effect : public Processor {
 public:
  enum { SIZE = 0U, DECAY, MIX, TONE, SHIMMER, MOD, WARP, STABILITY, NUM_PARAMS };

  // DECAY at or above this (99.0 s, the top of the pad) freezes the tail.
  static constexpr int32_t kFreezeAt = 990;

  uint32_t getBufferSize() const override final { return sm::Supermassive::kBufferFloats; }
  void init(float *buf) override final { fx_.init(buf); }
  void process(const float *__restrict in, float *__restrict out, uint32_t frames) override final { fx_.process(in, out, frames); }

  void setParameter(uint8_t index, int32_t value) override final {
    const float pct = value * 0.001f;
    switch (index) {
      case SIZE: fx_.setSize(pct); break;
      case DECAY:
        fx_.setFreeze(value >= kFreezeAt);
        fx_.setDecay(value * 0.1f);                // 10..1000 -> 1.0..100.0 s
        break;
      case MIX: fx_.setMix(pct); break;
      case TONE: fx_.setTone((float)value); break;  // Hz
      case SHIMMER: fx_.setShimmer(pct); break;
      case MOD: fx_.setModulation(pct); break;
      case WARP: fx_.setWarp((float)value); break;  // ms
      case STABILITY: fx_.setStability(pct); break;
      default: break;
    }
  }

 private:
  sm::Supermassive fx_;
};
