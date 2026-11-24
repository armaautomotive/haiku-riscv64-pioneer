#!/bin/sh
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q35-span.XXXXXXXX")
sh "$directory/../build_qwen35_span_image.sh" "$1" "$temporary/image"
for forbidden in PIONEER_QWEN35_RECOVERY_QUALIFY PIONEER_QWEN35_DEVICE_DISPATCH_TRIAL; do
 if ${CXX:-c++} -E -x c++ -DPIONEER_Q38_SPAN_FAST -DPIONEER_QWEN35_SUPPORTED_SPAN \
  -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY -DPIONEER_QWEN35_DEVICE_DISPATCH -D"$forbidden" \
  -I"$temporary/image" -include "$directory/../../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanImage.h" \
  /dev/null > "$temporary/guard.out" 2> "$temporary/guard.err"; then
  printf 'Q35 span guard failed to reject %s\n' "$forbidden" >&2
  exit 1
 fi
 rg -q 'restricted to the non-loadable rewrite witness' "$temporary/guard.err"
done
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_SPAN_FAST -DPIONEER_QWEN35_SUPPORTED_SPAN \
 -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY -DPIONEER_QWEN35_DEVICE_DISPATCH \
 -I"$temporary/image" "$directory/pioneer_qwen35_span_image_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$temporary/image"
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_SPAN_FAST -DPIONEER_QWEN35_SUPPORTED_SPAN \
 -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY -DPIONEER_QWEN35_DEVICE_DISPATCH \
 -I"$temporary/image" "$directory/pioneer_qwen35_span_controller_test.cpp" -o "$temporary/controller"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/controller" "$temporary/image"
printf 'Offline Q35 preload evidence retained at %s\n' "$temporary"
