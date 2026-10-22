#!/bin/sh
# All refusals BEFORE SDK/output,NO card/native access.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-ffn-matrix-gates.XXXXXXXX")
cases=0
refused() {
 cases=$((cases+1));status=0
 env "$@" sh "$directory/$script" "$temporary/MUST_NOT_OPEN_SDK" "$temporary/forbidden-$cases" > "$temporary/refusal-$cases.log" 2>&1 || status=$?
 [ "$status" = 2 ] && [ ! -e "$temporary/forbidden-$cases" ]
 if rg -F "$temporary/MUST_NOT_OPEN_SDK" "$temporary/refusal-$cases.log" > /dev/null;then exit 1;fi
}
script=build_qwen38_ffn_matrix_image.sh
for value in 0 01 -1 invalid;do refused "PIONEER_Q38_FFN_RESIDENT_MATRIX=$value";done
for key in PIONEER_Q38_OUTPUT_TRANSFER_ROOT PIONEER_Q38_WORK_ROOT_COMMAND_REUSE \
 PIONEER_Q38_WORK_BATCH4_FETCH PIONEER_Q38_PEER_PHASE_ROOT PIONEER_Q38_WORK_RAW_UNPACK_CACHE \
 PIONEER_Q38_RAW_UNPACK_CACHE_QUALIFICATION PIONEER_Q38_WORK_LOCAL_Q4 PIONEER_Q38_LOCAL_Q4_QUALIFICATION;do
 for value in 1 01 -1 invalid;do refused PIONEER_Q38_FFN_RESIDENT_MATRIX=1 "$key=$value";done
done
for script in build_qwen38_dispatch_image.sh build_qwen38_banks_driver.sh;do
 for value in 1 01 -1 invalid;do refused "PIONEER_Q38_FFN_RESIDENT_MATRIX=$value";done
done
printf 'PASS %s offline FFN matrix private/legacy/native/mixed-selector refusals BEFORE SDK/output;NO device access\n' "$cases"
printf 'Offline FFN build-gate evidence retained at %s\n' "$temporary"
