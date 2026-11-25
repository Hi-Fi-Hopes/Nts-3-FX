/*
 *  Glass Bells — NTS-3 kaoss pad kit unit header
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
    .unit_id = 0x00000008U,  // Glass Bells
    .version = 0x00010000U,  // 1.0.0
    .name = "Glass Bells",
    .num_params = 8,

    .params = {
      // min, max, center (= min for unipolar, per Korg), default, type, frac, frac mode, reserved, name
      {0, 1000, 0, 500, k_unit_param_type_strings, 0, 0, 0, {"NOTE"}},     // 2 octaves of the scale; shows the note
      {0, 1000, 0, 500, k_unit_param_type_percent, 1, 1, 0, {"TONE"}},     // celesta -> glass -> sparkle
      {0, 1000, 0, 700, k_unit_param_type_percent, 1, 1, 0, {"LEVEL"}},    // bells on top of the input
      {0, 11, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"KEY"}},
      {0, 5, 0, 1, k_unit_param_type_strings, 0, 0, 0, {"SCALE"}},
      {5, 300, 5, 60, k_unit_param_type_sec, 1, 1, 0, {"RING"}},           // 0.5 .. 30.0 s
      {0, 1000, 0, 500, k_unit_param_type_percent, 1, 1, 0, {"SPACE"}},    // reverb amount and length
      {0, 120, 0, 0, k_unit_param_type_hertz, 1, 1, 0, {"REPEAT"}},        // mallet roll while held; 0 = off
    },
  },
  .default_mappings = {
    // assign, curve, polarity, min, max, value
    {k_genericfx_param_assign_x, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 500},     // NOTE on X
    {k_genericfx_param_assign_y, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 500},     // TONE on Y
    {k_genericfx_param_assign_depth, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 700}, // LEVEL on DEPTH
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 11, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 5, 1},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 5, 300, 60},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 500},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 120, 0},
  },
};
