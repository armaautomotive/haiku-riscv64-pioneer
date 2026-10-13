#!/bin/sh
# Build artifacts only; no target upload, kexec load, execution or flash.
set -eu
if [ "$#" -gt 1 ]; then
	echo "Usage: $0 [output-directory]" >&2
	exit 2
fi
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
source_dir=$script_dir/kexec_probe
output_dir=${1:-$script_dir/../../generated.pioneer/kexec-probe-20261013/build}
cross=${PIONEER_CROSS_PREFIX:-/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-}
for tool in gcc ld objcopy objdump readelf nm; do
	command -v "${cross}${tool}" >/dev/null
done
mkdir -p "$output_dir"
output_dir=$(CDPATH= cd -- "$output_dir" && pwd)
common='-march=rv64imac_zicsr_zifencei -mabi=lp64 -mcmodel=medany -msmall-data-limit=0 -mno-relax -fno-pie -fno-pic'
# Deliberate word splitting: common contains only the fixed compiler flags above.
"${cross}gcc" $common -c "$source_dir/entry.S" -o "$output_dir/entry.o"
for file in main format; do
	"${cross}gcc" $common -std=c11 -O2 -ffreestanding -fno-builtin \
		-fno-stack-protector -fno-jump-tables -fno-unwind-tables \
		-fno-asynchronous-unwind-tables -Wall -Wextra -Werror \
		-c "$source_dir/$file.c" -o "$output_dir/$file.o"
done
# Use ld directly: the Haiku GCC specs request an unavailable lp64 emulation.
"${cross}ld" -m elf64lriscv --no-relax --build-id=none -z max-page-size=4096 \
	-T "$source_dir/image.ld" -Map "$output_dir/probe.map" \
	"$output_dir/entry.o" "$output_dir/main.o" "$output_dir/format.o" \
	-o "$output_dir/probe.elf"
undefined=$("${cross}nm" -u "$output_dir/probe.elf")
if [ -n "$undefined" ]; then
	echo 'Probe has unresolved runtime dependencies' >&2
	exit 1
fi
"${cross}objcopy" -O binary "$output_dir/probe.elf" "$output_dir/PioneerKexecProbe.Image"
# A second link catches absolute-address dependencies in this freestanding image.
"${cross}ld" -m elf64lriscv --no-relax --build-id=none -z max-page-size=4096 \
	--defsym=PROBE_LINK_BASE=0x80200000 -T "$source_dir/image.ld" \
	"$output_dir/entry.o" "$output_dir/main.o" "$output_dir/format.o" \
	-o "$output_dir/probe-relocated.elf"
"${cross}objcopy" -O binary "$output_dir/probe-relocated.elf" "$output_dir/probe-relocated.Image"
cmp "$output_dir/PioneerKexecProbe.Image" "$output_dir/probe-relocated.Image"
"${cross}objdump" -dr "$output_dir/probe.elf" > "$output_dir/probe.disassembly"
"${cross}readelf" -hW -lW -rW "$output_dir/probe.elf" > "$output_dir/probe.elf-info"
python3 "$source_dir/verify_image.py" "$output_dir/PioneerKexecProbe.Image"
echo 'PASS: identical image bytes when linked at 0x2200000 and 0x80200000'
echo "Artifact: $output_dir/PioneerKexecProbe.Image"
echo 'Use the raw Image for a reviewed kexec test; the ELF is debug-only.'
