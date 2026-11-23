# HFH core

The shared DSP library behind every Hi-Fi Hopes NTS-3 effect. Header-only C++11 with no SDK or libm dependencies, so the same code runs on the NTS-3 and on a desktop for testing. Including everything costs a few KB of the unit's ~32 KB budget.

```cpp
#include "hfh.h"   // or individual headers
```

## Modules

| Header | What's in it |
|---|---|
| `hfh_math.h` | Fast `fexp2/fexp/flog2/fpow/fsin/ftan/ftanh`, dB conversion, `expMap` for times and frequencies, equal-power crossfade, `softLimit` output safety, coefficient helpers |
| `hfh_smooth.h` | `Smoother` (one-pole) and `Ramp` (linear, arrives on time) |
| `hfh_filter.h` | `OnePoleLP/HP`, `DCBlocker`, `Svf` (zero-delay-feedback state-variable filter: LP/HP/BP/notch/peak), `Svf24` (24 dB/oct) |
| `hfh_delay.h` | `DelayLine` over external memory; linear or Hermite-spline reads; absolute reads for loopers and grains |
| `hfh_mod.h` | `Rng`, `Drift` (smooth random wander), `Lfo`, `Ramper` (bounce / rise / fall, smooth / linear / square) |
| `hfh_grain.h` | `windowHann`, `windowTukey`, `ArEnv` |
| `hfh_reverb.h` | `Haze`: lush 8-line modulated FDN reverb with diffusion, damping, size glide and freeze; decay is accurate in seconds; ~280 KB of SDRAM |
| `hfh_character.h` | The "factory mods": `Stability` (wow, flutter, hiss, darkening), `Clock` (sample rate as a sound) + `quantizeClock` (musical steps), `TapeSat` |

## House rules for every effect

1. **Sound first.** Build and tune the DSP on the desktop (`tools/render_demo.cc`) before touching the pad mapping.
2. **Tune against a reference.** Each effect names a real reference (e.g. the SR-202W for Spring Tank) and is compared against recordings of it.
3. **Nothing clicks.** Every continuous parameter goes through a `Smoother` or `Ramp`.
4. **Real units.** Parameters display ms, s, Hz, % or semitones, never raw numbers. Times and frequencies use `expMap`.
5. **Only sweet spots.** Ranges are limited to what sounds good; no dead or broken zones on the pad.
6. **Level-matched.** Turning the effect up should change its character, not jump the volume.
7. **Alive.** Use `Drift` and small modulation so sustained sounds never sit static.
8. **Quick on top, deep underneath.** X, Y and Depth carry the performance controls; everything else lives in the editor.
9. **Dreamy by default.** `Stability`, `Clock` and `TapeSat` are standard options, not afterthoughts.
10. **Within the device's limits.** Check code size and processor load and trade features for stability when needed.

## Tests

```
g++ -O2 -std=c++17 -I core/hfh tools/test_core.cc -o test_core && ./test_core
```

CI also compiles every module with the NTS-3's exact compiler flags (`tools/arm_smoke.cc`).

## Using it in a unit

In the unit's `config.mk`:

```
UINCDIR = ../hfh-core
```

CI copies `core/hfh` to `logue-sdk/platform/nts-3_kaoss/hfh-core` before building.
