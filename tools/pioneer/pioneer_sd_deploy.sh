#!/bin/sh

set -eu

usage()
{
	cat <<'EOF'
Usage: pioneer_sd_deploy.sh --device /dev/mmcblkN --payload FILE \
       --sha256 HASH [--loader FILE --loader-sha256 HASH \
       --firmware FILE --firmware-sha256 HASH] [--apply] [options]

Validate, back up, deploy, and readback-verify a Pioneer Haiku BFS payload.
Optionally update the RISC-V EFI loader in both its standalone and embedded
firmware locations on the FAT partition.

Required:
  --device DEVICE       Whole SD device, for example /dev/mmcblk1.
  --payload FILE        Exact 300 MiB BFS payload from pioneer_build.sh.
  --sha256 HASH         Expected payload SHA-256.

Options:
  --loader FILE         RISC-V haiku_loader.efi to install as BOOTRISCV64.EFI.
  --loader-sha256 HASH  Expected EFI loader SHA-256.
  --firmware FILE       Repacked MilkV-Pioneer.fd containing --loader.
  --firmware-sha256 HASH
                        Expected repacked firmware SHA-256.
  --apply               Perform the write. Without this, run validation only.
  --backup-dir DIR      Rollback directory (default: payload directory).
  --no-backup           Skip rollback creation (must be explicit).
  -h, --help            Show this help.

Expected layout:
  partition 1: vfat, label HAIKUPNR
               EFI/BOOT/BOOTRISCV64.EFI
               riscv64/MilkV-Pioneer.fd (contains the loader actually booted)
  partition 2: befs, label Haiku    (the BFS payload write target)

For safety, loader and firmware options are inseparable. The script rejects a
firmware whose embedded PE loader does not exactly match --loader-sha256.
EOF
}

DEVICE=
PAYLOAD=
EXPECTED_HASH=
LOADER=
LOADER_EXPECTED_HASH=
FIRMWARE=
FIRMWARE_EXPECTED_HASH=
BACKUP_DIR=
APPLY=0
BACKUP=1
PAYLOAD_BYTES=314572800
FIRMWARE_BYTES=8585216
FIRMWARE_OUTER_FFS_OFFSET=53752
FIRMWARE_GUIDED_SECTION_OFFSET=53776
FIRMWARE_LZMA_OFFSET=53800
FIRMWARE_LOADER_OFFSET=2906828
FIRMWARE_OUTER_FFS_GUID=93fd219e729c154c8c4be77f1db2d792
FIRMWARE_LZMA_GUID=98584eee143959429d6edc7bd79403cf
P1_TEMP_MOUNT=
FIRMWARE_VERIFY_TEMP=
SUDO=

cleanup()
{
	if [ -n "$P1_TEMP_MOUNT" ]; then
		$SUDO umount "$P1_TEMP_MOUNT" >/dev/null 2>&1 || true
		rmdir "$P1_TEMP_MOUNT" >/dev/null 2>&1 || true
	fi
	if [ -n "$FIRMWARE_VERIFY_TEMP" ]; then
		rm -rf -- "$FIRMWARE_VERIFY_TEMP"
	fi
}
trap cleanup EXIT HUP INT TERM

while [ "$#" -gt 0 ]; do
	case "$1" in
		--device) shift; DEVICE=${1:?missing argument for --device} ;;
		--payload) shift; PAYLOAD=${1:?missing argument for --payload} ;;
		--sha256) shift; EXPECTED_HASH=${1:?missing argument for --sha256} ;;
		--backup-dir) shift; BACKUP_DIR=${1:?missing argument for --backup-dir} ;;
		--loader) shift; LOADER=${1:?missing argument for --loader} ;;
		--loader-sha256) shift; LOADER_EXPECTED_HASH=${1:?missing argument for --loader-sha256} ;;
		--firmware) shift; FIRMWARE=${1:?missing argument for --firmware} ;;
		--firmware-sha256) shift; FIRMWARE_EXPECTED_HASH=${1:?missing argument for --firmware-sha256} ;;
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

