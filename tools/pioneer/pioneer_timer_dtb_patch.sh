#!/bin/sh

set -eu

usage()
{
	cat <<'EOF'
Usage: pioneer_timer_dtb_patch.sh --device /dev/mmcblkN \
       --expected-sha256 HASH [--apply] [--backup-dir DIR]

Add the OpenSBI-compatible "thead,c900-clint" fallback to every SG2042
CLINT MTIMER node in the Pioneer boot DTB. The original vendor compatible is
preserved. Without --apply, validate the card and report the planned change.

Options:
  --device DEVICE          Whole SD device, for example /dev/mmcblk1.
  --expected-sha256 HASH   Required SHA-256 of the unmodified boot DTB.
  --backup-dir DIR         Rollback directory (default: /mnt/ssd/haiku-deploy).
  --apply                  Back up, patch, install, and verify the DTB.
  -h, --help               Show this help.

Only riscv64/mango-milkv-pioneer.dtb on partition 1 is changed.
EOF
}

DEVICE=
EXPECTED_HASH=
BACKUP_DIR=/mnt/ssd/haiku-deploy
APPLY=0
EXPECTED_DEVICE_BYTES=255869321216

while [ "$#" -gt 0 ]; do
	case "$1" in
		--device) shift; DEVICE=${1:?missing argument for --device} ;;
		--expected-sha256) shift; EXPECTED_HASH=${1:?missing argument for --expected-sha256} ;;
		--backup-dir) shift; BACKUP_DIR=${1:?missing argument for --backup-dir} ;;
		--apply) APPLY=1 ;;
		-h|--help) usage; exit 0 ;;
		*) echo "Unknown option: $1" >&2; usage >&2; exit 2 ;;
	esac
	shift
done

[ -n "$DEVICE" ] && [ -n "$EXPECTED_HASH" ] || {
	echo "--device and --expected-sha256 are required" >&2
	usage >&2
	exit 2
}

DEVICE_NUMBER=${DEVICE#/dev/mmcblk}
[ "$DEVICE_NUMBER" != "$DEVICE" ] || {
	echo "Refusing non-mmc device: $DEVICE" >&2
	exit 1
}
case "$DEVICE_NUMBER" in
	''|*[!0-9]*) echo "Refusing malformed mmc device: $DEVICE" >&2; exit 1 ;;
esac
case "$EXPECTED_HASH" in
	*[!0-9a-fA-F]*|'') echo "Invalid SHA-256: $EXPECTED_HASH" >&2; exit 2 ;;
esac
[ "${#EXPECTED_HASH}" -eq 64 ] || {
	echo "SHA-256 must contain 64 hexadecimal characters" >&2
	exit 2
}
EXPECTED_HASH=$(printf '%s' "$EXPECTED_HASH" | tr 'A-F' 'a-f')

[ -b "$DEVICE" ] && [ -b "${DEVICE}p1" ] && [ -b "${DEVICE}p2" ] || {
	echo "Expected $DEVICE with partitions ${DEVICE}p1 and ${DEVICE}p2" >&2
	exit 1
}
DEVICE_BYTES=$(lsblk -bdnro SIZE "$DEVICE")
[ "$DEVICE_BYTES" -eq "$EXPECTED_DEVICE_BYTES" ] || {
	echo "Refusing unexpected device size: $DEVICE_BYTES" >&2
	exit 1
}

ROOT_SOURCE=$(findmnt -n -o SOURCE / 2>/dev/null || true)
case "$ROOT_SOURCE" in
	"$DEVICE"|"${DEVICE}p"*)
		echo "Refusing device that contains the running root filesystem: $DEVICE" >&2
		exit 1
		;;
esac

P1_TYPE=$(lsblk -dnro FSTYPE "${DEVICE}p1")
P1_LABEL=$(lsblk -dnro LABEL "${DEVICE}p1")
P2_TYPE=$(lsblk -dnro FSTYPE "${DEVICE}p2")
P2_LABEL=$(lsblk -dnro LABEL "${DEVICE}p2")
[ "$P1_TYPE" = vfat ] && [ "$P1_LABEL" = HAIKUPNR ] || {
	echo "Partition 1 must be vfat label HAIKUPNR; got '$P1_TYPE' '$P1_LABEL'" >&2
	exit 1
}
[ "$P2_TYPE" = befs ] && [ "$P2_LABEL" = Haiku ] || {
	echo "Partition 2 must be befs label Haiku; got '$P2_TYPE' '$P2_LABEL'" >&2
	exit 1
}

