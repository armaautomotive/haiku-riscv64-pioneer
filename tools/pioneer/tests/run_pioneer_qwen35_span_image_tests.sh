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
 rg -q 'requires the rewrite witness or restricted qualification' "$temporary/guard.err"
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
source="$directory/../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp"
for pair in PROBE_LOCK:pioneer_lock Q38_RETAIN_MEMORY:pioneer_retain_memory Q38_SPAN_CONTROL:pioneer_span_control; do
 marker=PIONEER_${pair%%:*}; name=${pair#*:}
 awk -v begin="${marker}_BEGIN" -v end="${marker}_END" '
  index($0,begin) {copying=1;next} index($0,end) {found=1;exit} copying {print} END {if(!found)exit 1}
 ' "$source" > "$temporary/$name.inc"
done
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DTT_PIONEER_EXPERIMENTAL_Q38_BANKS -DPIONEER_Q38_SPAN_FAST -DPIONEER_QWEN35_SUPPORTED_SPAN \
 -DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY -DPIONEER_QWEN35_DEVICE_DISPATCH \
 -I"$temporary" -I"$temporary/image" "$directory/pioneer_qwen35_span_service_test.cpp" -o "$temporary/service"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/service" "$temporary/image"
printf 'Offline Q35 preload evidence retained at %s\n' "$temporary"
