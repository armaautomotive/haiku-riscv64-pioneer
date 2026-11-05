#!/bin/sh
# Entire fixed full-K affine native qualification;local only,NEVER device IO.
set -eu
[ "$#" = 1 ] || exit 2
q4_affine_native_sdk=$1
cd "$(dirname "$0")/../../.."
q4_affine_native_build=$(mktemp -d /private/tmp/pioneer-native-exact-q4-affine-tests.XXXXXXXX)
python3 tools/pioneer/build_native_bfp_diagnostic.py "$q4_affine_native_sdk" "$q4_affine_native_build/build" --exact-q4-affine-fixed
c++ -std=c++17 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter -Wno-deprecated-declarations -ffp-contract=off \
 -fsanitize=address,undefined -DPIONEER_NATIVE_EXACT_Q4_AFFINE_DIAGNOSTIC=1 -I"$q4_affine_native_build/build" \
 tools/pioneer/tests/pioneer_native_bfp_owner_test.cpp tools/pioneer/PioneerModelWeightCache.cpp -o "$q4_affine_native_build/owner"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$q4_affine_native_build/owner" "$q4_affine_native_build/build/bfp4" "$q4_affine_native_build/build/bfp4"
python3 tools/pioneer/tests/pioneer_native_bfp_close_test.py
python3 tools/pioneer/tests/pioneer_native_bfp_mapping_test.py --exact-q4-affine
python3 tools/pioneer/tests/pioneer_native_bfp_control_test.py --exact-q4-affine
python3 tools/pioneer/tests/pioneer_native_bfp_client_test.py --exact-q4-affine
python3 tools/pioneer/tests/pioneer_native_exact_q4_arithmetic_test.py
python3 tools/pioneer/tests/pioneer_native_exact_q4_parent_test.py --affine
printf 'PASS_OFFBOARD_NATIVE_EXACT_Q4_FULLK_AFFINE_COMPLETE_SCOPE; artifacts %s\n' "$q4_affine_native_build"
