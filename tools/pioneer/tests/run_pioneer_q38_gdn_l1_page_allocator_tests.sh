#!/bin/sh
# Native-source page-map geometry only; no device or L1 SRAM access.
set -eu
[ "$#" -le 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d /private/tmp/pioneer-q38-gdn-l1-page-allocator.XXXXXXXX)
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$directory/pioneer_q38_gdn_l1_page_allocator_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
if [ "$#" = 1 ]; then
	sdk=$1
	compiler=$sdk/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++
	[ -x "$compiler" ] || exit 2
	develop=$sdk/objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop
	"$compiler" -std=gnu++17 -O2 -Wall -Wextra -Werror -ffreestanding \
		-fno-exceptions -fno-rtti -fstack-usage -mcmodel=medany -fpic \
		-D_KERNEL_MODE=1 -DPIONEER_Q38_FULL_OFFLOAD_NATIVE=1 \
		-DPIONEER_Q38_FULL_OFFLOAD_BATCHED=1 -DPIONEER_Q38_FULL_OFFLOAD_METRICS=1 \
		-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4=1 -DTT_PIONEER_DRIVER_RELOAD=1 \
		-DPIONEER_DRIVER_BUILD_TAG=0x2026110500010097ULL \
		-I"$develop/headers" -idirafter "$develop/headers/posix" \
		-c "$directory/pioneer_q38_gdn_l1_page_allocator_kernel_compile.cpp" \
		-o "$temporary/kernel-witness.o"
	test -s "$temporary/kernel-witness.o"
	awk -F '\t' '$1 ~ /pioneer_q38_gdn_l1_page_allocator_compile_witness/ {if($2+0>2048)exit 1;seen=1}
	 END {if(!seen)exit 1;print "PASS prepared-Q4 native-role cross-compile; compile-only, no kernel/module/device authority"}' \
		"$temporary/kernel-witness.su"
fi
