/*
 *  Constellation — NTS-3 kaoss pad kit unit header
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
    .unit_id = 0x00000006U,  // Constellation
    .version = 0x00010000U,  // 1.0.0
    .name = "Constellation",
    .num_params = 8,

    .params = {
      // min, max, center (= min for unipolar, per Korg), default, type, frac, frac mode, reserved, name
      {5, 400, 5, 80, k_unit_param_type_none, 1, 1, 0, {"DENSITY"}},  // 0.5 .. 40.0 grains per second
      {0, 499, 0, 200, k_unit_param_type_strings, 0, 0, 0, {"HEIGHT"}},  // 5 pitch sets, 100 steps each
      {0, 1000, 0, 600, k_unit_param_type_percent, 1, 1, 0, {"SPARKLE"}},  // added on top of dry
      {20, 300, 20, 90, k_unit_param_type_msec, 0, 0, 0, {"SIZE"}},  // grain length
      {30, 1200, 30, 600, k_unit_param_type_msec, 0, 0, 0, {"SCATTER"}},  // how far back grains reach
      {0, 1000, 0, 150, k_unit_param_type_percent, 1, 1, 0, {"REVERSE"}},  // share of grains played backwards
      {0, 1000, 0, 500, k_unit_param_type_percent, 1, 1, 0, {"SPACE"}},  // reverb on the sparkles
      {2000, 16000, 2000, 9000, k_unit_param_type_hertz, 0, 0, 0, {"TONE"}},
    },
  },
  .default_mappings = {
    // assign, curve, polarity, min, max, value
    {k_genericfx_param_assign_x, k_genericfx_curve_exp, k_genericfx_curve_unipolar, 5, 400, 80},  // DENSITY on X
    {k_genericfx_param_assign_y, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 499, 200},  // HEIGHT on Y
    {k_genericfx_param_assign_depth, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 600},  // SPARKLE on DEPTH
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 20, 300, 90},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 30, 1200, 600},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 150},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 500},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 2000, 16000, 9000},
  },
};
