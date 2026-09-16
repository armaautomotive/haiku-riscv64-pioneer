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
  --payload FILE        BFS image, whole MiB size matching its superblock.
  --sha256 HASH         Expected payload SHA-256.

Options:
  --loader FILE         RISC-V haiku_loader.efi to install as BOOTRISCV64.EFI.
  --loader-sha256 HASH  Expected EFI loader SHA-256.
  --firmware FILE       Repacked MilkV-Pioneer.fd containing --loader.
  --firmware-sha256 HASH
                        Expected repacked firmware SHA-256.
  --dtb FILE            Matching mango-milkv-pioneer.dtb (requires firmware).
  --dtb-sha256 HASH      Expected device-tree SHA-256.
  --firmware-only        Validate payload, but do not back up or write p2.
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
DTB=
DTB_EXPECTED_HASH=
FIRMWARE_ONLY=0
BACKUP_DIR=
APPLY=0
BACKUP=1
PAYLOAD_BYTES=
FIRMWARE_BYTES=8585216
FIRMWARE_OUTER_FFS_GUID=93fd219e729c154c8c4be77f1db2d792
FIRMWARE_LZMA_GUID=98584eee143959429d6edc7bd79403cf
HAIKU_LOADER_FFS_GUID=a144b84d771bb442a90de72331d0a142
UEFI_SHELL_FFS_GUID=83a5047c3e9e1c4fad65e05268d0b4d1
P1_TEMP_MOUNT=
FIRMWARE_VERIFY_TEMP=
SUDO=

# Read BFS geometry instead of assuming every installed filesystem is 300 MiB.
# Supports the little-endian images used by this RISC-V port; rejects ambiguity.
bfs_size()
{
	$SUDO python3 - "$1" <<'PY'
import struct
import sys

with open(sys.argv[1], "rb") as stream:
    stream.seek(512)
    header = stream.read(116)
if len(header) != 116:
    raise SystemExit("Truncated BFS superblock")
u32 = lambda offset: struct.unpack_from("<I", header, offset)[0]
if (u32(32), u32(36), u32(68), u32(112)) != (
        0x42465331, 0x42494745, 0xdd121031, 0x15b6830e):
    raise SystemExit("Not a supported little-endian BFS superblock")
block_size = u32(40)
blocks = struct.unpack_from("<q", header, 48)[0]
used = struct.unpack_from("<q", header, 56)[0]
if (block_size not in (1024, 2048, 4096, 8192) or u32(44) not in (10, 11, 12, 13)
        or (1 << u32(44)) != block_size or not 0 <= used <= blocks
        or blocks <= 0):
    raise SystemExit("Invalid BFS geometry")
print(blocks * block_size)
PY
}

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

find_guid_offset()
{
	python3 - "$1" "$2" <<'PY'
import pathlib
import sys

data = pathlib.Path(sys.argv[1]).read_bytes()
needle = bytes.fromhex(sys.argv[2])
offsets = []
start = 0
while True:
    offset = data.find(needle, start)
    if offset < 0:
        break
    offsets.append(offset)
    start = offset + 1
if len(offsets) != 1:
    raise SystemExit(
        f"expected one occurrence of GUID {sys.argv[2]}, found {len(offsets)}")
print(offsets[0])
PY
}

find_embedded_application_offset()
{
	python3 - "$1" "$HAIKU_LOADER_FFS_GUID" "$UEFI_SHELL_FFS_GUID" <<'PY'
import pathlib
import sys

data = pathlib.Path(sys.argv[1]).read_bytes()
matches = []
for value in sys.argv[2:]:
    needle = bytes.fromhex(value)
    start = 0
    while True:
        offset = data.find(needle, start)
        if offset < 0:
            break
        if (offset + 28 <= len(data) and data[offset + 18] == 0x09
                and data[offset + 27] == 0x10):
            matches.append(offset)
        start = offset + 1
if len(matches) != 1:
    raise SystemExit(
        f"expected one embedded Haiku loader or UEFI shell application, found {len(matches)}")
print(matches[0])
PY
}

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
		--dtb) shift; DTB=${1:?missing argument for --dtb} ;;
		--dtb-sha256) shift; DTB_EXPECTED_HASH=${1:?missing argument for --dtb-sha256} ;;
		--firmware-only) FIRMWARE_ONLY=1 ;;
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

[ "$FIRMWARE_ONLY" -eq 0 ] || [ -n "$FIRMWARE" ] || {
	echo '--firmware-only requires loader and firmware options' >&2
	exit 2
}
if [ -n "$DTB" ] || [ -n "$DTB_EXPECTED_HASH" ]; then
	[ -n "$FIRMWARE" ] && [ -f "$DTB" ] || {
		echo '--dtb requires an existing DTB file and firmware options' >&2
		exit 2
	}
	case "$DTB_EXPECTED_HASH" in
		*[!0-9a-fA-F]*|'') echo 'Invalid DTB SHA-256' >&2; exit 2 ;;
	esac
	[ "${#DTB_EXPECTED_HASH}" -eq 64 ] || exit 2
	DTB_EXPECTED_HASH=$(printf '%s' "$DTB_EXPECTED_HASH" | tr 'A-F' 'a-f')
	[ "$(sha256sum "$DTB" | awk '{print $1}')" = "$DTB_EXPECTED_HASH" ] || {
		echo 'DTB hash mismatch' >&2; exit 1
	}
	command -v dtc >/dev/null 2>&1 || { echo 'dtc required to validate DTB' >&2; exit 1; }
	dtc -q -I dtb -O dts "$DTB" >/dev/null
