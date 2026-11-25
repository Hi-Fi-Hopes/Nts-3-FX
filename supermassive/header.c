/*
 *  Supermassive — NTS-3 kaoss pad kit unit header
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
    .unit_id = 0x00000007U,  // Supermassive
    .version = 0x00010000U,  // 1.0.0
    .name = "Supermassive",
    .num_params = 8,

    .params = {
      // min, max, center (= min for unipolar, per Korg), default, type, frac, frac mode, reserved, name
      {0, 1000, 0, 600, k_unit_param_type_percent, 1, 1, 0, {"SIZE"}},     // chamber -> supermassive
      {10, 1000, 10, 120, k_unit_param_type_sec, 1, 1, 0, {"DECAY"}},       // 1.0 .. 99 s; top = freeze
      {0, 1000, 0, 500, k_unit_param_type_percent, 1, 1, 0, {"MIX"}},
      {800, 10000, 800, 5000, k_unit_param_type_hertz, 0, 0, 0, {"TONE"}},
      {0, 1000, 0, 0, k_unit_param_type_percent, 1, 1, 0, {"SHIMMER"}},     // octave-up bloom
      {0, 1000, 0, 500, k_unit_param_type_percent, 1, 1, 0, {"MOD"}},       // tail movement
      {20, 5000, 20, 600, k_unit_param_type_msec, 0, 0, 0, {"WARP"}},       // size glide time
      {0, 1000, 0, 0, k_unit_param_type_percent, 1, 1, 0, {"STABILITY"}},   // tape wow, flutter, hiss
    },
  },
  .default_mappings = {
    // assign, curve, polarity, min, max, value
    {k_genericfx_param_assign_x, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 600},    // SIZE on X
    {k_genericfx_param_assign_y, k_genericfx_curve_exp, k_genericfx_curve_unipolar, 10, 1000, 120},     // DECAY on Y
    {k_genericfx_param_assign_depth, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 500},  // MIX on DEPTH
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 800, 10000, 5000},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 500},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 20, 5000, 600},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1000, 0},
  },
};