[ -z "$LOADER" ] && [ -z "$LOADER_EXPECTED_HASH" ] \
	&& [ -z "$FIRMWARE" ] && [ -z "$FIRMWARE_EXPECTED_HASH" ] || {
	[ -n "$LOADER" ] && [ -n "$LOADER_EXPECTED_HASH" ] \
		&& [ -n "$FIRMWARE" ] && [ -n "$FIRMWARE_EXPECTED_HASH" ] || {
		echo "--loader, --loader-sha256, --firmware, and --firmware-sha256 must be specified together" >&2
		exit 2
	}
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

LOADER_ACTUAL_HASH=
FIRMWARE_ACTUAL_HASH=
if [ -n "$LOADER" ]; then
	[ -f "$LOADER" ] || { echo "EFI loader not found: $LOADER" >&2; exit 1; }
	case "$LOADER_EXPECTED_HASH" in
		*[!0-9a-fA-F]*|'') echo "Invalid loader SHA-256: $LOADER_EXPECTED_HASH" >&2; exit 2 ;;
	esac
	LOADER_HASH_LENGTH=$(printf '%s' "$LOADER_EXPECTED_HASH" | wc -c | tr -d ' ')
	[ "$LOADER_HASH_LENGTH" -eq 64 ] || {
		echo "Loader SHA-256 must contain 64 hexadecimal characters" >&2
		exit 2
	}
	LOADER_EXPECTED_HASH=$(printf '%s' "$LOADER_EXPECTED_HASH" | tr 'A-F' 'a-f')
	LOADER_ACTUAL_HASH=$(sha256sum "$LOADER" | awk '{print $1}')
	[ "$LOADER_ACTUAL_HASH" = "$LOADER_EXPECTED_HASH" ] || {
		echo "EFI loader hash mismatch" >&2
		echo "Expected: $LOADER_EXPECTED_HASH" >&2
		echo "Actual:   $LOADER_ACTUAL_HASH" >&2
		exit 1
	}

	[ -f "$FIRMWARE" ] || { echo "Firmware not found: $FIRMWARE" >&2; exit 1; }
	case "$FIRMWARE_EXPECTED_HASH" in
		*[!0-9a-fA-F]*|'') echo "Invalid firmware SHA-256: $FIRMWARE_EXPECTED_HASH" >&2; exit 2 ;;
	esac
	FIRMWARE_HASH_LENGTH=$(printf '%s' "$FIRMWARE_EXPECTED_HASH" | wc -c | tr -d ' ')
	[ "$FIRMWARE_HASH_LENGTH" -eq 64 ] || {
		echo "Firmware SHA-256 must contain 64 hexadecimal characters" >&2
		exit 2
	}
	FIRMWARE_EXPECTED_HASH=$(printf '%s' "$FIRMWARE_EXPECTED_HASH" | tr 'A-F' 'a-f')
	FIRMWARE_ACTUAL_HASH=$(sha256sum "$FIRMWARE" | awk '{print $1}')
	[ "$FIRMWARE_ACTUAL_HASH" = "$FIRMWARE_EXPECTED_HASH" ] || {
		echo "Firmware hash mismatch" >&2
		echo "Expected: $FIRMWARE_EXPECTED_HASH" >&2
		echo "Actual:   $FIRMWARE_ACTUAL_HASH" >&2
		exit 1
	}
	ACTUAL_FIRMWARE_BYTES=$(wc -c < "$FIRMWARE" | tr -d ' ')
	[ "$ACTUAL_FIRMWARE_BYTES" -eq "$FIRMWARE_BYTES" ] || {
		echo "Firmware must be exactly $FIRMWARE_BYTES bytes; got $ACTUAL_FIRMWARE_BYTES" >&2
		exit 1
	}

	command -v xz >/dev/null 2>&1 || {
		echo "xz is required to verify the firmware's embedded loader" >&2
		exit 1
	}
	OUTER_GUID=$(dd if="$FIRMWARE" bs=1 skip=$FIRMWARE_OUTER_FFS_OFFSET count=16 status=none \
		| od -An -tx1 | tr -d ' \n')
	GUIDED_GUID=$(dd if="$FIRMWARE" bs=1 skip=$((FIRMWARE_GUIDED_SECTION_OFFSET + 4)) \
		count=16 status=none | od -An -tx1 | tr -d ' \n')
	[ "$OUTER_GUID" = "$FIRMWARE_OUTER_FFS_GUID" ] || {
		echo "Unexpected outer DXE FFS GUID in firmware" >&2
		exit 1
	}
	[ "$GUIDED_GUID" = "$FIRMWARE_LZMA_GUID" ] || {
		echo "Unexpected LZMA guided-section GUID in firmware" >&2
		exit 1
	}
	set -- $(dd if="$FIRMWARE" bs=1 skip=$FIRMWARE_GUIDED_SECTION_OFFSET count=3 \
		status=none | od -An -tu1)
	GUIDED_SECTION_BYTES=$(($1 + $2 * 256 + $3 * 65536))
	[ "$GUIDED_SECTION_BYTES" -gt 24 ] || {
		echo "Invalid guided-section size in firmware" >&2
		exit 1
	}
	FIRMWARE_LZMA_BYTES=$((GUIDED_SECTION_BYTES - 24))
	FIRMWARE_VERIFY_TEMP=$(mktemp -d)
	dd if="$FIRMWARE" of="$FIRMWARE_VERIFY_TEMP/dxe.lzma" bs=1 \
		skip=$FIRMWARE_LZMA_OFFSET count=$FIRMWARE_LZMA_BYTES status=none
	xz --format=lzma --decompress --stdout "$FIRMWARE_VERIFY_TEMP/dxe.lzma" \
		> "$FIRMWARE_VERIFY_TEMP/dxe.raw"
	LOADER_BYTES=$(wc -c < "$LOADER" | tr -d ' ')
	dd if="$FIRMWARE_VERIFY_TEMP/dxe.raw" of="$FIRMWARE_VERIFY_TEMP/embedded-loader.efi" \
		bs=1 skip=$FIRMWARE_LOADER_OFFSET count=$LOADER_BYTES status=none
	EMBEDDED_LOADER_BYTES=$(wc -c < "$FIRMWARE_VERIFY_TEMP/embedded-loader.efi" | tr -d ' ')
	[ "$EMBEDDED_LOADER_BYTES" -eq "$LOADER_BYTES" ] || {
		echo "Embedded loader extraction was incomplete" >&2
		exit 1
	}
	EMBEDDED_LOADER_HASH=$(sha256sum "$FIRMWARE_VERIFY_TEMP/embedded-loader.efi" | awk '{print $1}')
	[ "$EMBEDDED_LOADER_HASH" = "$LOADER_EXPECTED_HASH" ] || {
		echo "FIRMWARE EMBEDDED LOADER MISMATCH" >&2
		echo "Expected loader: $LOADER_EXPECTED_HASH" >&2
		echo "Embedded loader: $EMBEDDED_LOADER_HASH" >&2
		exit 1
	}