fi

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

command -v python3 >/dev/null 2>&1 || { echo 'python3 is required' >&2; exit 1; }
ACTUAL_BYTES=$(wc -c < "$PAYLOAD" | tr -d ' ')
PAYLOAD_BYTES=$(bfs_size "$PAYLOAD")
[ "$ACTUAL_BYTES" -eq "$PAYLOAD_BYTES" ] \
	&& [ "$PAYLOAD_BYTES" -ge 314572800 ] \
	&& [ $((PAYLOAD_BYTES % 1048576)) -eq 0 ] || {
	echo "Payload length must match BFS geometry and be whole MiB (at least 300 MiB)" >&2
	exit 1
}
PAYLOAD_MIB=$((PAYLOAD_BYTES / 1048576))

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
	command -v python3 >/dev/null 2>&1 || {
		echo "python3 is required to verify the firmware structure" >&2
		exit 1
	}
	FIRMWARE_OUTER_FFS_OFFSET=$(find_guid_offset \
		"$FIRMWARE" "$FIRMWARE_OUTER_FFS_GUID") || exit 1
	FIRMWARE_GUIDED_SECTION_OFFSET=$((FIRMWARE_OUTER_FFS_OFFSET + 24))
	FIRMWARE_LZMA_OFFSET=$((FIRMWARE_GUIDED_SECTION_OFFSET + 24))
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
	FIRMWARE_APPLICATION_OFFSET=$(find_embedded_application_offset \
		"$FIRMWARE_VERIFY_TEMP/dxe.raw") || exit 1
	FIRMWARE_LOADER_OFFSET=$((FIRMWARE_APPLICATION_OFFSET + 28))
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

if [ "$(id -u)" -ne 0 ]; then
	command -v sudo >/dev/null 2>&1 || { echo 'sudo required' >&2; exit 1; }
	SUDO=sudo
fi
CURRENT_FS_BYTES=$(bfs_size "${DEVICE}p2")
[ "$CURRENT_FS_BYTES" -le "$P2_BYTES" ] || {
	echo 'Existing BFS geometry exceeds partition size' >&2; exit 1
}
if [ "$FIRMWARE_ONLY" -eq 0 ] && [ "$PAYLOAD_BYTES" -lt "$CURRENT_FS_BYTES" ]; then
	echo "Refusing to shrink installed BFS from $CURRENT_FS_BYTES to $PAYLOAD_BYTES bytes" >&2
	exit 1
fi
[ $((P2_BYTES % 1048576)) -eq 0 ] || {
	echo 'Backup requires a whole-MiB partition size' >&2; exit 1
}
BACKUP_MIB=$((P2_BYTES / 1048576))

echo "Validated Pioneer SD layout:"
lsblk -o NAME,PATH,SIZE,FSTYPE,LABEL,MOUNTPOINTS "$DEVICE"
echo "Payload SHA-256: $ACTUAL_HASH"
if [ "$FIRMWARE_ONLY" -eq 1 ]; then
	echo "Firmware-only deployment: ${DEVICE}p2 will not be changed."
else
	echo "Write target: ${DEVICE}p2"
fi
[ -z "$DTB" ] || echo "Device-tree SHA-256: $DTB_EXPECTED_HASH"
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
	DTB_TARGET=$P1_MOUNTPOINT/riscv64/mango-milkv-pioneer.dtb
	if [ -n "$DTB" ]; then
		$SUDO test -f "$DTB_TARGET" || {
			echo "Existing device tree not found: $DTB_TARGET" >&2; exit 1
		}
	fi
	$SUDO test -f "$LOADER_TARGET" || {
		echo "Existing EFI loader not found: $LOADER_TARGET" >&2
		exit 1
	}
	$SUDO test -f "$FIRMWARE_TARGET" || {
		echo "Existing Pioneer firmware not found: $FIRMWARE_TARGET" >&2
		exit 1
	}
fi

if [ "$FIRMWARE_ONLY" -eq 0 ]; then
	MOUNTPOINT=$(findmnt -n -o TARGET -S "${DEVICE}p2" 2>/dev/null || true)
	[ -z "$MOUNTPOINT" ] || $SUDO umount "${DEVICE}p2"
fi

