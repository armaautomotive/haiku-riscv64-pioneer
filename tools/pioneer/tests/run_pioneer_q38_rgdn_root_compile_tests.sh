#!/bin/sh
# Kernel compile/stack witness ONLY,NO usable module/native selection.
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
sdk=$1/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-rgdn-root-compile.XXXXXXXX")
"$1/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++" -std=gnu++17 -O2 -Wall -Wextra -Werror \
 -fno-exceptions -fno-rtti -ffreestanding -fstack-usage -mcmodel=medany -fpic -D_KERNEL_MODE \
 -I"$sdk/headers" -idirafter "$sdk/headers/posix" -c "$directory/pioneer_q38_rgdn_root_kernel_compile.cpp" -o "$temporary/root-controller.o"
test -s "$temporary/root-controller.o"
awk -F '\t' '$1 ~ /RgdnRoot|rgdn_root_.*_compile/ {if ($2+0>1024){print "Excess kernel frame:",$0;bad=1};if($2+0>maximum)maximum=$2+0;seen=1}
 END {if(!seen||bad)exit 1;print "MAX_ROOT_GDN_COMPILE_ONLY_FRAME_BYTES="maximum}' "$temporary/root-controller.su"
printf 'Root GDN controller COMPILE-ONLY evidence at %s;NO native adapter/module/qualification\n' "$temporary"
