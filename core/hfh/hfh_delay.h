#pragma once
/*
 *  HFH core — interpolation and delay lines
 *  Memory comes from outside (the NTS-3's SDRAM), so these never allocate.
 *  Interpolation is a sound choice, not just a quality setting:
 *    - Hermite (Catmull-Rom spline): neutral, clean pitch shifting
 *    - Linear: slightly darker, gritty at slow speeds
 */

#include "hfh_math.h"

namespace hfh {

inline float interpLinear(float y0, float y1, float f) { return y0 + (y1 - y0) * f; }

// 4-point Catmull-Rom. f in [0,1) between y0 and y1; ym1 before, y2 after.
inline float interpHermite(float ym1, float y0, float y1, float y2, float f) {
  const float c1 = 0.5f * (y1 - ym1);
  const float c2 = ym1 - 2.5f * y0 + 2.f * y1 - 0.5f * y2;
  const float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);
  return ((c3 * f + c2) * f + c1) * f + y0;
}

// Circular buffer. Size must be a power of two.
class DelayLine {
 public:
  void init(float *buffer, uint32_t sizePow2) {
    buf_ = buffer;
    mask_ = sizePow2 - 1;
    w_ = 0;
    clear();
  }
  void clear() {
    if (!buf_) return;
    for (uint32_t i = 0; i <= mask_; ++i) buf_[i] = 0.f;
  }
  uint32_t size() const { return mask_ + 1; }

  void write(float x) { buf_[w_] = x; w_ = (w_ + 1) & mask_; }

  // Sample written d samples ago (d = 0 is the newest).
  float at(int32_t d) const { return buf_[(w_ - 1 - d) & mask_]; }

  float readLinear(float d) const {
    const int32_t i = floorToInt(d);
    return interpLinear(at(i), at(i + 1), d - (float)i);
  }
  float readHermite(float d) const {
    const int32_t i = floorToInt(d);
    return interpHermite(at(i - 1), at(i), at(i + 1), at(i + 2), d - (float)i);
  }

  // Absolute access for loopers/granular: position in samples from buffer start.
  float *data() { return buf_; }
  uint32_t writeIndex() const { return w_; }
  float readAbsHermite(float pos) const {
    const int32_t i = floorToInt(pos);
    const float f = pos - (float)i;
    return interpHermite(buf_[(i - 1) & mask_], buf_[i & mask_], buf_[(i + 1) & mask_], buf_[(i + 2) & mask_], f);
  }
  float readAbsLinear(float pos) const {
    const int32_t i = floorToInt(pos);
    return interpLinear(buf_[i & mask_], buf_[(i + 1) & mask_], pos - (float)i);
  }

 private:
  float *buf_ = nullptr;
  uint32_t mask_ = 0, w_ = 0;
};

}  // namespace hfh
