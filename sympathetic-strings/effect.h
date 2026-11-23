#pragma once
/*
 *  Sympathetic Strings — NTS-3 parameter glue.
 */

#include "processor.h"
#include "unit_genericfx.h"
#include "strings_dsp.h"

class Effect : public Processor {
 public:
  enum { CHORD = 0U, RING, LEVEL, KEY, SCALE, TONE, DRIFT, STABILITY, NUM_PARAMS };

  uint32_t getBufferSize() const override final { return sym::SympatheticStrings::kBufferFloats; }
  void init(float *buf) override final { fx_.init(buf); }
  void process(const float *__restrict in, float *__restrict out, uint32_t frames) override final { fx_.process(in, out, frames); }

  void setParameter(uint8_t index, int32_t value) override final {
    switch (index) {
      case CHORD: fx_.setChordPosition(value); break;      // 0..600, 100 per chord
      case RING: fx_.setRing(value * 0.1f); break;          // 5..300 -> 0.5..30.0 s
      case LEVEL: fx_.setLevel(value * 0.001f); break;      // 0..1000 -> 0..1, added to dry
      case KEY: fx_.setKey(value); break;
      case SCALE: fx_.setScale(value); break;
      case TONE: fx_.setTone((float)value); break;          // Hz
      case DRIFT: fx_.setDrift(value * 0.001f); break;
      case STABILITY: fx_.setStability(value * 0.001f); break;
      default: break;
    }
  }

  const char *getParameterStrValue(uint8_t index, int32_t value) const override final {
    static const char *kKeys[12] = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
    static const char *kScales[2] = {"MAJOR", "MINOR"};
    switch (index) {
      case CHORD: return sym::chordName(fx_.scale(), value / 100 > 6 ? 6 : value / 100);
      case KEY: return (value >= 0 && value < 12) ? kKeys[value] : nullptr;
      case SCALE: return (value >= 0 && value < 2) ? kScales[value] : nullptr;
      default: return nullptr;
    }
  }

 private:
  sym::SympatheticStrings fx_;
};
