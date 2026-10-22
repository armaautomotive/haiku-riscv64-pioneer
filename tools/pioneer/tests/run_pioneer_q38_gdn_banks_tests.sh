#!/bin/sh
set -eu
[ "$#" -le 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-gdn-banks.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_gdn_banks_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
for mode in 1 -1 invalid; do
 status=0
 PIONEER_Q38_GDN_MUTABLE_STATE=$mode sh "$directory/../build_qwen38_banks_driver.sh" /nonexistent-sdk "$temporary/forbidden" > "$temporary/refusal-$mode.log" 2>&1 || status=$?
 [ "$status" = 2 ] && [ ! -e "$temporary/forbidden" ]
 # Invalid values may fail silently before the ordinary-GDN diagnostic.
 # The contract is refusal BEFORE SDK/output,not an obsolete log phrase.
 if rg -F '/nonexistent-sdk' "$temporary/refusal-$mode.log" > /dev/null; then exit 1; fi
done
if [ "$#" = 1 ]; then
 sdk=$1/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
 "$1/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++" -std=gnu++17 -O2 -Wall -Wextra -Werror \
  -fno-exceptions -fno-rtti -ffreestanding -fstack-usage -mcmodel=medany -fpic -D_KERNEL_MODE \
  -I"$sdk/headers" -idirafter "$sdk/headers/posix" -c "$directory/pioneer_q38_gdn_kernel_compile.cpp" -o "$temporary/kernel-owner.o"
 # The owner itself is heap-only. Bind's coverage proof must stay small.
 awk -F '\t' 'NF==3 { if ($2+0>2048) { print "Excess kernel stack:",$0; bad=1 }; seen=1 } END { if (!seen || bad) exit 1 }' "$temporary/kernel-owner.su"
fi
printf 'Offline mutable GDN bank owner evidence retained at %s;NO hardware accessed\n' "$temporary"
