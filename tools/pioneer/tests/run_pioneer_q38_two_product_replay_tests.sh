#!/bin/sh
# OFFBOARD ONLY. Actual RV images,arithmetic and explicit role guards.
set -eu
test "$#" = 1 || exit 2
replay_sdk=$1
replay_dir=$(mktemp -d /private/tmp/pioneer-q4-replay13-tests.XXXXXXXX)
for width in 5120 6144 17408;do
 python3 tools/pioneer/build_q38_exact_q4_affine.py "$replay_sdk" "$replay_dir/image$width" "$width" --two-product-replay
done
for test in pioneer_q38_two_product_restore_zero_test pioneer_q38_two_product_replay_words_test pioneer_q38_exact_q4_two_product_test pioneer_q38_exact_q4_two_product_image_test;do
 c++ -std=c++17 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter \
  -ffp-contract=off -fsanitize=address,undefined -DPIONEER_Q38_TWO_PRODUCT_REPLAY_TEST=1 \
  "tools/pioneer/tests/$test.cpp" -o "$replay_dir/$test"
done
for test in pioneer_q38_two_product_restore_zero_test pioneer_q38_two_product_replay_words_test pioneer_q38_exact_q4_two_product_test;do
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$replay_dir/$test"
done
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$replay_dir/pioneer_q38_exact_q4_two_product_image_test" \
 "$replay_dir/image5120" "$replay_dir/image6144" "$replay_dir/image17408"
c++ -std=c++17 -fsyntax-only -x c++ -include tools/pioneer/PioneerQ38ExactQ4TwoProductReplay.h /dev/null
for role in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE PIONEER_Q38_CPU_VALIDATION;do
 if c++ -std=c++17 -fsyntax-only -x c++ "-D$role=1" \
  -include tools/pioneer/PioneerQ38ExactQ4TwoProductReplay.h /dev/null 2> "$replay_dir/$role.stderr";then
  printf 'REFUSE unexpectedly accepted %s\n' "$role" >&2;exit 1
 fi
 rg -q 'New full-K arithmetic prototype requires separate native integration' "$replay_dir/$role.stderr"
done
if python3 tools/pioneer/build_q38_exact_q4_affine.py "$replay_sdk" "$replay_dir/invalid-both" 17408 \
 --two-product-replay --two-product > "$replay_dir/invalid-both.stdout" 2> "$replay_dir/invalid-both.stderr";then
 printf 'REFUSE unexpectedly accepted incompatible replay variants\n' >&2;exit 1
fi
test ! -e "$replay_dir/invalid-both"
rg -q 'not allowed with argument' "$replay_dir/invalid-both.stderr"
printf 'PASS_LOCAL_REPLAY13_ROLE_REFUSALS3 incompatible_variant_refusal1 positive_header_parse1\n'
printf 'PASS_OFFBOARD_REPLAY13_TWO_PRODUCT;artifacts %s;NO_NATIVE_OR_CARD_SPEED_GRANT\n' "$replay_dir"
