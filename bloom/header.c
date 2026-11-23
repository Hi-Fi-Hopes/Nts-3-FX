/*
 *  Bloom — NTS-3 kaoss pad kit unit header
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
    .unit_id = 0x00000003U,  // Bloom
    .version = 0x00010000U,  // 1.0.0
    .name = "Bloom",
    .num_params = 8,

    .params = {
      // min, max, center, default, type, frac, frac mode, reserved, name
      {20, 4000, 0, 800, k_unit_param_type_msec, 0, 0, 0, {"SWELL"}},
      {0, 1000, 0, 500, k_unit_param_type_percent, 1, 1, 0, {"SPACE"}},
      {5, 200, 0, 60, k_unit_param_type_sec, 1, 1, 0, {"DECAY"}},          // 0.5 .. 20.0 s
      {0, 1000, 0, 500, k_unit_param_type_percent, 1, 1, 0, {"SUSTAIN"}},
      {0, 1000, 0, 600, k_unit_param_type_percent, 1, 1, 0, {"SENS"}},
      {0, 2, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"CURVE"}},
      {1500, 12000, 0, 6000, k_unit_param_type_hertz, 0, 0, 0, {"TONE"}},
      {0, 1000, 0, 0, k_unit_param_type_percent, 1, 1, 0, {"STABILITY"}},
    },
  },
  .default_mappings = {
    // assign, curve, polarity, min, max, value
    {k_genericfx_param_assign_x, k_genericfx_curve_exp, k_genericfx_curve_unipolar, 20, 4000, 800},       // SWELL on X
    {k_genericfx_param_assign_y, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 500},     // SPACE on Y
    {k_genericfx_param_assign_depth, k_genericfx_curve_exp, k_genericfx_curve_unipolar, 5, 200, 60},      // DECAY on Depth
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 500},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 600},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 2, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 1500, 12000, 6000},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 0},
  },
};
