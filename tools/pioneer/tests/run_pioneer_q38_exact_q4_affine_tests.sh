#!/bin/sh
# Local/compiled image qualification ONLY. Never touches a Pioneer device.
set -eu
[ "$#" = 1 ] || exit 2
q4_affine_sdk=$1
q4_affine_build=$(mktemp -d /private/tmp/pioneer-q4-affine-tests.XXXXXXXX)
for width in 5120 6144 17408;do
 python3 tools/pioneer/build_q38_exact_q4_affine.py "$q4_affine_sdk" "$q4_affine_build/image$width" "$width"
done
c++ -std=c++17 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter -ffp-contract=off \
 -fsanitize=address,undefined tools/pioneer/tests/pioneer_q38_exact_q4_affine_test.cpp -o "$q4_affine_build/direct"
"$q4_affine_build/direct"
c++ -std=c++17 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter -ffp-contract=off \
 -fsanitize=address,undefined tools/pioneer/tests/pioneer_q38_exact_q4_affine_image_test.cpp -o "$q4_affine_build/image-test"
"$q4_affine_build/image-test" "$q4_affine_build/image5120" "$q4_affine_build/image6144" "$q4_affine_build/image17408"
printf 'PASS_OFFBOARD_EXACT_Q4_FULLK_ORIGINAL_AFFINE; artifacts %s\n' "$q4_affine_build"
