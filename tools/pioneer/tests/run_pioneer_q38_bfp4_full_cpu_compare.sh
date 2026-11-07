#!/bin/sh
# Build and run CPU-only synthetic refusals; prints a reusable diagnostic path.
# It does not read the model, open a device, or launch a full comparison.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-bfp4-full-cpu.XXXXXXXX")
common='-std=c++17 -O2 -Wall -Wextra -Werror -ffp-contract=off'
${CXX:-c++} $common -fsanitize=address,undefined "$directory/pioneer_q38_distribution_compare_test.cpp" -o "$temporary/distribution-test"
"$temporary/distribution-test"
${CXX:-c++} $common "$directory/pioneer_q38_bfp4_full_backend_test.cpp" \
 "$directory/pioneer_q38_bfp4_full_cpu_backend.cpp" "$directory/../PioneerModelWeightCache.cpp" -o "$temporary/backend-test"
"$temporary/backend-test"
${CXX:-c++} $common -DPIONEER_Q38_BFP4_QUALITY_DIAGNOSTIC=1 \
 "$directory/pioneer_q38_bfp4_full_cpu_compare.cpp" "$directory/pioneer_q38_bfp4_full_cpu_backend.cpp" \
 "$directory/pioneer_q38_cpu_model_reference.cpp" "$directory/pioneer_q38_whole_original_bytes.cpp" \
 "$directory/../PioneerModelWeightCache.cpp" -o "$temporary/compare"
${CXX:-c++} $common -DPIONEER_Q38_BFP4_QUALITY_DIAGNOSTIC=1 \
 "$directory/pioneer_q38_bfp4_sequence_cpu_compare.cpp" "$directory/pioneer_q38_bfp4_full_cpu_backend.cpp" \
 "$directory/pioneer_q38_cpu_model_reference.cpp" "$directory/pioneer_q38_whole_original_bytes.cpp" \
 "$directory/../PioneerModelWeightCache.cpp" -o "$temporary/sequence-compare"
if "$temporary/compare" --full relative.gguf;then exit 1;fi
if "$temporary/compare" --invalid /not-a-model;then exit 1;fi
if "$temporary/sequence-compare" --two-fixed-window13 relative.gguf;then exit 1;fi
if "$temporary/sequence-compare" --hello-three-window13 relative.gguf;then exit 1;fi
if "$temporary/sequence-compare" --prefix-three-window13 relative.gguf 42;then exit 1;fi
if "$temporary/sequence-compare" --prefix-three-window13 /not-a-model 248320;then exit 1;fi
if "$temporary/sequence-compare" --prefix-three-window13 /not-a-model 1x;then exit 1;fi
if "$temporary/sequence-compare" --invalid /not-a-model;then exit 1;fi
for role in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE TT_PIONEER_Q38_WORK_MODE PIONEER_Q38_CPU_VALIDATION PIONEER_Q38_FULL_MODEL_CPU_REFERENCE;do
 if ${CXX:-c++} -std=c++17 -D"$role" -E "$directory/pioneer_q38_bfp4_full_cpu_backend.cpp" > /dev/null 2> "$temporary/$role.stderr";then exit 1;fi
 rg -q 'BFP4 full-model backend is CPU-only diagnostic code' "$temporary/$role.stderr"
done
for role in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE TT_PIONEER_Q38_WORK_MODE;do
 if ${CXX:-c++} -std=c++17 -DPIONEER_Q38_BFP4_QUALITY_DIAGNOSTIC=1 -D"$role" -E "$directory/pioneer_q38_bfp4_sequence_cpu_compare.cpp" > /dev/null 2> "$temporary/sequence-$role.stderr";then exit 1;fi
 rg -q 'CPU-only BFP4 sequence comparison role required' "$temporary/sequence-$role.stderr"
done
printf 'PASS BFP4 full CPU comparator build and isolated roles; binary %s/compare\n' "$temporary"
