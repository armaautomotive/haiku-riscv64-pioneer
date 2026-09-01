#!/bin/sh

set -eu

usage()
{
	cat <<'EOF'
Usage: pioneer_firmware_embed.sh --base MilkV-Pioneer.fd \
       --loader haiku_loader.efi --output MilkV-Pioneer.updated.fd [options]

Replace the Haiku EFI loader embedded in the Milk-V Pioneer EDK2 firmware.
The vendor DXE volume and every byte outside its outer compressed FFS allocation
are preserved. The completed firmware is decompressed again and its loader is
compared byte-for-byte with the requested input before the output is published.

Required:
  --base FILE          Known-working MilkV-Pioneer.fd used as the base.
  --loader FILE        Newly built PE-format RISC-V haiku_loader.efi.
  --output FILE        Repacked firmware output (must differ from both inputs).

Options:
  --edk2-tools DIR     Directory containing LzmaCompress, GenSec, and GenFfs.
                      Default:
                      /private/tmp/haiku-pioneer-edk2-tools/BaseTools/Source/C/bin
  -h, --help           Show this help.

This is intentionally specific to the validated Pioneer firmware layout. It
fails closed if GUIDs, offsets, sizes, compression, or readback do not match.
EOF
}

BASE=
LOADER=
OUTPUT=
EDK2_TOOLS=/private/tmp/haiku-pioneer-edk2-tools/BaseTools/Source/C/bin

FIRMWARE_BYTES=8585216
OUTER_FFS_OFFSET=53752
GUIDED_SECTION_OFFSET=53776
LZMA_OFFSET=53800
RAW_INNER_FFS_OFFSET=2906800
RAW_LOADER_OFFSET=2906828
OUTER_FFS_GUID=93fd219e729c154c8c4be77f1db2d792
LZMA_GUID=98584eee143959429d6edc7bd79403cf
INNER_LOADER_FFS_GUID=a144b84d771bb442a90de72331d0a142

while [ "$#" -gt 0 ]; do
	case "$1" in
		--base) shift; BASE=${1:?missing argument for --base} ;;
		--loader) shift; LOADER=${1:?missing argument for --loader} ;;
		--output) shift; OUTPUT=${1:?missing argument for --output} ;;
		--edk2-tools) shift; EDK2_TOOLS=${1:?missing argument for --edk2-tools} ;;
		-h|--help) usage; exit 0 ;;
		*) echo "Unknown option: $1" >&2; usage >&2; exit 2 ;;
	esac
	shift
done

[ -n "$BASE" ] && [ -n "$LOADER" ] && [ -n "$OUTPUT" ] || {
	echo "--base, --loader, and --output are required" >&2
	usage >&2
	exit 2
}
[ -f "$BASE" ] || { echo "Base firmware not found: $BASE" >&2; exit 1; }
[ -f "$LOADER" ] || { echo "EFI loader not found: $LOADER" >&2; exit 1; }
[ "$OUTPUT" != "$BASE" ] && [ "$OUTPUT" != "$LOADER" ] || {
	echo "Refusing to overwrite an input file" >&2
	exit 1
}
[ -d "$(dirname -- "$OUTPUT")" ] || {
	echo "Output directory not found: $(dirname -- "$OUTPUT")" >&2
	exit 1
}

LZMA_COMPRESS=$EDK2_TOOLS/LzmaCompress
GEN_SEC=$EDK2_TOOLS/GenSec
GEN_FFS=$EDK2_TOOLS/GenFfs
for TOOL in "$LZMA_COMPRESS" "$GEN_SEC" "$GEN_FFS"; do
	[ -x "$TOOL" ] || { echo "Required EDK2 tool not found: $TOOL" >&2; exit 1; }
done

hash_file()
{
	if command -v sha256sum >/dev/null 2>&1; then
		sha256sum "$1" | awk '{print $1}'
	else
		shasum -a 256 "$1" | awk '{print $1}'
	fi
}

