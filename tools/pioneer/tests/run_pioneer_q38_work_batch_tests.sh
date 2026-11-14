#!/bin/sh
set -eu
[ "$#" = 1 ] || [ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
test "$(shasum -a 256 "$1/text.bin" | cut -d ' ' -f 1)" = 377b6a60ec8eb054e8f28c59bccbc4380d62b78cf01c1663033a80e83fa116ae
test "$(shasum -a 256 "$1/boot.bin" | cut -d ' ' -f 1)" = e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-work-batch.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_COMPACT_ACTIVATIONS -DPIONEER_Q38_REUSE_ACTIVATIONS -DPIONEER_Q38_REUSE_CROSS_QUANT \
 -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR -DTT_PIONEER_Q38_WORK_MODE \
 "$directory/pioneer_q38_work_batch_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1"
source=$directory/../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp
for section in PROBE_LOCK Q38_DISPATCH_PREPARE Q38_DISPATCH_CONTROL Q38_DISPATCH_RETAIN Q38_DISPATCH_COOKIE_RELEASE; do
 marker=PIONEER_$section
 case "$section" in PROBE_LOCK) name=pioneer_lock;; *) name=$(printf '%s' "$marker" | tr '[:upper:]' '[:lower:]');; esac
 awk -v begin="${marker}_BEGIN" -v end="${marker}_END" '
  index($0,begin) {copying=1;next} index($0,end) {found=1;exit} copying {print} END {if(!found)exit 1}
 ' "$source" > "$temporary/$name.inc"
done
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined \
 -DPIONEER_Q38_COMPACT_ACTIVATIONS -DPIONEER_Q38_REUSE_ACTIVATIONS -DPIONEER_Q38_REUSE_CROSS_QUANT \
 -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD \
 -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR -DTT_PIONEER_Q38_WORK_MODE -I"$temporary" \
 "$directory/pioneer_q38_work_batch_control_test.cpp" -o "$temporary/control"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/control" "$1"
if [ "$#" = 2 ]; then
 test "$(shasum -a 256 "$2/text.bin" | cut -d ' ' -f 1)" = ba2670093a1d8432e0c8ce74f26d58a67d3f9071601673df73926cd6ae836aa5
 test "$(shasum -a 256 "$2/boot.bin" | cut -d ' ' -f 1)" = e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b
 ${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined \
  -DPIONEER_Q38_COMPACT_ACTIVATIONS -DPIONEER_Q38_REUSE_ACTIVATIONS -DPIONEER_Q38_REUSE_CROSS_QUANT \
  -DPIONEER_Q38_WORK_POSTED_STAGE -DPIONEER_Q38_WORK_COHORT_CACHE -DPIONEER_Q38_Q4_TWO_PHASE \
  -DPIONEER_Q38_SPAN_PRELOAD -DPIONEER_Q38_SPAN_FAST -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH \
  -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR \
  -DTT_PIONEER_Q38_WORK_MODE -I"$temporary" "$directory/pioneer_q38_work_batch_control_test.cpp" -o "$temporary/control-span"
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/control-span" "$2"
fi
awk '
 /PIONEER_Q38_FFN_CODE_NATIVE_ROUTING_BEGIN/ {copying=1;next}
 /PIONEER_Q38_FFN_CODE_NATIVE_ROUTING_END/ {found=1;exit}
 copying {print} END {if(!found)exit 1}
' "$source" > "$temporary/routing.inc"
for mode in ordinary candidate; do
 extra=
 [ "$mode" = ordinary ] || extra=-DPIONEER_Q38_WORK_BATCH_NATIVE_152=1
 ${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $extra \
  "-DPIONEER_WORK_BATCH_ROUTING_INCLUDE=\"$temporary/routing.inc\"" \
  "$directory/pioneer_q38_work_batch_routing_test.cpp" -o "$temporary/routing-$mode"
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/routing-$mode"
done
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR \
 -DPIONEER_Q38_WORK_PHASE_PROFILE=1 \
 "$directory/pioneer_q38_work_batch_session_test.cpp" -o "$temporary/session"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/session"
if ${CXX:-c++} -std=c++17 -DPIONEER_Q38_CPU_VALIDATION=1 -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR \
 -fsyntax-only "$directory/pioneer_q38_work_batch_session_test.cpp" > "$temporary/forbidden.log" 2>&1; then exit 1; fi
grep -q 'Batch client is a distinct no-reference Work role' "$temporary/forbidden.log"
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR -I"$directory/fixtures/batch-probe" \
 "$directory/pioneer_q38_work_batch_probe_test.cpp" -o "$temporary/debug-probe-test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/debug-probe-test"
printf 'Offline batch evidence retained at %s\n' "$temporary"
