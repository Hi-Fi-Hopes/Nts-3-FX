/*
 *  Spring Tank — NTS-3 kaoss pad kit unit header
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
    .unit_id = 0x00000001U,  // Spring Tank
    .version = 0x00010002U,  // 1.0.2
    .name = "Spring Tank",
    .num_params = 8,

    .params = {
      // min, max, center (= min for unipolar, per Korg), default, type, frac, frac mode, reserved, name
      // Percent parameters run 0..1000 with one decimal: 0.0 .. 100.0 %
      {0, 1000, 0, 400, k_unit_param_type_percent, 1, 1, 0, {"DWELL"}},
      {0, 1000, 0, 500, k_unit_param_type_percent, 1, 1, 0, {"MIX"}},
      {0, 1000, 0, 450, k_unit_param_type_percent, 1, 1, 0, {"DECAY"}},
      {0, 1000, 0, 500, k_unit_param_type_percent, 1, 1, 0, {"TONE"}},
      {0, 1000, 0, 550, k_unit_param_type_percent, 1, 1, 0, {"DRIP"}},
      {2, 3, 2, 3, k_unit_param_type_none, 0, 0, 0, {"SPRINGS"}},
      {0, 1, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"CHARACTER"}},
      {0, 1, 0, 1, k_unit_param_type_onoff, 0, 0, 0, {"CRASH"}},
    },
  },
  .default_mappings = {
    // assign, curve, polarity, min, max, value
    {k_genericfx_param_assign_x, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 400},     // DWELL on X
    {k_genericfx_param_assign_y, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 500},     // MIX on Y
    {k_genericfx_param_assign_depth, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 450}, // DECAY on Depth
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 500},  // TONE
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 550},  // DRIP
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 2, 3, 3},       // SPRINGS
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1, 0},       // CHARACTER
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1, 1},       // CRASH
  },
};