MOUNTPOINT=$(findmnt -n -o TARGET -S "${DEVICE}p1" 2>/dev/null || true)
[ -n "$MOUNTPOINT" ] || {
	echo "Partition ${DEVICE}p1 must be mounted" >&2
	exit 1
}
if [ "$(id -u)" -eq 0 ]; then
	SUDO=
else
	command -v sudo >/dev/null 2>&1 || { echo "Run as root or install sudo" >&2; exit 1; }
	SUDO=sudo
fi
DTB_PATH=$MOUNTPOINT/riscv64/mango-milkv-pioneer.dtb
$SUDO test -f "$DTB_PATH" || { echo "Boot DTB not found: $DTB_PATH" >&2; exit 1; }
command -v fdtget >/dev/null
command -v fdtput >/dev/null
command -v dtc >/dev/null

ACTUAL_HASH=$($SUDO sha256sum "$DTB_PATH" | awk '{print $1}')
[ "$ACTUAL_HASH" = "$EXPECTED_HASH" ] || {
	echo "Boot DTB hash mismatch; refusing to patch" >&2
	echo "Expected: $EXPECTED_HASH" >&2
	echo "Actual:   $ACTUAL_HASH" >&2
	exit 1
}

for SUFFIX in 0 1 2 3 4 5 6 7 8 9 a b c d e f; do
	NODE=/soc/clint-mtimer@70ac0${SUFFIX}0000
	COMPAT=$($SUDO fdtget "$DTB_PATH" "$NODE" compatible)
	[ "$COMPAT" = "thead,c900-clint-mtimer" ] || {
		echo "Unexpected compatible at $NODE: $COMPAT" >&2
		exit 1
	}
done

echo "Validated Pioneer SD and 16 SG2042 timer nodes"
echo "Boot DTB SHA-256: $ACTUAL_HASH"
echo "Planned fallback: thead,c900-clint"
if [ "$APPLY" -ne 1 ]; then
	echo "Dry run complete. Re-run with --apply to back up and patch the DTB."
	exit 0
fi

[ -d "$BACKUP_DIR" ] || { echo "Backup directory not found: $BACKUP_DIR" >&2; exit 1; }
STAMP=$(date -u +%Y%m%dT%H%M%SZ)
BACKUP_FILE=$BACKUP_DIR/mango-milkv-pioneer-before-timer-fix-$STAMP.dtb
WORK_FILE=$BACKUP_DIR/mango-milkv-pioneer-timer-fix-$STAMP.dtb
$SUDO cp "$DTB_PATH" "$BACKUP_FILE"
$SUDO cp "$DTB_PATH" "$WORK_FILE"
$SUDO chown "$(id -u):$(id -g)" "$BACKUP_FILE" "$WORK_FILE"
sha256sum "$BACKUP_FILE" > "$BACKUP_FILE.sha256"

for SUFFIX in 0 1 2 3 4 5 6 7 8 9 a b c d e f; do
	NODE=/soc/clint-mtimer@70ac0${SUFFIX}0000
	fdtput -t s "$WORK_FILE" "$NODE" compatible \
		thead,c900-clint-mtimer thead,c900-clint
done
dtc -q -I dtb -O dts -o /dev/null "$WORK_FILE"
for SUFFIX in 0 1 2 3 4 5 6 7 8 9 a b c d e f; do
	NODE=/soc/clint-mtimer@70ac0${SUFFIX}0000
	[ "$(fdtget "$WORK_FILE" "$NODE" compatible)" = \
		"thead,c900-clint-mtimer thead,c900-clint" ] || {
		echo "Patched compatible verification failed at $NODE" >&2
		exit 1
	}
done

PATCHED_HASH=$(sha256sum "$WORK_FILE" | awk '{print $1}')
INSTALL_TEMP=$DTB_PATH.timer-fix-new
$SUDO cp "$WORK_FILE" "$INSTALL_TEMP"
$SUDO sync
TEMP_HASH=$($SUDO sha256sum "$INSTALL_TEMP" | awk '{print $1}')
[ "$TEMP_HASH" = "$PATCHED_HASH" ] || {
	$SUDO rm -f "$INSTALL_TEMP"
	echo "Temporary DTB readback verification failed" >&2
	exit 1
}
$SUDO mv "$INSTALL_TEMP" "$DTB_PATH"
$SUDO sync
INSTALLED_HASH=$($SUDO sha256sum "$DTB_PATH" | awk '{print $1}')
[ "$INSTALLED_HASH" = "$PATCHED_HASH" ] || {
	echo "Installed DTB readback verification failed" >&2
	exit 1
}

echo "Timer DTB patch installed and verified: $INSTALLED_HASH"
echo "Rollback copy: $BACKUP_FILE"