bytes_hex()
{
	dd if="$1" bs=1 skip="$2" count="$3" status=none \
		| od -An -tx1 | tr -d ' \n'
}

read_le24()
{
	set -- $(dd if="$1" bs=1 skip="$2" count=3 status=none | od -An -tu1)
	echo $(($1 + $2 * 256 + $3 * 65536))
}

BASE_BYTES=$(wc -c < "$BASE" | tr -d ' ')
[ "$BASE_BYTES" -eq "$FIRMWARE_BYTES" ] || {
	echo "Base firmware must be exactly $FIRMWARE_BYTES bytes; got $BASE_BYTES" >&2
	exit 1
}
[ "$(bytes_hex "$BASE" "$OUTER_FFS_OFFSET" 16)" = "$OUTER_FFS_GUID" ] || {
	echo "Base firmware outer DXE FFS GUID does not match the validated layout" >&2
	exit 1
}
[ "$(bytes_hex "$BASE" $((GUIDED_SECTION_OFFSET + 4)) 16)" = "$LZMA_GUID" ] || {
	echo "Base firmware LZMA guided-section GUID does not match" >&2
	exit 1
}

WORK=$(mktemp -d)
cleanup()
{
	rm -rf -- "$WORK"
}
trap cleanup EXIT HUP INT TERM

GUIDED_BYTES=$(read_le24 "$BASE" "$GUIDED_SECTION_OFFSET")
[ "$GUIDED_BYTES" -gt 24 ] || { echo "Invalid guided-section size" >&2; exit 1; }
LZMA_BYTES=$((GUIDED_BYTES - 24))
dd if="$BASE" of="$WORK/base.lzma" bs=1 skip=$LZMA_OFFSET count=$LZMA_BYTES status=none
"$LZMA_COMPRESS" -d -o "$WORK/base.raw" "$WORK/base.lzma"

[ "$(bytes_hex "$WORK/base.raw" "$RAW_INNER_FFS_OFFSET" 16)" \
	= "$INNER_LOADER_FFS_GUID" ] || {
	echo "Embedded Haiku loader FFS GUID does not match" >&2
	exit 1
}
SECTION_TYPE=$(bytes_hex "$WORK/base.raw" $((RAW_INNER_FFS_OFFSET + 27)) 1)
[ "$SECTION_TYPE" = 10 ] || {
	echo "Embedded Haiku loader is not in a PE32 section" >&2
	exit 1
}
SECTION_BYTES=$(read_le24 "$WORK/base.raw" $((RAW_INNER_FFS_OFFSET + 24)))
[ "$SECTION_BYTES" -gt 4 ] || { echo "Invalid embedded PE section size" >&2; exit 1; }
LOADER_SLOT_BYTES=$((SECTION_BYTES - 4))
LOADER_BYTES=$(wc -c < "$LOADER" | tr -d ' ')
[ "$LOADER_BYTES" -le "$LOADER_SLOT_BYTES" ] || {
	echo "Loader is too large for the existing firmware PE slot" >&2
	echo "Loader: $LOADER_BYTES bytes; slot: $LOADER_SLOT_BYTES bytes" >&2
	exit 1
}

cp "$WORK/base.raw" "$WORK/updated.raw"
dd if=/dev/zero of="$WORK/updated.raw" bs=1 seek=$RAW_LOADER_OFFSET \
	count=$LOADER_SLOT_BYTES conv=notrunc status=none
dd if="$LOADER" of="$WORK/updated.raw" bs=1 seek=$RAW_LOADER_OFFSET \
	conv=notrunc status=none

"$LZMA_COMPRESS" -e -o "$WORK/updated.lzma" "$WORK/updated.raw"
"$GEN_SEC" -s EFI_SECTION_GUID_DEFINED \
	-g EE4E5898-3914-4259-9D6E-DC7BD79403CF -r PROCESSING_REQUIRED \
	-o "$WORK/updated.guided.sec" "$WORK/updated.lzma"
