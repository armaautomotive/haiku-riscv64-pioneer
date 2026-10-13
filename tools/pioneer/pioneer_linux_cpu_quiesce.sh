#!/bin/sh
# Temporary runtime diagnostic. No persistent boot configuration or kexec load.
set -eu
fail()
{
	printf 'BLOCKED: %s\n' "$1" >&2
	exit 1
}
[ "$#" -eq 1 ] || fail 'Usage: pioneer_linux_cpu_quiesce.sh status|quiesce|restore'
case "$1" in status|quiesce|restore) ;; *) fail 'Unknown operation' ;; esac
[ "$(uname -m)" = riscv64 ] || fail 'Run on native Pioneer Linux'
[ ! -d /sys/firmware/efi ] || fail 'Unexpected EFI boot path'
[ "$(findmnt -n -o SOURCE /)" = /dev/nvme0n1p3 ] || fail 'Unexpected root disk'
tr '\000' '\n' < /proc/device-tree/compatible \
	| grep -qxE 'sophgo,(sg2042|mango)' || fail 'Unexpected board identity'
for device in /sys/block/mmcblk*; do
	[ ! -e "$device" ] || fail 'MMC storage present; confirm SD removal'
done
cpu_root=/sys/devices/system/cpu
[ "$(cat /sys/kernel/kexec_loaded)" = 0 ] || fail 'Do not alter CPUs with an image loaded'
[ -d "$cpu_root/cpu0" ] && [ ! -d "$cpu_root/cpu64" ] || fail 'Unexpected CPU topology'
if [ "$1" = status ]; then
	cat "$cpu_root/online"
	exit 0
fi
[ "$(id -u)" = 0 ] || fail 'CPU state changes require root'
command -v timeout >/dev/null || fail 'timeout is required'
[ "$(cat "$cpu_root/online")" != '' ] || fail 'Missing online CPU mask'
for cpu_number in $(seq 1 63); do
	[ -w "$cpu_root/cpu$cpu_number/online" ] || fail 'Missing CPU hotplug control'
done
restore_cpus()
{
	for cpu_number in $(seq 1 63); do
		cpu_file=$cpu_root/cpu$cpu_number/online
		if [ "$(cat "$cpu_file")" = 0 ]; then
			timeout --kill-after=5 15 sh -c 'printf 1 > "$1"' sh "$cpu_file" || return 1
		fi
	done
	[ "$(cat "$cpu_root/online")" = 0-63 ]
}
if [ "$1" = restore ]; then
	case "$(cat "$cpu_root/online")" in
		0|0-63) ;;
		*) fail 'Restore accepts only the owned single-CPU state or the full mask' ;;
	esac
	restore_cpus || fail 'CPU restore failed; use the approved recovery plan'
	printf 'PASS: restored CPUs 0-63\n'
	exit 0
fi
[ "$(cat "$cpu_root/online")" = 0-63 ] || fail 'Quiesce requires the original full CPU mask'
# The initial 64-CPU gate establishes which CPU state this operation owns.
# Keep CPU 0 online. Roll back on errors/signals; a stuck kernel operation may
# still need relay recovery. An EXIT trap cannot repair an unresponsive kernel.
trap 'trap - EXIT HUP INT TERM; restore_cpus || true; exit 1' EXIT HUP INT TERM
sync
for cpu_number in $(seq 1 63); do
	cpu_file=$cpu_root/cpu$cpu_number/online
	timeout --kill-after=5 15 sh -c 'printf 0 > "$1"' sh "$cpu_file"
	[ "$(cat "$cpu_file")" = 0 ] || fail 'CPU did not go offline'
	printf 'CPU %s off\n' "$cpu_number"
done
[ "$(cat "$cpu_root/online")" = 0 ] || fail 'Expected only CPU 0 online'
trap - EXIT HUP INT TERM
printf 'PASS: only CPU 0 online; temporary state retained for the reviewed test\n'
printf 'Restore CPUs or recover before leaving Linux for ordinary use.\n'
