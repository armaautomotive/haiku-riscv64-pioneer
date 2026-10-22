#!/bin/sh
# ALL refusals BEFORE SDK/output access;NO native access.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-ffn-persistent-gates.XXXXXXXX")
cases=0
refused() {
 cases=$((cases+1));status=0
 env "$@" sh "$directory/$script" "$temporary/MUST_NOT_OPEN_SDK" "$temporary/forbidden-$cases" > "$temporary/refusal-$cases.log" 2>&1 || status=$?
 [ "$status" = 2 ] && [ ! -e "$temporary/forbidden-$cases" ]
 if rg -F "$temporary/MUST_NOT_OPEN_SDK" "$temporary/refusal-$cases.log" > /dev/null;then exit 1;fi
}
script=build_qwen38_ffn_persistent_image.sh
for value in 0 01 -1 invalid;do refused "PIONEER_Q38_FFN_PERSISTENT=$value";done
for key in PIONEER_Q38_FFN_RESIDENT_MATRIX PIONEER_Q38_WORK_LOCAL_Q4 PIONEER_Q38_LOCAL_Q4_QUALIFICATION \
 PIONEER_Q38_WORK_Q4_NIBBLE PIONEER_Q38_WORK_Q4_DIRECT_DECODE PIONEER_Q38_WORK_SIGNED_MAG2 \
 PIONEER_Q38_WORK_PEER_PHASE_PROFILE PIONEER_Q38_WORK_FRONTEND_BARRIER PIONEER_Q38_WORK_RAW_UNPACK_CACHE \
 PIONEER_Q38_WORK_BATCH4_FETCH PIONEER_Q38_WORK_BATCH_RAW_INPUT PIONEER_Q38_WORK_TILE_PREFETCH \
 PIONEER_Q38_PREDECODED_Q4 PIONEER_Q38_NATIVE_Q8 PIONEER_Q38_PREPARED_Q4_BLOCK PIONEER_Q38_PREPARED_Q4_AFFINE \
 PIONEER_Q38_BFLOAT_PANEL PIONEER_Q38_Q4_GROUP32 PIONEER_Q38_FAULT_RETURN PIONEER_Q38_PEER_PROGRESS;do
 for value in 1 01 -1 invalid;do refused PIONEER_Q38_FFN_PERSISTENT=1 "$key=$value";done
done
for script in build_qwen38_persistent_rows_image.sh build_qwen38_dispatch_image.sh build_qwen38_banks_driver.sh build_qwen38_ffn_matrix_image.sh;do
 for value in 1 01 -1 invalid;do refused "PIONEER_Q38_FFN_PERSISTENT=$value";done
done
printf 'PASS %s FFN persistent private/mixed/legacy/native refusals BEFORE SDK/output;NO device access\n' "$cases"
printf 'Offline persistent FFN build-gate evidence retained at %s\n' "$temporary"
