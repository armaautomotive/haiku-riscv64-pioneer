#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-boot-helper.XXXXXXXX")
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
script=$directory/../pioneer_linux_boot_haiku.sh
sh -n "$script"
smp_script=$directory/../pioneer_linux_boot_haiku_smp.sh
sh -n "$smp_script"
journal_script=$directory/../pioneer_verify_samsung_journal_view.sh
sh -n "$journal_script"
if sh "$journal_script" ../unsafe > /dev/null 2>&1; then exit 1; fi
if sh "$journal_script" session extra > /dev/null 2>&1; then exit 1; fi
if sh "$smp_script" prepare session 64 memory8-journal-recovery-control > /dev/null 2>&1; then exit 1; fi
for profile in memory128-kdl-control memory128-kdl-journal-control memory128-kdl-nosd-control memory128-kdl3-nosd-control memory128-kdl3-nosd-journal-control memory128-kdl4-nosd-control memory128-kdl4-nosd-journal-control memory128-kdl5-spin-idle-nosd-control memory128-kdl5-spin-idle-nosd-journal-control; do
 if sh "$smp_script" prepare session 2 "$profile" > /dev/null 2>&1; then exit 1; fi
done
awk '
 /PIONEER_PRIVATE_PROFILE_FLAGS_BEGIN/ {copying=1; next}
 /PIONEER_PRIVATE_PROFILE_FLAGS_END/ {found=1; exit}
 copying {print}
 END {if(!found) exit 1}
' "$smp_script" > "$temporary/private_flags.inc"
for profile in active-map-control memory128-map-order-control memory128-kdl-control; do
 . "$temporary/private_flags.inc"
 [ "$private_nosd:$journal_view" = 0:0 ]
done
for profile in memory128-kdl-nosd-control memory128-kdl3-nosd-control memory128-kdl4-nosd-control memory128-kdl5-spin-idle-nosd-control; do
 . "$temporary/private_flags.inc"
 [ "$private_nosd:$journal_view" = 1:0 ]
done
for profile in memory128-kdl3-nosd-journal-control memory128-kdl4-nosd-journal-control memory128-kdl5-spin-idle-nosd-journal-control; do
 . "$temporary/private_flags.inc"
 [ "$private_nosd:$journal_view" = 1:1 ]
done
for profile in memory8-journal-recovery-control memory128-kdl-journal-control; do
 . "$temporary/private_flags.inc"
 [ "$private_nosd:$journal_view" = 0:1 ]
done
if sh "$smp_script" prepare ../unsafe 2 > /dev/null 2>&1; then exit 1; fi
if sh "$smp_script" unsupported session 64 > /dev/null 2>&1; then exit 1; fi
if sh "$smp_script" prepare session 64 unpinned > /dev/null 2>&1; then exit 1; fi
if sh "$smp_script" prepare session 64 current extra > /dev/null 2>&1; then exit 1; fi
for count in '' 0 1 3 63 65 0x40 08 -1; do
 if sh "$smp_script" prepare session "$count" > /dev/null 2>&1; then exit 1; fi
done
if sh "$script" prepare ../unsafe > /dev/null 2>&1; then exit 1; fi
if sh "$script" unsupported session > /dev/null 2>&1; then exit 1; fi
awk '
$0 == "root_manifest()" { copy = 1 }
copy { print }
copy && $0 == "}" { exit }
' "$script" > "$temporary/root_manifest.inc"
test -s "$temporary/root_manifest.inc"
. "$temporary/root_manifest.inc"
# Mock only the device/od boundary; execute the real manifest/checksum logic.
dd() { printf 'mock sector'; }
words=128
od()
{
 case " $* " in *' -v '*) :;; *) return 1;; esac
 while IFS= read -r line; do :; done
 index=0
 while [ "$index" -lt "$words" ]; do
  printf '4294967295\n'
  index=$((index + 1))
 done
}
manifest=$(root_manifest)
set -- $manifest
[ "$#" = 20 ]
index=1
for word do
 case "$index" in 8|11|14|17|20) [ "$word" = ffffff80 ];; esac
 index=$((index + 1))
done
words=127
if root_manifest > /dev/null; then exit 1; fi
words=0
if root_manifest > /dev/null; then exit 1; fi
printf 'PASS: boot helper argument guards, production root manifest/modular checksum, short-read rejection\n'
