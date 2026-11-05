#!/bin/sh
# SPDX-License-Identifier: MIT
# OFFBOARD only. No physical/native owner, model or device access.
set -eu
[ "$#" = 1 ] || exit 2
q4_primary_sdk=$1
cd "$(dirname "$0")/../../.."
q4_primary_build=$(mktemp -d /private/tmp/pioneer-q4-primary-tests.XXXXXXXX)
python3 tools/pioneer/build_q38_native_bfp_matvec.py "$q4_primary_sdk" "$q4_primary_build/image" 4 --exact-q4-primary
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off \
 -fsanitize=address,undefined tools/pioneer/tests/pioneer_q38_exact_q4_primary_test.cpp \
 -o "$q4_primary_build/direct"
"$q4_primary_build/direct"
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -ffp-contract=off \
 -Wno-deprecated-declarations -fsanitize=address,undefined \
 tools/pioneer/tests/pioneer_q38_exact_q4_primary_image_test.cpp tools/pioneer/PioneerModelWeightCache.cpp \
 -o "$q4_primary_build/image-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$q4_primary_build/image-test" "$q4_primary_build/image"
printf 'PASS_OFFBOARD_EXACT_Q4_PRIMARY_DIRECT_AND_ACTUAL_IMAGE; artifacts %s\n' "$q4_primary_build"
