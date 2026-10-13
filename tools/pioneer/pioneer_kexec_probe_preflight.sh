#!/bin/sh
# Read-only gates. Does not load/execute a kernel or change boot configuration.
set -eu
fail()
{
	printf 'BLOCKED: %s\n' "$1" >&2
	exit 1
}
[ "$#" -eq 0 ] || fail 'This preflight takes no arguments'
[ "$(uname -m)" = riscv64 ] || fail 'Run on the native Pioneer Linux system'
[ ! -d /sys/firmware/efi ] || fail 'Expected the known non-EFI SPI/Linux boot path'
[ -r /proc/device-tree/compatible ] || fail 'Missing live device-tree identity'
tr '\000' '\n' < /proc/device-tree/compatible \
	| grep -qxE 'sophgo,(sg2042|mango)' || fail 'Unexpected board compatible'
command -v findmnt >/dev/null || fail 'findmnt is required for root-disk verification'
[ "$(findmnt -n -o SOURCE /)" = /dev/nvme0n1p3 ] \
	|| fail 'Linux root is not the recorded NVMe partition'
for device in /sys/block/mmcblk*; do
	[ ! -e "$device" ] || fail 'MMC storage is present; confirm SD removal first'
done
[ -r /sys/firmware/fdt ] || fail 'Live FDT is unavailable for a reviewed handoff'
[ -r /sys/kernel/kexec_loaded ] || fail 'Kernel kexec interface is missing'
[ "$(cat /sys/kernel/kexec_loaded)" = 0 ] \
	|| fail 'A kexec image is already loaded; do not overwrite another request'
[ -r /proc/sys/kernel/kexec_load_disabled ] || fail 'Cannot verify kexec load policy'
[ "$(cat /proc/sys/kernel/kexec_load_disabled)" = 0 ] \
	|| fail 'Kernel kexec loading is disabled; do not change this security policy'
command -v kexec >/dev/null || fail 'kexec tool is missing; build/review it separately'
kernel_release=$(uname -r)
config_source=/boot/config-$kernel_release
if [ ! -r "$config_source" ]; then
	config_source=/proc/config.gz
fi
[ -r "$config_source" ] || fail 'Cannot verify the running kernel configuration'
read_config()
{
	case "$config_source" in
		*.gz) gzip -dc "$config_source" ;;
		*) cat "$config_source" ;;
	esac
}
config_lines=$(read_config | grep -E '^CONFIG_(KEXEC|KEXEC_FILE|HOTPLUG_CPU|SMP)=' || true)
printf '%s\n' "$config_lines" | grep -qx 'CONFIG_KEXEC=y' \
	|| fail 'Classic kexec support is not confirmed in this kernel'
online=$(cat /sys/devices/system/cpu/online)
if [ "$online" != 0 ]; then
	printf '%s\n' "$config_lines" | grep -qx 'CONFIG_HOTPLUG_CPU=y' \
		|| fail 'Secondary-CPU shutdown support is not confirmed'
fi
kexec --version
printf 'PASS: read-only platform/recovery gates\n'
printf 'Kernel: %s; online CPUs: %s\n' "$kernel_release" "$online"
printf '%s\n' "$config_lines"
printf 'Inspect kexec --help and Image-loader support before any load.\n'
printf 'This preflight did not load or execute anything.\n'
