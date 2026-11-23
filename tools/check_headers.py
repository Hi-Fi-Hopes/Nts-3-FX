#!/usr/bin/env python3
"""Validate every unit's header.c against the rules the KORG KONTROL Editor
enforces, so a bad header fails CI instead of failing on the editor.

Checks (from the logue SDK NTS-3 README):
  - unit name: <= 19 chars, valid character set
  - param name: <= 21 chars, valid character set
  - min <= max; center within [min, max] (center = min for unipolar);
    default within [min, max]
  - default mappings: min/max/value inside the parameter's range
"""
import glob
import re
import sys

VALID = set(" ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_")
PARAM = re.compile(r"\{\s*(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*(k_unit_param_type_\w+),[^{]*\{\"([^\"]*)\"\}\s*\}")
MAPPING = re.compile(r"\{\s*(k_genericfx_param_assign_\w+),\s*k_genericfx_curve_\w+,\s*k_genericfx_curve_\w+,\s*(-?\d+),\s*(-?\d+),\s*(-?\d+)\s*\}")
NAME = re.compile(r"\.name\s*=\s*\"([^\"]*)\"")

errors = []
for path in sorted(glob.glob("*/header.c")):
    src = open(path).read()
    m = NAME.search(src)
    if not m:
        errors.append(f"{path}: no .name")
    else:
        name = m.group(1)
        if len(name) > 19 or not set(name) <= VALID:
            errors.append(f"{path}: unit name {name!r} too long or has invalid characters")
    params = PARAM.findall(src)
    if len(params) != 8:
        errors.append(f"{path}: expected 8 parameter descriptors, found {len(params)}")
    for i, (mn, mx, ctr, dflt, ptype, pname) in enumerate(params):
        mn, mx, ctr, dflt = int(mn), int(mx), int(ctr), int(dflt)
        tag = f"{path}: param {i} {pname!r}"
        if not pname and ptype == "k_unit_param_type_none":
            continue  # unused slot
        if mn > mx:
            errors.append(f"{tag}: min > max")
        if not mn <= ctr <= mx:
            errors.append(f"{tag}: center {ctr} outside [{mn}, {mx}] (use min for unipolar)")
        if not mn <= dflt <= mx:
            errors.append(f"{tag}: default {dflt} outside [{mn}, {mx}]")
        if len(pname) > 21 or not set(pname) <= VALID:
            errors.append(f"{tag}: name too long or has invalid characters")
    maps = MAPPING.findall(src)
    if len(maps) != 8:
        errors.append(f"{path}: expected 8 default mappings, found {len(maps)}")
    for i, (assign, mn, mx, val) in enumerate(maps):
        if i >= len(params):
            break
        pmn, pmx = int(params[i][0]), int(params[i][1])
        for label, v in (("min", mn), ("max", mx), ("value", val)):
            if not pmn <= int(v) <= pmx:
                errors.append(f"{path}: mapping {i} {label} {v} outside param range [{pmn}, {pmx}]")
    print(f"{path}: {len(params)} params, {len(maps)} mappings checked")

if errors:
    print("\n".join("ERROR " + e for e in errors))
    sys.exit(1)
print("all headers OK")
