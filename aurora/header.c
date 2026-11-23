/*
 *  Aurora — NTS-3 kaoss pad kit unit header
 *  Hi-Fi Hopes
 *
 *  Based on the KORG logue SDK genericfx template (BSD 3-Clause, (c) 2023 KORG INC.).
 */

#include "unit_genericfx.h"

const __unit_header genericfx_unit_header_t unit_header = {
  .common = {
    .header_size = sizeof(genericfx_unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_genericfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x48464850U,   // 'HFHP' — Hi-Fi Hopes
    .unit_id = 0x00000005U,  // Aurora
    .version = 0x00010000U,  // 1.0.0
    .name = "Aurora",
    .num_params = 8,

    .params = {
      // min, max, center (= min for unipolar, per Korg), default, type, frac, frac mode, reserved, name
      {0, 799, 0, 200, k_unit_param_type_strings, 0, 0, 0, {"CHORD"}},  // 8 chords, 100 steps each
      {0, 1000, 0, 400, k_unit_param_type_percent, 1, 1, 0, {"BLOOM"}},  // shimmer amount
      {10, 300, 10, 80, k_unit_param_type_sec, 1, 1, 0, {"DECAY"}},  // 1.0 .. 30.0 s
      {0, 1000, 0, 500, k_unit_param_type_percent, 1, 1, 0, {"MIX"}},
      {1500, 14000, 1500, 7000, k_unit_param_type_hertz, 0, 0, 0, {"TONE"}},
      {0, 1000, 0, 500, k_unit_param_type_percent, 1, 1, 0, {"MOD"}},  // tail movement
      {0, 1000, 0, 800, k_unit_param_type_percent, 1, 1, 0, {"SIZE"}},
      {0, 1000, 0, 0, k_unit_param_type_percent, 1, 1, 0, {"STABILITY"}},  // tape wow, flutter, hiss
    },
  },
  .default_mappings = {
    // assign, curve, polarity, min, max, value
    {k_genericfx_param_assign_x, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 799, 200},  // CHORD on X
    {k_genericfx_param_assign_y, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 400},  // BLOOM on Y
    {k_genericfx_param_assign_depth, k_genericfx_curve_exp, k_genericfx_curve_unipolar, 10, 300, 80},  // DECAY on DEPTH
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 500},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 1500, 14000, 7000},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 500},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 800},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 0},
  },
};
