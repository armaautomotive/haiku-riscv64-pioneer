#!/bin/sh
# ALL private/mixed/legacy/native refusals BEFORE SDK/output;NO hardware.
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-ffn-graph-gates.XXXXXXXX")
cases=0
refused() {
 cases=$((cases+1));status=0
 if [ "$script" = build_qwen38_local_image.sh ];then
  env "$@" sh "$directory/$script" ffn_resident_gather2 "$temporary/MUST_NOT_OPEN_SDK" "$temporary/forbidden-$cases" > "$temporary/refusal-$cases.log" 2>&1 || status=$?
 else
  env "$@" sh "$directory/$script" "$temporary/MUST_NOT_OPEN_SDK" "$temporary/forbidden-$cases" > "$temporary/refusal-$cases.log" 2>&1 || status=$?
 fi
 [ "$status" = 2 ] && [ ! -e "$temporary/forbidden-$cases" ]
 if rg -F "$temporary/MUST_NOT_OPEN_SDK" "$temporary/refusal-$cases.log" > /dev/null;then exit 1;fi
}
script=build_qwen38_ffn_graph_image.sh
for value in 0 01 -1 invalid;do refused "PIONEER_Q38_FFN_GRAPH_ROOT=$value";done
for key in PIONEER_Q38_FFN_RESIDENT_MATRIX PIONEER_Q38_FFN_PERSISTENT PIONEER_Q38_OUTPUT_TRANSFER_ROOT \
 PIONEER_Q38_WORK_ROOT_COMMAND_REUSE PIONEER_Q38_WORK_BATCH4_FETCH PIONEER_Q38_PEER_PHASE_ROOT \
 PIONEER_Q38_WORK_RAW_UNPACK_CACHE PIONEER_Q38_RAW_UNPACK_CACHE_QUALIFICATION PIONEER_Q38_WORK_LOCAL_Q4 PIONEER_Q38_LOCAL_Q4_QUALIFICATION;do
 for value in 1 01 -1 invalid;do refused PIONEER_Q38_FFN_GRAPH_ROOT=1 "$key=$value";done
done
for script in build_qwen38_ffn_matrix_image.sh build_qwen38_dispatch_image.sh build_qwen38_banks_driver.sh \
 build_qwen38_persistent_rows_image.sh build_qwen38_ffn_persistent_image.sh build_qwen38_local_image.sh;do
 for value in 1 01 -1 invalid;do refused "PIONEER_Q38_FFN_GRAPH_ROOT=$value";done
done
printf 'PASS %s complete FFN graph private/mixed/legacy/native refusals BEFORE SDK/output;NO device access\n' "$cases"
printf 'Offline complete FFN graph build-gate evidence retained at %s\n' "$temporary"
