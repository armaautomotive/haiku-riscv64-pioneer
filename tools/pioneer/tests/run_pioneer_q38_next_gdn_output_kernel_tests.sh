#!/bin/sh
# Independent QOP2 metadata/catalogue, not a linked or installed driver.
set -eu
python3 -c 'import os,sys;sys.exit(0 if all(not k.startswith("PIONEER_") or v=="0" for k,v in os.environ.items()) else 2)' || exit 2
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d /private/tmp/pioneer-q38-next-gdn-output-kernel.XXXXXXXX)
sdk=$1/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
for copy in a b;do
 "$1/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++" -std=gnu++17 -O2 -Wall -Wextra -Werror \
  -fno-exceptions -fno-rtti -ffreestanding -fstack-usage -mcmodel=medany -fpic -D_KERNEL_MODE \
  -DPIONEER_Q38_GDN_GATED_NATIVE=1 -DPIONEER_Q38_GDN_RECURRENT_NATIVE=1 \
  -I"$sdk/headers" -idirafter "$sdk/headers/posix" -c "$directory/pioneer_q38_next_gdn_output_kernel_compile.cpp" -o "$temporary/$copy.o"
 awk -F '\t' 'NF==3 {if($2+0>2048)bad=1;if($2+0>maximum)maximum=$2+0;seen=1} END {if(!seen||bad)exit 1;print "MAX_QOP2_KERNEL_DATA_FRAME_BYTES="maximum}' "$temporary/$copy.su"
done
cmp "$temporary/a.o" "$temporary/b.o"
shasum -a 256 "$temporary/a.o" "$temporary/b.o"
printf 'PASS QOP2 real RISC-V DATA parsers/receipts/catalogue reproducible;NO native owner/route/driver/card access:%s\n' "$temporary"
