#!/bin/sh
# Real ownership algorithm,simulated endpoints. No SDK/card access.
set -eu
[ "$#" -le 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-ffn-peer-lifetime.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_ffn_peer_lifetime_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
if [ "$#" = 1 ]; then
 sdk=$1/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
 "$1/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++" -std=gnu++17 -O2 -Wall -Wextra -Werror \
  -fno-exceptions -fno-rtti -ffreestanding -fstack-usage -mcmodel=medany -fpic -D_KERNEL_MODE \
  -I"$sdk/headers" -idirafter "$sdk/headers/posix" -c "$directory/pioneer_q38_ffn_peer_lifetime_kernel_compile.cpp" -o "$temporary/kernel.o"
 awk -F '\t' 'NF==3 {if($2+0>1024){print "Excess kernel frame:",$0;bad=1};if($2+0>maximum)maximum=$2+0;seen=1} END {if(!seen||bad)exit 1;print "MAX_PEER_LIFETIME_KERNEL_FRAME_BYTES="maximum";COMPILE_ONLY_NOT_NATIVE_FACTORY"}' "$temporary/kernel.su"
fi
printf 'Offline FFN worker lifetime evidence retained at %s;NO device access\n' "$temporary"
