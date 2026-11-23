/*
 *  Memory Fog — NTS-3 kaoss pad kit unit header
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
    .unit_id = 0x00000004U,  // Memory Fog
    .version = 0x00010000U,  // 1.0.0
    .name = "Memory Fog",
    .num_params = 8,

    .params = {
      // min, max, center (= min for unipolar, per Korg), default, type, frac, frac mode, reserved, name
      {20, 1300, 20, 450, k_unit_param_type_msec, 0, 0, 0, {"TIME"}},  // delay time, glides like tape
      {0, 1000, 0, 400, k_unit_param_type_percent, 1, 1, 0, {"AGE"}},  // damage per repeat
      {0, 1000, 0, 500, k_unit_param_type_percent, 1, 1, 0, {"FEEDBACK"}},  // 100% = self-sustaining
      {0, 1000, 0, 450, k_unit_param_type_percent, 1, 1, 0, {"MIX"}},
      {0, 1000, 0, 500, k_unit_param_type_percent, 1, 1, 0, {"SPREAD"}},  // ping-pong
      {0, 1000, 0, 350, k_unit_param_type_percent, 1, 1, 0, {"WARBLE"}},  // wow and flutter
      {0, 1000, 0, 300, k_unit_param_type_percent, 1, 1, 0, {"HISS"}},
      {0, 1000, 0, 500, k_unit_param_type_percent, 1, 1, 0, {"TONE"}},  // darker / brighter start
    },
  },
  .default_mappings = {
    // assign, curve, polarity, min, max, value
    {k_genericfx_param_assign_x, k_genericfx_curve_exp, k_genericfx_curve_unipolar, 20, 1300, 450},  // TIME on X
    {k_genericfx_param_assign_y, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 400},  // AGE on Y
    {k_genericfx_param_assign_depth, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 500},  // FEEDBACK on DEPTH
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 450},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 500},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 350},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 300},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 500},
  },
};
