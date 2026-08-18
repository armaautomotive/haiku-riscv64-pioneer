#!/bin/sh

set -eu

usage()
{
	cat <<'EOF'
Usage: pioneer_sd_deploy.sh --device /dev/mmcblkN --payload FILE \
       --sha256 HASH [--apply] [options]

Validate, back up, deploy, and readback-verify a Pioneer Haiku BFS payload.
The script never writes the firmware/FAT partition.

Required:
  --device DEVICE       Whole SD device, for example /dev/mmcblk1.
  --payload FILE        Exact 300 MiB BFS payload from pioneer_build.sh.
  --sha256 HASH         Expected payload SHA-256.

Options:
  --apply               Perform the write. Without this, run validation only.
  --backup-dir DIR      Rollback directory (default: payload directory).
  --no-backup           Skip rollback creation (must be explicit).
  -h, --help            Show this help.

Expected layout:
  partition 1: vfat, label HAIKUPNR (validated, never written)
  partition 2: befs, label Haiku    (the only write target)
EOF
}

DEVICE=
PAYLOAD=
EXPECTED_HASH=
BACKUP_DIR=
APPLY=0
BACKUP=1
PAYLOAD_BYTES=314572800

while [ "$#" -gt 0 ]; do
	case "$1" in
		--device) shift; DEVICE=${1:?missing argument for --device} ;;
		--payload) shift; PAYLOAD=${1:?missing argument for --payload} ;;
		--sha256) shift; EXPECTED_HASH=${1:?missing argument for --sha256} ;;
		--backup-dir) shift; BACKUP_DIR=${1:?missing argument for --backup-dir} ;;
		--apply) APPLY=1 ;;
		--no-backup) BACKUP=0 ;;
		-h|--help) usage; exit 0 ;;
		*) echo "Unknown option: $1" >&2; usage >&2; exit 2 ;;
	esac
	shift
done

[ -n "$DEVICE" ] && [ -n "$PAYLOAD" ] && [ -n "$EXPECTED_HASH" ] || {
	echo "--device, --payload, and --sha256 are required" >&2
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
[ -b "$DEVICE" ] || { echo "Not a block device: $DEVICE" >&2; exit 1; }
[ -b "${DEVICE}p1" ] && [ -b "${DEVICE}p2" ] || {
	echo "Expected partitions ${DEVICE}p1 and ${DEVICE}p2" >&2
	exit 1
}
[ -f "$PAYLOAD" ] || { echo "Payload not found: $PAYLOAD" >&2; exit 1; }

ACTUAL_BYTES=$(wc -c < "$PAYLOAD" | tr -d ' ')
[ "$ACTUAL_BYTES" -eq "$PAYLOAD_BYTES" ] || {
	echo "Payload must be exactly $PAYLOAD_BYTES bytes; got $ACTUAL_BYTES" >&2
	exit 1
}

case "$EXPECTED_HASH" in
	*[!0-9a-fA-F]*|'') echo "Invalid SHA-256: $EXPECTED_HASH" >&2; exit 2 ;;
