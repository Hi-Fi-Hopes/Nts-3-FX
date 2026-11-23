/*
 *  Sympathetic Strings — NTS-3 kaoss pad kit unit header
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
    .unit_id = 0x00000002U,  // Sympathetic Strings
    .version = 0x00010003U,  // 1.0.3
    .name = "Sympathetic Strings",
    .num_params = 8,

    .params = {
      // min, max, center (= min for unipolar, per Korg), default, type, frac, frac mode, reserved, name
      {0, 600, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"CHORD"}},       // 100 steps per chord, 7 chords
      {5, 300, 5, 60, k_unit_param_type_sec, 1, 1, 0, {"RING"}},           // 0.5 .. 30.0 s
      {0, 1000, 0, 700, k_unit_param_type_percent, 1, 1, 0, {"STRINGS"}},     // strings level, added to dry
      {0, 11, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"KEY"}},
      {0, 1, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"SCALE"}},
      {800, 12000, 800, 5000, k_unit_param_type_hertz, 0, 0, 0, {"TONE"}},
      {0, 1000, 0, 250, k_unit_param_type_percent, 1, 1, 0, {"DRIFT"}},
      {0, 1000, 0, 0, k_unit_param_type_percent, 1, 1, 0, {"STABILITY"}},
    },
  },
  .default_mappings = {
    // assign, curve, polarity, min, max, value
    {k_genericfx_param_assign_x, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 600, 0},        // CHORD on X
    {k_genericfx_param_assign_y, k_genericfx_curve_exp, k_genericfx_curve_unipolar, 5, 300, 60},          // RING on Y
    {k_genericfx_param_assign_depth, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 700}, // STRINGS level on Depth
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 11, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 800, 12000, 5000},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 250},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 0},
  },
};