"$GEN_FFS" -t EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE \
	-g 9E21FD93-9C72-4C15-8C4B-E77F1DB2D792 \
	-i "$WORK/updated.guided.sec" -o "$WORK/updated.outer.ffs"
# GenFfs emits the standalone state byte 0x07. The containing firmware volume
# has erase polarity set, so the installed FFS state must be its inverse 0xf8.
printf '\370' | dd of="$WORK/updated.outer.ffs" bs=1 seek=23 conv=notrunc status=none

OLD_OUTER_BYTES=$(read_le24 "$BASE" $((OUTER_FFS_OFFSET + 20)))
OLD_OUTER_ALLOCATION=$(((OLD_OUTER_BYTES + 7) / 8 * 8))
NEW_OUTER_BYTES=$(wc -c < "$WORK/updated.outer.ffs" | tr -d ' ')
[ "$NEW_OUTER_BYTES" -le "$OLD_OUTER_ALLOCATION" ] || {
	echo "Repacked DXE FFS no longer fits its existing firmware allocation" >&2
	echo "Repacked: $NEW_OUTER_BYTES bytes; allocation: $OLD_OUTER_ALLOCATION bytes" >&2
	exit 1
}

cp "$BASE" "$WORK/candidate.fd"
LC_ALL=C dd if=/dev/zero bs=1 count=$OLD_OUTER_ALLOCATION status=none \
	| LC_ALL=C tr '\000' '\377' \
	| dd of="$WORK/candidate.fd" bs=1 seek=$OUTER_FFS_OFFSET conv=notrunc status=none
dd if="$WORK/updated.outer.ffs" of="$WORK/candidate.fd" bs=1 \
	seek=$OUTER_FFS_OFFSET conv=notrunc status=none

# Verify the bytes that will actually boot, not merely the intermediate files.
NEW_GUIDED_BYTES=$(read_le24 "$WORK/candidate.fd" "$GUIDED_SECTION_OFFSET")
NEW_LZMA_BYTES=$((NEW_GUIDED_BYTES - 24))
dd if="$WORK/candidate.fd" of="$WORK/readback.lzma" bs=1 skip=$LZMA_OFFSET \
	count=$NEW_LZMA_BYTES status=none
"$LZMA_COMPRESS" -d -o "$WORK/readback.raw" "$WORK/readback.lzma"
cmp -s "$WORK/readback.raw" "$WORK/updated.raw" || {
	echo "Firmware DXE decompression readback mismatch" >&2
	exit 1
}
dd if="$WORK/readback.raw" of="$WORK/readback-loader.efi" bs=1 \
	skip=$RAW_LOADER_OFFSET count=$LOADER_BYTES status=none
cmp -s "$WORK/readback-loader.efi" "$LOADER" || {
	echo "Embedded loader readback mismatch" >&2
	exit 1
}
CANDIDATE_BYTES=$(wc -c < "$WORK/candidate.fd" | tr -d ' ')
[ "$CANDIDATE_BYTES" -eq "$FIRMWARE_BYTES" ] || {
	echo "Candidate firmware size changed unexpectedly" >&2
	exit 1
}

install -m 0644 "$WORK/candidate.fd" "$OUTPUT.new"
mv "$OUTPUT.new" "$OUTPUT"
OUTPUT_HASH=$(hash_file "$OUTPUT")
LOADER_HASH=$(hash_file "$LOADER")
printf '%s  %s\n' "$OUTPUT_HASH" "$(basename -- "$OUTPUT")" > "$OUTPUT.sha256"

echo "Embedded loader verified: $LOADER_HASH"
echo "Repacked firmware: $OUTPUT"
echo "Repacked firmware size: $CANDIDATE_BYTES bytes"
echo "Repacked firmware SHA-256: $OUTPUT_HASH"
echo "Manifest: $OUTPUT.sha256"
