#pragma once
/*
 *  Glass Bells — NTS-3 parameter glue.
 *  Touching the pad strikes a bell; X (NOTE) and Y (TONE) arrive as parameters.
 */

#include "processor.h"
#include "unit_genericfx.h"
#include "bells_dsp.h"

class Effect : public Processor {
 public:
  enum { NOTE = 0U, TONE, LEVEL, KEY, SCALE, RING, SPACE, REPEAT, NUM_PARAMS };

  uint32_t getBufferSize() const override final { return gb::GlassBells::kBufferFloats; }
  void init(float *buf) override final { fx_.init(buf); }
  void process(const float *__restrict in, float *__restrict out, uint32_t frames) override final { fx_.process(in, out, frames); }

  void setParameter(uint8_t index, int32_t value) override final {
    const float pct = value * 0.001f;
    switch (index) {
      case NOTE: fx_.setNoteParam(value); break;
      case TONE: fx_.setTone(pct); break;
      case LEVEL: fx_.setLevel(pct); break;
      case KEY: fx_.setKey(value); break;
      case SCALE: fx_.setScale(value); break;
      case RING: fx_.setRing(value * 0.1f); break;       // 5..300 -> 0.5..30.0 s
      case SPACE: fx_.setSpace(pct); break;
      case REPEAT: fx_.setRepeat(value * 0.1f); break;   // 0..120 -> 0..12.0 Hz
      default: break;
    }
  }

  const char *getParameterStrValue(uint8_t index, int32_t value) const override final {
    static char noteBuf[8];
    switch (index) {
      case NOTE: return fx_.noteName(value, noteBuf);
      case KEY: return gb::GlassBells::keyName(value);
      case SCALE: return gb::GlassBells::scaleName(value);
      default: return nullptr;
    }
  }

  void touchEvent(uint8_t id, uint8_t phase, uint32_t x, uint32_t y) override final {
    (void)id; (void)x; (void)y;
    switch (phase) {
      case k_unit_touch_phase_began: fx_.touchBegan(); break;
      case k_unit_touch_phase_ended:
      case k_unit_touch_phase_cancelled: fx_.touchEnded(); break;
      default: break;
    }
  }

 private:
  gb::GlassBells fx_;
};
