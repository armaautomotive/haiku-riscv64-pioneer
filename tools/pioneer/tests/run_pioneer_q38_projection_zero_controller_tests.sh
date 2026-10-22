#!/bin/sh
# ACTUAL exact frozen zero ROM with new512-page controller,NO native IO.
set -eu
[ "$#" = 2 ] || [ "$#" = 3 ] || exit 2
[ "$(shasum -a 256 "$1/text.bin" | awk '{print $1}')" = 39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-projection-zero.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DPIONEER_Q38_GDN_MUTABLE_STATE \
 "$directory/pioneer_q38_projection_zero_controller_test.cpp" -o "$temporary/test"
if [ "$#" = 3 ];then
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1" "$3"
else
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1"
fi
sdk=$2/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
"$2/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++" -std=gnu++17 -O2 -Wall -Wextra -Werror \
 -fno-exceptions -fno-rtti -ffreestanding -fstack-usage -mcmodel=medany -fpic -D_KERNEL_MODE \
 -I"$sdk/headers" -idirafter "$sdk/headers/posix" -c "$directory/pioneer_q38_projection_zero_kernel_compile.cpp" -o "$temporary/kernel.o"
awk -F '\t' 'NF==3 {if($2+0>2048)bad=1;if($2+0>maximum)maximum=$2+0;seen=1} END {if(!seen||bad)exit 1;print "MAX_PROJECTION_ZERO_KERNEL_FRAME_BYTES="maximum";NO_NATIVE_ADAPTER"}' "$temporary/kernel.su"
printf 'Offline new projection zero evidence:%s;NO device access\n' "$temporary"
