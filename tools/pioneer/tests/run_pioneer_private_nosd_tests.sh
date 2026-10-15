#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-private-nosd.XXXXXXXX")
trap 'rm -f "$temporary/live-fdt.dtb" "$temporary/probe-fdt.dtb" "$temporary/remove.inc"; rmdir "$temporary"' EXIT HUP INT TERM
PATH=${PIONEER_FDT_TOOLS_PREFIX:-/opt/homebrew/opt/dtc}/bin:$PATH
export PATH
source_fdt=${1:-$directory/../../../generated.pioneer/haiku-kexec-loader-20261013/smoke-1/live-fdt.dtb}
cp "$source_fdt" "$temporary/live-fdt.dtb"
cp "$source_fdt" "$temporary/probe-fdt.dtb"
before=$(shasum -a 256 "$temporary/live-fdt.dtb" | cut -d ' ' -f1)
awk '
 /PIONEER_PRIVATE_NOSD_BEGIN/ {copying=1; next}
 /PIONEER_PRIVATE_NOSD_END/ {found=1; exit}
 copying {print}
 END {if(!found) exit 1}
' "$directory/../pioneer_linux_boot_haiku_smp.sh" > "$temporary/remove.inc"
run=$temporary
fail() { printf 'expected/refused: %s\n' "$1" >&2; exit 1; }
# Execute the exact production body on a disposable regular DTB only.
( . "$temporary/remove.inc" )
test "$(shasum -a 256 "$temporary/live-fdt.dtb" | cut -d ' ' -f1)" = "$before"
test "$(fdtget -t s "$temporary/live-fdt.dtb" /soc/bm-sd@704002B000 compatible)" = bitmain,bm-sd
if fdtget "$temporary/probe-fdt.dtb" /soc/bm-sd@704002B000 compatible >/dev/null 2>&1; then exit 1; fi
for property in '#address-cells' '#size-cells'; do
 test "$(fdtget -t x "$temporary/live-fdt.dtb" / "$property")" = "$(fdtget -t x "$temporary/probe-fdt.dtb" / "$property")"
done
test "$(fdtget -l "$temporary/live-fdt.dtb" /cpus)" = "$(fdtget -l "$temporary/probe-fdt.dtb" /cpus)"
test "$(fdtget -t x "$temporary/live-fdt.dtb" /cpus timebase-frequency)" = "$(fdtget -t x "$temporary/probe-fdt.dtb" /cpus timebase-frequency)"
sh "$directory/run_pioneer_memory_tests.sh" "$temporary/probe-fdt.dtb"
# Repeating removal must refuse the now-missing target rather than silently
# changing a different controller. Then check the same-inode source refusal.
if ( . "$temporary/remove.inc" ) >/dev/null 2>&1; then exit 1; fi
rm "$temporary/probe-fdt.dtb"
ln "$temporary/live-fdt.dtb" "$temporary/probe-fdt.dtb"
if ( . "$temporary/remove.inc" ) >/dev/null 2>&1; then exit 1; fi
test "$(shasum -a 256 "$temporary/live-fdt.dtb" | cut -d ' ' -f1)" = "$before"
for property in compatible reg interrupts; do
 rm -f "$temporary/probe-fdt.dtb"
 cp "$temporary/live-fdt.dtb" "$temporary/probe-fdt.dtb"
 case "$property" in
  compatible) fdtput -t s "$temporary/probe-fdt.dtb" /soc/bm-sd@704002B000 compatible bitmain,wrong;;
  reg) fdtput -t x "$temporary/probe-fdt.dtb" /soc/bm-sd@704002B000 reg 70 4002a000 0 1000;;
  interrupts) fdtput -t x "$temporary/probe-fdt.dtb" /soc/bm-sd@704002B000 interrupts 87 4;;
 esac
 candidate=$(shasum -a 256 "$temporary/probe-fdt.dtb" | cut -d ' ' -f1)
 if ( . "$temporary/remove.inc" ) >/dev/null 2>&1; then exit 1; fi
 test "$(shasum -a 256 "$temporary/probe-fdt.dtb" | cut -d ' ' -f1)" = "$candidate"
done
printf 'PASS: private no-SD removes only the identity-checked controller; original/CPU/memory data preserved; aliases refused\n'
