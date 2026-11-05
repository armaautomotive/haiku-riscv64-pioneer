#!/bin/sh
# LOCAL CPU VALIDATION ONLY,never a Work/fallback/native/device entry point.
# Existing compiled images5120/6144/17408 and original read-only GGUF.
set -eu
[ "$#" = 4 ] || exit 2
q4_affine_real_build=$(mktemp -d /private/tmp/pioneer-q4-affine-real.XXXXXXXX)
c++ -std=c++17 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter -ffp-contract=off \
 -fsanitize=address,undefined tools/pioneer/tests/pioneer_q38_exact_q4_affine_real_test.cpp \
 tools/pioneer/PioneerModelWeightCache.cpp -o "$q4_affine_real_build/test"
"$q4_affine_real_build/test" "$1" "$2" "$3" "$4"
printf 'PASS_LOCAL_REAL_Q4_FULLK_AFFINE_VALIDATION_ONLY; artifacts %s\n' "$q4_affine_real_build"
