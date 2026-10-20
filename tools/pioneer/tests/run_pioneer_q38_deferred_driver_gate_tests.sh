#!/bin/sh
# Refusal tests ONLY. No SDK/hardware/output artifacts are touched.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-deferred-driver-gate.XXXXXXXX")
for mode in -1 2 invalid; do
 status=0
 PIONEER_Q38_WORK_DEFER_AFFINE=$mode sh "$directory/../build_qwen38_banks_driver.sh" /nonexistent-sdk "$temporary/refused-$mode" > "$temporary/$mode.log" 2>&1 || status=$?
 [ "$status" = 2 ] && [ ! -e "$temporary/refused-$mode" ]
done
for missing in PIONEER_Q38_PANEL_DRIVER PIONEER_DRIVER_RELOAD PIONEER_Q38_WORK_MODE PIONEER_Q38_NATIVE_OPERAND_STORE PIONEER_Q38_WORK_COMPACT_OUTPUT PIONEER_Q38_NORMAL_CLOCK PIONEER_Q38_DISPATCH_PROFILE; do
 status=0
 # The final override replaces exactly ONE required field, including
 # forbidding a profile/candidate combination. No inherited shell state.
 value=0; [ "$missing" != PIONEER_Q38_DISPATCH_PROFILE ] || value=1
 env PIONEER_Q38_WORK_DEFER_AFFINE=1 PIONEER_Q38_PANEL_DRIVER=10 PIONEER_DRIVER_RELOAD=1 \
  PIONEER_Q38_WORK_MODE=1 PIONEER_Q38_NATIVE_OPERAND_STORE=1 PIONEER_Q38_WORK_COMPACT_OUTPUT=1 \
  PIONEER_Q38_NORMAL_CLOCK=1 PIONEER_Q38_DISPATCH_PROFILE=0 "$missing=$value" \
  sh "$directory/../build_qwen38_banks_driver.sh" /nonexistent-sdk "$temporary/refused-$missing" > "$temporary/$missing.log" 2>&1 || status=$?
 [ "$status" = 2 ] && [ ! -e "$temporary/refused-$missing" ]
 rg -q 'STOP deferred affine requires the complete non-profile compact Work candidate' "$temporary/$missing.log"
done
printf 'PASS deferred-affine driver gate:invalid values/each missing prerequisite/profile mix refuse BEFORE SDK/output access;NO build/native/performance claim: %s\n' "$temporary"
