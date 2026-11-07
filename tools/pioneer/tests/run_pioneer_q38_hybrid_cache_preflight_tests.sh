#!/bin/sh
# Negative paths and role isolation only; no real model, cache or card access.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-hybrid-preflight-test.XXXXXXXX")
fingerprint=5dc4ffb1257bf99a8b38f7139e5fe95d04e42f11d63beda382d78aeacb1dc45c
common='-std=c++17 -O2 -ffp-contract=off -Wall -Wextra -Werror'
${CXX:-c++} $common -DPIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC=1 \
 -DPIONEER_BFP4_IMPLEMENTATION_SHA256="\"$fingerprint\"" \
 "$directory/../pioneer_q38_hybrid_cache_preflight.cpp" "$directory/../PioneerModelWeightCache.cpp" -o "$temporary/preflight"
if "$temporary/preflight" --verify-prepared relative /not-a-cache;then exit 1;fi
if "$temporary/preflight" --verify-prepared "$temporary/missing4" "$temporary/missing8";then exit 1;fi
[ ! -e "$temporary/missing4" ] && [ ! -e "$temporary/missing8" ]
mkdir -m 700 "$temporary/existing"
ln -s "$temporary/existing" "$temporary/link"
if "$temporary/preflight" --verify-prepared "$temporary/link" "$temporary/existing";then exit 1;fi
for role in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE TT_PIONEER_Q38_WORK_MODE PIONEER_Q38_CPU_VALIDATION PIONEER_Q38_FULL_MODEL_CPU_REFERENCE;do
 if ${CXX:-c++} $common -DPIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC=1 -D"$role" \
  -DPIONEER_BFP4_IMPLEMENTATION_SHA256="\"$fingerprint\"" -fsyntax-only "$directory/../pioneer_q38_hybrid_cache_preflight.cpp" 2> "$temporary/$role.stderr";then exit 1;fi
 rg -q 'Hybrid cache preflight requires its distinct CPU-only preparation role' "$temporary/$role.stderr"
done
for role in PIONEER_Q38_FULL_OFFLOAD_CLIENT PIONEER_Q38_FULL_OFFLOAD_NATIVE PIONEER_Q38_BF16_FULL_CLIENT PIONEER_Q38_BF16_FULL_NATIVE PIONEER_Q38_TOKEN_NATIVE_CONTEXT PIONEER_DRIVER_BUILD_TAG;do
 if ${CXX:-c++} $common -DPIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC=1 -D"$role" \
  -DPIONEER_BFP4_IMPLEMENTATION_SHA256="\"$fingerprint\"" -fsyntax-only "$directory/../pioneer_q38_hybrid_cache_preflight.cpp" 2> "$temporary/model-$role.stderr";then exit 1;fi
 rg -q 'Hybrid verified model requires the isolated preparation role' "$temporary/model-$role.stderr"
done
printf 'PASS hybrid cache preflight negative paths and 11 forbidden roles; NO MODEL, DEVICE OR CACHE WRITES\n'