fi

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
if [ -n "$LOADER" ]; then
	echo "EFI loader SHA-256: $LOADER_ACTUAL_HASH"
	echo "Embedded EFI loader SHA-256: $EMBEDDED_LOADER_HASH"
	echo "Firmware SHA-256: $FIRMWARE_ACTUAL_HASH"
	echo "Standalone loader target: ${DEVICE}p1:/EFI/BOOT/BOOTRISCV64.EFI"
	echo "Booted firmware target: ${DEVICE}p1:/riscv64/MilkV-Pioneer.fd"
else
	echo "Firmware partition ${DEVICE}p1 will not be modified."
fi

if [ "$APPLY" -ne 1 ]; then
	echo "Dry run complete. Re-run with --apply to back up and deploy."
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

P1_MOUNTPOINT=
if [ -n "$LOADER" ]; then
	P1_MOUNTPOINT=$(findmnt -n -o TARGET -S "$DEVICE"p1 2>/dev/null || true)
	if [ -z "$P1_MOUNTPOINT" ]; then
		P1_TEMP_MOUNT=$(mktemp -d)
		$SUDO mount "$DEVICE"p1 "$P1_TEMP_MOUNT"
		P1_MOUNTPOINT=$P1_TEMP_MOUNT
	fi
	LOADER_TARGET=$P1_MOUNTPOINT/EFI/BOOT/BOOTRISCV64.EFI
	FIRMWARE_TARGET=$P1_MOUNTPOINT/riscv64/MilkV-Pioneer.fd
	$SUDO test -f "$LOADER_TARGET" || {
		echo "Existing EFI loader not found: $LOADER_TARGET" >&2
		exit 1
	}
	$SUDO test -f "$FIRMWARE_TARGET" || {
		echo "Existing Pioneer firmware not found: $FIRMWARE_TARGET" >&2
		exit 1
	}