STAMP=$(date -u +%Y%m%dT%H%M%SZ)
if [ "$BACKUP" -eq 1 ]; then
	AVAILABLE=$(df -PB1 "$BACKUP_DIR" | awk 'NR == 2 {print $4}')
	REQUIRED=$((P2_BYTES + 67108864))
	[ "$FIRMWARE_ONLY" -eq 0 ] || REQUIRED=67108864
	[ "$AVAILABLE" -ge "$REQUIRED" ] || {
		echo "Insufficient conservative backup space in $BACKUP_DIR" >&2
		echo "Need at least $REQUIRED bytes; available $AVAILABLE" >&2
		echo "Choose another --backup-dir or explicitly use --no-backup." >&2
		exit 1
	}
	if [ "$FIRMWARE_ONLY" -eq 0 ]; then
	BACKUP_FILE=$BACKUP_DIR/haiku-pioneer-bfs-before-$STAMP.img.gz
	echo "Creating rollback image: $BACKUP_FILE"
	if ! $SUDO dd if="${DEVICE}p2" bs=1M count="$BACKUP_MIB" status=none \
			| gzip -1 > "$BACKUP_FILE"; then
		rm -f "$BACKUP_FILE"
		echo "Rollback image creation failed; SD was not written" >&2
		exit 1
	fi
	gzip -t "$BACKUP_FILE"
	BACKUP_BYTES=$(gzip -dc "$BACKUP_FILE" | wc -c | tr -d ' ')
	[ "$BACKUP_BYTES" -eq "$P2_BYTES" ] || {
		rm -f "$BACKUP_FILE"
		echo "Rollback image is incomplete; SD was not written" >&2
		exit 1
	}
	sha256sum "$BACKUP_FILE" > "$BACKUP_FILE.sha256"
	fi

	if [ -n "$LOADER" ]; then
		LOADER_BACKUP=$BACKUP_DIR/BOOTRISCV64-before-$STAMP.EFI
		echo "Backing up EFI loader: $LOADER_BACKUP"
		$SUDO cp "$LOADER_TARGET" "$LOADER_BACKUP"
		$SUDO sha256sum "$LOADER_BACKUP" > "$LOADER_BACKUP.sha256"
		FIRMWARE_BACKUP=$BACKUP_DIR/MilkV-Pioneer-before-$STAMP.fd
		echo "Backing up embedded-loader firmware: $FIRMWARE_BACKUP"
		$SUDO cp "$FIRMWARE_TARGET" "$FIRMWARE_BACKUP"
		$SUDO sha256sum "$FIRMWARE_BACKUP" > "$FIRMWARE_BACKUP.sha256"
		if [ -n "$DTB" ]; then
			DTB_BACKUP=$BACKUP_DIR/mango-milkv-pioneer-before-$STAMP.dtb
			$SUDO cp "$DTB_TARGET" "$DTB_BACKUP"
			$SUDO cmp "$DTB_TARGET" "$DTB_BACKUP"
			$SUDO sha256sum "$DTB_BACKUP" > "$DTB_BACKUP.sha256"
		fi
	fi
fi

if [ "$FIRMWARE_ONLY" -eq 0 ]; then
echo "Writing verified BFS payload to ${DEVICE}p2"
$SUDO dd if="$PAYLOAD" of="${DEVICE}p2" bs=1M count="$PAYLOAD_MIB" conv=fsync status=progress
$SUDO sync

echo "Verifying complete $PAYLOAD_MIB MiB readback"
READBACK_HASH=$($SUDO dd if="${DEVICE}p2" bs=1M count="$PAYLOAD_MIB" iflag=direct status=none | sha256sum | awk '{print $1}')
[ "$READBACK_HASH" = "$EXPECTED_HASH" ] || {
	echo "READBACK VERIFICATION FAILED" >&2
	echo "Expected: $EXPECTED_HASH" >&2
	echo "Actual:   $READBACK_HASH" >&2
	exit 1
}

echo "Deployment verified: $READBACK_HASH"
fi

if [ -n "$LOADER" ]; then
	echo "Staging verified standalone and embedded EFI loaders"
	$SUDO install -m 0644 "$LOADER" "$LOADER_TARGET.new"
	$SUDO install -m 0644 "$FIRMWARE" "$FIRMWARE_TARGET.new"
	if [ -n "$DTB" ]; then
		$SUDO install -m 0644 "$DTB" "$DTB_TARGET.new"
		[ "$($SUDO sha256sum "$DTB_TARGET.new" | awk '{print $1}')" = "$DTB_EXPECTED_HASH" ] || {
			echo 'DTB staging verification failed; live firmware unchanged' >&2; exit 1
		}
	fi
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
	# Both files are staged and verified before publishing. FAT cannot make
	# the pair atomic: never reboot until the entire deployment succeeds.
	[ -z "$DTB" ] || $SUDO mv "$DTB_TARGET.new" "$DTB_TARGET"
	$SUDO mv "$FIRMWARE_TARGET.new" "$FIRMWARE_TARGET"
	$SUDO sync
	if [ -n "$DTB" ]; then
		[ "$($SUDO sha256sum "$DTB_TARGET" | awk '{print $1}')" = "$DTB_EXPECTED_HASH" ] || {
			echo 'DTB readback verification failed; do not reboot' >&2; exit 1
		}
		echo "Device-tree deployment verified: $DTB_EXPECTED_HASH"
	fi
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

echo "Deployment complete. Shut Linux down cleanly before the next cold boot."
