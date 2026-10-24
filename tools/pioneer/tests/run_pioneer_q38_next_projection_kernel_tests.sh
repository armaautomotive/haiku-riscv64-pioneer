#!/bin/sh
# Independent QPS2 DATA only,real kernel ABI;no linked driver/native grant.
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-next-projection-kernel.XXXXXXXX")
sdk=$1/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
for copy in a b;do
 "$1/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++" -std=gnu++17 -O2 -Wall -Wextra -Werror \
  -fno-exceptions -fno-rtti -ffreestanding -fstack-usage -mcmodel=medany -fpic -D_KERNEL_MODE -DPIONEER_Q38_GDN_MUTABLE_STATE \
  -I"$sdk/headers" -idirafter "$sdk/headers/posix" -c "$directory/pioneer_q38_next_projection_kernel_compile.cpp" -o "$temporary/$copy.o"
 awk -F '\t' 'NF==3 {if($2+0>2048)bad=1;if($2+0>maximum)maximum=$2+0;seen=1} END {if(!seen||bad)exit 1;print "MAX_QPS2_KERNEL_DATA_FRAME_BYTES="maximum";HEAP_ONLY"}' "$temporary/$copy.su"
done
cmp "$temporary/a.o" "$temporary/b.o"
shasum -a 256 "$temporary/a.o" "$temporary/b.o"
printf 'PASS QPS2 real RISC-V kernel DATA factories/proofs/catalog reproducible;NO actual native owner/driver/route;artifacts:%s\n' "$temporary"
