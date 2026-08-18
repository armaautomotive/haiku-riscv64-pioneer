#!/bin/sh

set -eu

usage()
{
	cat <<'EOF'
Usage: pioneer_build.sh [--kernel-only | --image] [options]

Build a Milk-V Pioneer RISC-V kernel or deployable BFS payload.

Options:
  --kernel-only        Build kernel_riscv64 only (default).
  --image              Build haiku-mmc.image and extract its BFS payload.
  --source DIR         Source checkout (default: repository containing script).
  --mirror DIR         Build source mirror (default: /Volumes/HaikuBuild/haiku).
  --output DIR         Configured build directory.
  --buildtools DIR     Haiku buildtools checkout.
  --jobs N             Parallel build jobs (default: 4).
  --payload FILE       Extracted BFS payload path.
  --no-sync            Do not synchronize the source checkout to the mirror.
  -h, --help           Show this help.

The image mode extracts only the 300 MiB BFS filesystem payload used for
/dev/mmcblk1p2. It does not create or modify an SD card.
EOF
}

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
DEFAULT_SOURCE=$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)

SOURCE_ROOT=$DEFAULT_SOURCE
BUILD_MIRROR=/Volumes/HaikuBuild/haiku
BUILD_OUTPUT=/Volumes/HaikuBuild/generated.riscv64
BUILDTOOLS=/private/tmp/haiku-buildtools
JOBS=4
MODE=kernel
SYNC_SOURCE=1
PAYLOAD=

while [ "$#" -gt 0 ]; do
	case "$1" in
		--kernel-only) MODE=kernel ;;
		--image) MODE=image ;;
		--source) shift; SOURCE_ROOT=${1:?missing argument for --source} ;;
		--mirror) shift; BUILD_MIRROR=${1:?missing argument for --mirror} ;;
		--output) shift; BUILD_OUTPUT=${1:?missing argument for --output} ;;
		--buildtools) shift; BUILDTOOLS=${1:?missing argument for --buildtools} ;;
		--jobs) shift; JOBS=${1:?missing argument for --jobs} ;;
		--payload) shift; PAYLOAD=${1:?missing argument for --payload} ;;
		--no-sync) SYNC_SOURCE=0 ;;
		-h|--help) usage; exit 0 ;;
		*) echo "Unknown option: $1" >&2; usage >&2; exit 2 ;;
	esac
	shift
done

case "$JOBS" in
	''|*[!0-9]*) echo "--jobs must be a positive integer" >&2; exit 2 ;;
esac
[ "$JOBS" -gt 0 ] || { echo "--jobs must be greater than zero" >&2; exit 2; }

[ -d "$SOURCE_ROOT/.git" ] || {
	echo "Not a Git source checkout: $SOURCE_ROOT" >&2
	exit 1
}
[ -d "$BUILD_MIRROR" ] || {
	echo "Build mirror not found: $BUILD_MIRROR" >&2
	exit 1
}
[ -d "$BUILD_OUTPUT" ] || {
	echo "Build output not found: $BUILD_OUTPUT" >&2
	exit 1
}

JAM=$BUILDTOOLS/jam/bin.macosxarm/jam
[ -x "$JAM" ] || {
	echo "Jam executable not found: $JAM" >&2
	exit 1
}

if [ "$SYNC_SOURCE" -eq 1 ]; then
	command -v rsync >/dev/null 2>&1 || {
		echo "rsync is required to update the build mirror" >&2
		exit 1
	}
	echo "Synchronizing $SOURCE_ROOT -> $BUILD_MIRROR"
	rsync -a --exclude='.git/' "$SOURCE_ROOT/" "$BUILD_MIRROR/"
fi

PATH_VALUE=$BUILD_OUTPUT/cross-tools-riscv64/bin:/usr/bin:/bin:/usr/sbin:/sbin
TARGET=kernel_riscv64
[ "$MODE" = image ] && TARGET=haiku-mmc.image

echo "Building $TARGET"
(
	cd "$BUILD_OUTPUT"
	env HAIKU_REVISION=hrev99999 PATH="$PATH_VALUE" \
		"$JAM" -j"$JOBS" "$TARGET"
)

KERNEL=$BUILD_OUTPUT/objects/haiku/riscv64/release/system/kernel/kernel_riscv64
[ -f "$KERNEL" ] || {
	echo "Kernel target was not produced: $KERNEL" >&2
	exit 1
}

echo "Kernel: $KERNEL"
echo "Kernel size: $(wc -c < "$KERNEL" | tr -d ' ') bytes"
echo "Kernel SHA-256: $(shasum -a 256 "$KERNEL" | awk '{print $1}')"

[ "$MODE" = image ] || exit 0

IMAGE=$BUILD_OUTPUT/haiku-mmc.image
[ -f "$IMAGE" ] || {
	echo "Image target was not produced: $IMAGE" >&2
	exit 1
}

if [ -z "$PAYLOAD" ]; then
	PAYLOAD=$BUILD_OUTPUT/haiku-pioneer-bfs.img
fi

# Partition 2 begins at sector 65540 in the generated image. The Haiku image
# filesystem is 614400 sectors (300 MiB), even though the physical SD partition
# is intentionally larger.
dd if="$IMAGE" of="$PAYLOAD" bs=512 skip=65540 count=614400 status=none

IMAGE_HASH=$(shasum -a 256 "$IMAGE" | awk '{print $1}')
PAYLOAD_HASH=$(shasum -a 256 "$PAYLOAD" | awk '{print $1}')
MANIFEST=$PAYLOAD.sha256
printf '%s  %s\n' "$PAYLOAD_HASH" "$(basename "$PAYLOAD")" > "$MANIFEST"

echo "Image: $IMAGE"
echo "Image SHA-256: $IMAGE_HASH"
echo "BFS payload: $PAYLOAD"
echo "BFS payload size: $(wc -c < "$PAYLOAD" | tr -d ' ') bytes"
echo "BFS payload SHA-256: $PAYLOAD_HASH"
echo "Manifest: $MANIFEST"