fi

MOUNTPOINT=$(findmnt -n -o TARGET -S "${DEVICE}p2" 2>/dev/null || true)
[ -z "$MOUNTPOINT" ] || $SUDO umount "${DEVICE}p2"

STAMP=$(date -u +%Y%m%dT%H%M%SZ)
if [ "$BACKUP" -eq 1 ]; then
	AVAILABLE=$(df -PB1 "$BACKUP_DIR" | awk 'NR == 2 {print $4}')
	REQUIRED=$((PAYLOAD_BYTES + 67108864))
	[ "$AVAILABLE" -ge "$REQUIRED" ] || {
		echo "Insufficient conservative backup space in $BACKUP_DIR" >&2
		echo "Need at least $REQUIRED bytes; available $AVAILABLE" >&2
		echo "Choose another --backup-dir or explicitly use --no-backup." >&2
		exit 1
	}
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

	if [ -n "$LOADER" ]; then
		LOADER_BACKUP=$BACKUP_DIR/BOOTRISCV64-before-$STAMP.EFI
		echo "Backing up EFI loader: $LOADER_BACKUP"
		$SUDO cp "$LOADER_TARGET" "$LOADER_BACKUP"
		$SUDO sha256sum "$LOADER_BACKUP" > "$LOADER_BACKUP.sha256"
		FIRMWARE_BACKUP=$BACKUP_DIR/MilkV-Pioneer-before-$STAMP.fd
		echo "Backing up embedded-loader firmware: $FIRMWARE_BACKUP"
		$SUDO cp "$FIRMWARE_TARGET" "$FIRMWARE_BACKUP"
		$SUDO sha256sum "$FIRMWARE_BACKUP" > "$FIRMWARE_BACKUP.sha256"
	fi
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

if [ -n "$LOADER" ]; then
	echo "Staging verified standalone and embedded EFI loaders"
	$SUDO install -m 0644 "$LOADER" "$LOADER_TARGET.new"
	$SUDO install -m 0644 "$FIRMWARE" "$FIRMWARE_TARGET.new"
	LOADER_TEMP_HASH=$($SUDO sha256sum "$LOADER_TARGET.new" | awk '{print $1}')
	FIRMWARE_TEMP_HASH=$($SUDO sha256sum "$FIRMWARE_TARGET.new" | awk '{print $1}')
	if [ "$LOADER_TEMP_HASH" != "$LOADER_EXPECTED_HASH" ] \
		|| [ "$FIRMWARE_TEMP_HASH" != "$FIRMWARE_EXPECTED_HASH" ]; then
		$SUDO rm -f "$LOADER_TARGET.new"
		$SUDO rm -f "$FIRMWARE_TARGET.new"
		echo "EFI LOADER/FIRMWARE STAGING VERIFICATION FAILED" >&2
		exit 1
	fi
	$SUDO mv "$LOADER_TARGET.new" "$LOADER_TARGET"
	$SUDO mv "$FIRMWARE_TARGET.new" "$FIRMWARE_TARGET"
	$SUDO sync
	LOADER_READBACK_HASH=$($SUDO sha256sum "$LOADER_TARGET" | awk '{print $1}')
	FIRMWARE_READBACK_HASH=$($SUDO sha256sum "$FIRMWARE_TARGET" | awk '{print $1}')
	[ "$LOADER_READBACK_HASH" = "$LOADER_EXPECTED_HASH" ] || {
		echo "EFI LOADER READBACK VERIFICATION FAILED" >&2
		echo "Expected: $LOADER_EXPECTED_HASH" >&2
		echo "Actual:   $LOADER_READBACK_HASH" >&2
		exit 1
	}
	[ "$FIRMWARE_READBACK_HASH" = "$FIRMWARE_EXPECTED_HASH" ] || {
		echo "FIRMWARE READBACK VERIFICATION FAILED" >&2
		echo "Expected: $FIRMWARE_EXPECTED_HASH" >&2
		echo "Actual:   $FIRMWARE_READBACK_HASH" >&2
		exit 1
	}
	echo "Standalone EFI loader deployment verified: $LOADER_READBACK_HASH"
	echo "Embedded-loader firmware deployment verified: $FIRMWARE_READBACK_HASH"
fi

echo "It is safe to power off before the next cold boot."