esac
[ "${#EXPECTED_HASH}" -eq 64 ] || {
	echo "SHA-256 must contain 64 hexadecimal characters" >&2
	exit 2
}
EXPECTED_HASH=$(printf '%s' "$EXPECTED_HASH" | tr 'A-F' 'a-f')
ACTUAL_HASH=$(sha256sum "$PAYLOAD" | awk '{print $1}')
[ "$ACTUAL_HASH" = "$EXPECTED_HASH" ] || {
	echo "Payload hash mismatch" >&2
	echo "Expected: $EXPECTED_HASH" >&2
	echo "Actual:   $ACTUAL_HASH" >&2
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
P2_BYTES=$(lsblk -bdnro SIZE "${DEVICE}p2")

[ "$P1_TYPE" = vfat ] && [ "$P1_LABEL" = HAIKUPNR ] || {
	echo "Partition 1 must be vfat label HAIKUPNR; got '$P1_TYPE' '$P1_LABEL'" >&2
	exit 1
}
[ "$P2_TYPE" = befs ] && [ "$P2_LABEL" = Haiku ] || {
	echo "Partition 2 must be befs label Haiku; got '$P2_TYPE' '$P2_LABEL'" >&2
	exit 1
}
[ "$P2_BYTES" -ge "$PAYLOAD_BYTES" ] || {
	echo "Partition 2 is smaller than the payload" >&2
	exit 1
}

echo "Validated Pioneer SD layout:"
lsblk -o NAME,PATH,SIZE,FSTYPE,LABEL,MOUNTPOINTS "$DEVICE"
echo "Payload SHA-256: $ACTUAL_HASH"
echo "Write target: ${DEVICE}p2"
echo "Firmware partition ${DEVICE}p1 will not be modified."

if [ "$APPLY" -ne 1 ]; then
	echo "Dry run complete. Re-run with --apply to back up and write partition 2."
	exit 0
fi

if [ "$(id -u)" -eq 0 ]; then
	SUDO=
else
	command -v sudo >/dev/null 2>&1 || {
		echo "Run as root or install sudo" >&2
		exit 1
	}
	SUDO=sudo
fi

if [ -z "$BACKUP_DIR" ]; then
	BACKUP_DIR=$(dirname -- "$PAYLOAD")
fi
[ -d "$BACKUP_DIR" ] || { echo "Backup directory not found: $BACKUP_DIR" >&2; exit 1; }

MOUNTPOINT=$(findmnt -n -o TARGET -S "${DEVICE}p2" 2>/dev/null || true)
[ -z "$MOUNTPOINT" ] || $SUDO umount "${DEVICE}p2"

if [ "$BACKUP" -eq 1 ]; then
	AVAILABLE=$(df -PB1 "$BACKUP_DIR" | awk 'NR == 2 {print $4}')
	REQUIRED=$((PAYLOAD_BYTES + 67108864))
	[ "$AVAILABLE" -ge "$REQUIRED" ] || {
		echo "Insufficient conservative backup space in $BACKUP_DIR" >&2
		echo "Need at least $REQUIRED bytes; available $AVAILABLE" >&2
		echo "Choose another --backup-dir or explicitly use --no-backup." >&2
		exit 1
	}
	STAMP=$(date -u +%Y%m%dT%H%M%SZ)
	BACKUP_FILE=$BACKUP_DIR/haiku-pioneer-bfs-before-$STAMP.img.gz
	echo "Creating rollback image: $BACKUP_FILE"
	if ! $SUDO dd if="${DEVICE}p2" bs=1M count=300 status=none \
			| gzip -1 > "$BACKUP_FILE"; then
		rm -f "$BACKUP_FILE"
		echo "Rollback image creation failed; SD was not written" >&2
		exit 1
	fi
	gzip -t "$BACKUP_FILE"
	BACKUP_BYTES=$(gzip -dc "$BACKUP_FILE" | wc -c | tr -d ' ')
	[ "$BACKUP_BYTES" -eq "$PAYLOAD_BYTES" ] || {
		rm -f "$BACKUP_FILE"
		echo "Rollback image is incomplete; SD was not written" >&2
		exit 1
	}
	sha256sum "$BACKUP_FILE" > "$BACKUP_FILE.sha256"
fi

echo "Writing verified BFS payload to ${DEVICE}p2"
$SUDO dd if="$PAYLOAD" of="${DEVICE}p2" bs=1M count=300 conv=fsync status=progress
$SUDO sync

echo "Verifying complete 300 MiB readback"
READBACK_HASH=$($SUDO dd if="${DEVICE}p2" bs=1M count=300 status=none | sha256sum | awk '{print $1}')
[ "$READBACK_HASH" = "$EXPECTED_HASH" ] || {
	echo "READBACK VERIFICATION FAILED" >&2
	echo "Expected: $EXPECTED_HASH" >&2
	echo "Actual:   $READBACK_HASH" >&2
	exit 1
}

echo "Deployment verified: $READBACK_HASH"
echo "It is safe to power off before the next cold boot."
