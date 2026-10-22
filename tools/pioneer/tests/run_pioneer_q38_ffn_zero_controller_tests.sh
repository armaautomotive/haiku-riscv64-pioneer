#!/bin/sh
# OFFLINE ONLY:private owner+controller+EXACT RV32 zero image;NO card/driver.
set -eu
[ "$#" -ge 1 ] && [ "$#" -le 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
hash_is() { actual=$(shasum -a 256 "$1"); [ "${actual%% *}" = "$2" ]; }
hash_is "$1/boot.bin" e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b
hash_is "$1/text.bin" 39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-ffn-zero-controller.XXXXXXXX")
for name in banks zero_controller; do
 ${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
  -DPIONEER_Q38_GDN_MUTABLE_STATE "$directory/pioneer_q38_ffn_${name}_test.cpp" -o "$temporary/$name"
 if [ "$name" = banks ]; then
  ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/$name"
 else
  ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/$name" "$1"
 fi
done
for mode in 1 -1 invalid; do
 status=0
 PIONEER_Q38_GDN_MUTABLE_STATE=$mode sh "$directory/../build_qwen38_banks_driver.sh" /nonexistent-sdk "$temporary/forbidden-$mode" > "$temporary/refusal-$mode.log" 2>&1 || status=$?
 [ "$status" = 2 ] && [ ! -e "$temporary/forbidden-$mode" ]
 if rg -F '/nonexistent-sdk' "$temporary/refusal-$mode.log" > /dev/null; then exit 1; fi
done
if [ "$#" = 2 ]; then
 sdk=$2/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
 "$2/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++" -std=gnu++17 -O2 -Wall -Wextra -Werror \
  -fno-exceptions -fno-rtti -ffreestanding -fstack-usage -mcmodel=medany -fpic -D_KERNEL_MODE \
  -I"$sdk/headers" -idirafter "$sdk/headers/posix" -c "$directory/pioneer_q38_ffn_zero_kernel_compile.cpp" -o "$temporary/kernel-controller.o"
 test -s "$temporary/kernel-controller.o"
 awk -F '\t' 'NF==3 {if ($2+0>1024) {print "Excess kernel witness frame:",$0;bad=1};if($2+0>maximum)maximum=$2+0;seen=1} END {if(!seen||bad)exit 1;print "MAX_KERNEL_WITNESS_FRAME_BYTES="maximum";COMPILE_ONLY_NOT_NATIVE_ADAPTER"}' "$temporary/kernel-controller.su"
fi
printf 'Offline FFN workspace initialization evidence retained at %s\n' "$temporary"
