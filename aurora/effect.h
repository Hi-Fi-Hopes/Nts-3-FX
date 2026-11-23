#pragma once
/*
 *  Aurora — NTS-3 parameter glue.
 */

#include "processor.h"
#include "unit_genericfx.h"
#include "aurora_dsp.h"

class Effect : public Processor {
 public:
  enum { CHORD = 0U, BLOOM, DECAY, MIX, TONE, MOD, SIZE, STABILITY, NUM_PARAMS };

  uint32_t getBufferSize() const override final { return aurora::Aurora::kBufferFloats; }
  void init(float *buf) override final { fx_.init(buf); }
  void process(const float *__restrict in, float *__restrict out, uint32_t frames) override final { fx_.process(in, out, frames); }

  void setParameter(uint8_t index, int32_t value) override final {
    const float pct = value * 0.001f;
    switch (index) {
      case CHORD: fx_.setChordPosition(value); break;     // 0..799, 100 per chord
      case BLOOM: fx_.setBloom(pct); break;
      case DECAY: fx_.setDecay(value * 0.1f); break;      // 10..300 -> 1.0..30.0 s
      case MIX: fx_.setMix(pct); break;
      case TONE: fx_.setTone((float)value); break;        // Hz
      case MOD: fx_.setModulation(pct); break;
      case SIZE: fx_.setSize(pct); break;
      case STABILITY: fx_.setStability(pct); break;
      default: break;
    }
  }

  const char *getParameterStrValue(uint8_t index, int32_t value) const override final {
    if (index == CHORD) return aurora::Aurora::chordName(value);
    return nullptr;
  }

 private:
  aurora::Aurora fx_;
};
