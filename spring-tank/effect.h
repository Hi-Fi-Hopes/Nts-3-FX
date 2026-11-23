#pragma once
/*
 *  Spring Tank — NTS-3 parameter glue.
 *  Maps the SDK's integer parameters and touch events onto st::SpringTank.
 */

#include "processor.h"
#include "unit_genericfx.h"
#include "spring_dsp.h"

class Effect : public Processor {
 public:
  enum {
    DWELL = 0U,  // X
    MIX,         // Y
    DECAY,       // Depth
    TONE,
    DRIP,
    SPRINGS,
    CHARACTER,
    CRASH,
    NUM_PARAMS
  };

  uint32_t getBufferSize() const override final { return st::SpringTank::kBufferFloats; }

  void init(float *allocated_buffer) override final { tank_.init(allocated_buffer); }

  void process(const float *__restrict in, float *__restrict out, uint32_t frames) override final {
    tank_.process(in, out, frames);
  }

  void setParameter(uint8_t index, int32_t value) override final {
    const float pct = value * 0.001f;  // 0..1000 -> 0..1
    switch (index) {
      case DWELL: tank_.setDwell(pct); break;
      case MIX: tank_.setMix(pct); break;
      case DECAY: tank_.setDecay(pct); break;
      case TONE: tank_.setTone(pct); break;
      case DRIP: tank_.setDrip(pct); break;
      case SPRINGS: tank_.setSprings(value); break;
      case CHARACTER: tank_.setCharacter(value); break;
      case CRASH: tank_.setCrashEnabled(value != 0); break;
      default: break;
    }
  }

  const char *getParameterStrValue(uint8_t index, int32_t value) const override final {
    static const char *kCharNames[] = {"SR-202W", "SURF"};
    if (index == CHARACTER && value >= 0 && value < 2) return kCharNames[value];
    return nullptr;
  }

  void touchEvent(uint8_t id, uint8_t phase, uint32_t x, uint32_t y) override final {
    (void)id; (void)x; (void)y;
    switch (phase) {
      case k_unit_touch_phase_began: tank_.touchBegan(); break;
      case k_unit_touch_phase_ended: tank_.touchEnded(); break;
      case k_unit_touch_phase_cancelled: tank_.touchCancelled(); break;
      default: break;
    }
  }

  void reset() override final { tank_.reset(); }

 private:
  st::SpringTank tank_;
};
