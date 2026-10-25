#!/bin/sh
# Real RV64 SDK compilation ONLY; SDK is read-only, no native selector/card.
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
temporary=$(mktemp -d /private/tmp/pioneer-output-proof-kernel.XXXXXXXX)
sdk=$1/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
"$1/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++" -std=gnu++17 -O2 -Wall -Wextra -Werror \
 -fno-exceptions -fno-rtti -ffreestanding -fstack-usage -mcmodel=medany -fpic -D_KERNEL_MODE \
 -I"$sdk/headers" -idirafter "$sdk/headers/posix" -c \
 "$directory/pioneer_q38_resident_output_proof_kernel_compile.cpp" -o "$temporary/proof.o"
awk -F '\t' 'NF==3 {if($2+0>maximum)maximum=$2+0;if($2+0>2048||$3!="static")bad=1;seen=1} END {if(!seen||bad)exit 1;print "MAX_OUTPUT_PROOF_KERNEL_FRAME_BYTES="maximum}' "$temporary/proof.su"
shasum -a 256 "$temporary/proof.o"
printf 'Output completion proof:%s;DATA ONLY,NO native factory/driver/link/install/card access\n' "$temporary"
