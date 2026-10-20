#!/bin/sh
# Extract ACTUAL native ownership gate + physical-audit call site. No card.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-native-zero-scope.XXXXXXXX")
source=$directory/../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp
awk '/PIONEER_Q38_STATE_ZERO_NATIVE_SCOPE_BEGIN/ {copying=1;next}
 /PIONEER_Q38_STATE_ZERO_NATIVE_SCOPE_END/ {found=1;exit}
 copying {print} END {if(!found)exit 1}' "$source" > "$temporary/native_scope.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DPIONEER_Q38_GDN_MUTABLE_STATE -I"$temporary" "$directory/pioneer_q38_state_zero_native_scope_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
printf 'Offline native zero-scope evidence retained at %s\n' "$temporary"
