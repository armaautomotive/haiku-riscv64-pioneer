#!/bin/sh
# Opt-in private RAM boot/probe image. Does not deploy, load, reboot or flash.
set -eu
[ "$#" -eq 2 ] || { printf 'Usage: %s CONFIGURED_BUILD_DIRECTORY OUTPUT_DIRECTORY\n' "$0" >&2; exit 2; }
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
build_dir=$(CDPATH= cd -- "$1" && pwd)
mkdir -p "$2"
output_dir=$(CDPATH= cd -- "$2" && pwd)
jam=${PIONEER_JAM:-/private/tmp/pioneer-buildtools-20261012/jam/bin.macosxarm/jam}
cross=$build_dir/cross-tools-riscv64/bin/riscv64-unknown-haiku-
[ -x "$jam" ] && [ -x "${cross}objcopy" ]
mirror=${PIONEER_BUILD_MIRROR:-$build_dir/../haiku}
for source in src/system/boot/platform/riscv/crt0.S \
 src/system/boot/platform/riscv/Jamfile \
 src/system/boot/platform/riscv/pioneer_kexec.cpp \
 src/system/boot/platform/riscv/pioneer_kexec.h \
 src/system/boot/platform/riscv/pioneer_memory.h \
 src/system/boot/platform/riscv/pioneer_smp.h \
 src/system/boot/platform/riscv/start.cpp \
 src/system/boot/platform/riscv/debug.cpp \
 src/system/boot/platform/riscv/console.cpp \
 src/system/boot/platform/riscv/devices.cpp \
 src/system/boot/platform/riscv/video.cpp \
 src/system/boot/platform/riscv/mmu.cpp \
 src/system/boot/platform/riscv/mmu.h \
 src/system/boot/platform/riscv/entry.S \
 src/system/boot/platform/riscv/smp.cpp \
 src/system/boot/platform/riscv/smp.h \
 src/system/boot/platform/riscv/fdt.cpp \
 src/system/boot/platform/riscv/fdt.h \
 src/system/kernel/messaging/KMessage.cpp \
 src/system/ldscripts/riscv64/boot_loader_riscv.ld; do
	cmp "$script_dir/../../$source" "$mirror/$source"
done
run_build()
{
	# Touch only our profile-dependent sources: Jam may otherwise reuse
	# objects built with a different command-line macro. Contents are unchanged.
	# Jam compares whole-second timestamps; avoid a same-second false no-op.
	sleep 1
	for source in crt0.S pioneer_kexec.cpp start.cpp debug.cpp console.cpp devices.cpp video.cpp mmu.cpp smp.cpp fdt.cpp entry.S; do
		touch "$mirror/src/system/boot/platform/riscv/$source"
	done
	(cd "$build_dir" && env HAIKU_REVISION=hrev99999 \
	 PATH="$build_dir/cross-tools-riscv64/bin:/usr/bin:/bin:/usr/sbin:/sbin" \
	 "$jam" -j4 "-sHAIKU_PIONEER_KEXEC_PROBE=$1" "-sHAIKU_BOOT_LOADER_BASE=$2" haiku_loader.riscv)
}
restore_generic()
{
	build_result=$?
	trap - EXIT
	# The recorded Pioneer mirror's normal base is 0x1000000. This only
	# restores shared build artifacts, never deployed media or boot settings.
	if ! run_build 0 "${PIONEER_RESTORE_LOADER_BASE:-0x1000000}"; then
		printf 'ERROR: generic loader rebuild failed; do not package shared artifacts\n' >&2
		exit 1
	fi
	generic_elf=$build_dir/objects/haiku/riscv64/release/system/boot/riscv/boot_loader_riscv
	generic_start=$("${cross}nm" -n "$generic_elf" | awk '$3 == "_riscv_start" { print "0x" $1 }')
	if [ "$((generic_start))" -ne "$((${PIONEER_RESTORE_LOADER_BASE:-0x1000000}))" ] \
		|| [ -n "$("${cross}nm" "$generic_elf" | awk '$3 == "pioneer_kexec_start" { print $1 }')" ]; then
		printf 'ERROR: shared output is still a test profile; do not package it\n' >&2
		exit 1
	fi
	printf 'Restored shared generic non-EFI build profile (probe macro disabled)\n'
	exit "$build_result"
}
trap restore_generic EXIT
# This mirror has no .git; retain the existing development build convention.
run_build 1 0x2200000
elf=$build_dir/objects/haiku/riscv64/release/system/boot/riscv/boot_loader_riscv
start=$("${cross}nm" -n "$elf" | awk '$3 == "_riscv_start" { print "0x" $1 }')
end=$("${cross}nm" -n "$elf" | awk '$3 == "_riscv_image_end" { print "0x" $1 }')
[ "$((start))" -eq 35651584 ] && [ "$((end - start))" -le 4194304 ]
[ -z "$("${cross}nm" -u "$elf")" ]
"${cross}readelf" -r "$elf" | grep -q 'There are no relocations in this file'
cp "$elf" "$output_dir/HaikuPioneerKexecProbe.elf"
# Materialize the reserved BSS/stack so the Image header exactly matches bytes.
"${cross}objcopy" -O binary --gap-fill=0 --pad-to="$end" "$elf" \
	"$output_dir/HaikuPioneerKexecProbe.Image"
"${cross}objdump" -dr "$elf" > "$output_dir/loader.disassembly"
"${cross}readelf" -h -l -r "$elf" > "$output_dir/loader.elf-info"
python3 "$script_dir/kexec_probe/verify_image.py" --max-size=0x400000 \
	"$output_dir/HaikuPioneerKexecProbe.Image"
printf 'PRIVATE OPT-IN IMAGE: fixed physical base 0x2200000; modes 1-3 halt, mode 4 permits kernel entry\n'
