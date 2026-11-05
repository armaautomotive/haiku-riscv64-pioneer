#!/bin/sh
# Functional ISA arithmetic and actual RV32 images;never opens a card.
set -eu
test "$#" = 1 || exit 2
two_product_sdk=$1
two_product_dir=$(mktemp -d /private/tmp/pioneer-two-product-q4.XXXXXXXX)
for width in 5120 6144 17408;do
 python3 tools/pioneer/build_q38_exact_q4_affine.py "$two_product_sdk" "$two_product_dir/image$width" "$width" --two-product
done
c++ -std=c++17 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter \
 -ffp-contract=off -fsanitize=address,undefined \
 tools/pioneer/tests/pioneer_q38_exact_q4_two_product_test.cpp -o "$two_product_dir/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$two_product_dir/test"
c++ -std=c++17 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter \
 -ffp-contract=off -fsanitize=address,undefined \
 tools/pioneer/tests/pioneer_q38_exact_q4_two_product_image_test.cpp -o "$two_product_dir/image-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$two_product_dir/image-test" \
 "$two_product_dir/image5120" "$two_product_dir/image6144" "$two_product_dir/image17408"
# The positive parse prevents unrelated include failures masquerading as
# a role guard. These are compiler/CLI tests only,not native authority.
c++ -std=c++17 -fsyntax-only -x c++ -include tools/pioneer/PioneerQ38ExactQ4TwoProduct.h /dev/null
for role in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE PIONEER_Q38_CPU_VALIDATION;do
 if c++ -std=c++17 -fsyntax-only -x c++ "-D$role=1" \
  -include tools/pioneer/PioneerQ38ExactQ4TwoProduct.h /dev/null 2> "$two_product_dir/$role.stderr";then
  printf 'REFUSE unexpectedly accepted %s\n' "$role" >&2;exit 1
 fi
 rg -q 'New full-K arithmetic prototype requires separate native integration' "$two_product_dir/$role.stderr"
done
if python3 tools/pioneer/build_q38_exact_q4_affine.py "$two_product_sdk" "$two_product_dir/invalid-both" 17408 \
 --two-product --fixed-fixture > "$two_product_dir/invalid-both.stdout" 2> "$two_product_dir/invalid-both.stderr";then
 printf 'REFUSE unexpectedly accepted incompatible image variants\n' >&2;exit 1
fi
test ! -e "$two_product_dir/invalid-both"
rg -q 'not allowed with argument' "$two_product_dir/invalid-both.stderr"
printf 'PASS_OFFBOARD_TWO_PRODUCT_ROLE_REFUSALS3 incompatible_variant_refusal1 positive_header_parse1\n'
printf 'Two-product arithmetic fixture retained at %s;NO physical speed result\n' "$two_product_dir"
