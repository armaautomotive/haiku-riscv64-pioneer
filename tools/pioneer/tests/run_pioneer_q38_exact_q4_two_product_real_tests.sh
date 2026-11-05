#!/bin/sh
# LOCAL CPU VALIDATION ONLY. Existing distinct candidate images and
# original read-only GGUF;no native context,card,cache rewrite or fallback.
set -eu
test "$#" = 4 || exit 2
two_product_real_dir=$(mktemp -d /private/tmp/pioneer-two-product-real.XXXXXXXX)
c++ -std=c++17 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter \
 -ffp-contract=off -fsanitize=address,undefined -DPIONEER_Q38_TWO_PRODUCT_REAL_TEST=1 \
 tools/pioneer/tests/pioneer_q38_exact_q4_affine_real_test.cpp \
 tools/pioneer/PioneerModelWeightCache.cpp -o "$two_product_real_dir/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$two_product_real_dir/test" "$1" "$2" "$3" "$4"
printf 'PASS_LOCAL_REAL_Q4_TWO_PRODUCT_VALIDATION_ONLY; artifacts %s\n' "$two_product_real_dir"
