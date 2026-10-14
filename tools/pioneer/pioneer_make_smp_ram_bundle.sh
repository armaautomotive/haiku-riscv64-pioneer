#!/bin/sh
# Recorded native Linux only. Fresh private RAM bundle; no Samsung/SD writes.
set -eu
test "$(id -u)" = 0
test "$(uname -s):$(uname -m)" = Linux:riscv64
test "$(findmnt -n -o SOURCE /mnt/ssd)" = /dev/sdb2
base=/mnt/ssd/pioneer-haiku-kexec-20261013
case "${1:-current}" in
 current)
  run=$base/smp-kernel-bundle-1
  candidate=$base/smp-kernel-1/kernel_riscv64
  kernel_hash=cd02cdb537a41641a1eb6442abd6cfd083614243b85a7841ebf824291e8013e3
  ;;
 validated-control)
  # Extracted from the SHA-pinned, previously SD-tested September 8 image.
  # The packaged kernel is revision-stamped, unlike the old build-object hash.
  run=$base/smp-kernel-bundle-validated-1
  candidate=$base/smp-kernel-validated-1/kernel_riscv64
  kernel_hash=660147f6ad79cdbe1d1f60b4abffeb55b9ed655d5617959820bf7ddfa62cd2d8
  ;;
 capped-control)
  run=$base/smp-kernel-bundle-capped-1
  candidate=$base/smp-kernel-capped-1/kernel_riscv64
  kernel_hash=beff31ad047033dc0b76685aaaffe0f6fe53c94d07442efb1d09f9caea1cceac
  ;;
 active-map-control)
  run=$base/smp-kernel-bundle-active-map-1
  candidate=$base/smp-kernel-active-map-1/kernel_riscv64
  kernel_hash=d7aa573407757c25402793bb24c576f465982778966ce38eea26541936ae8c0c
  ;;
 memory-control)
  run=$base/smp-kernel-bundle-memory-1
  candidate=$base/smp-kernel-memory-1/kernel_riscv64
  kernel_hash=1dca50d6976fb4629f0eca35be7c81305408ec4a053882d37135c3ef2ca99c91
  ;;
 memory-map-order-control)
  run=$base/smp-kernel-bundle-memory-map-order-1
  candidate=$base/smp-kernel-memory-map-order-1/kernel_riscv64
  kernel_hash=dab2171f6d7d80e2a2879eb7ffd2d6c501eb5066a3da9e83b3398cf659174e7f
  ;;
 *) echo 'Usage: pioneer_make_smp_ram_bundle.sh [current|validated-control|capped-control|active-map-control|memory-control|memory-map-order-control]' >&2; exit 2;;
esac
[ "$#" -le 1 ]
source=$base/p100a-root-probe-2/samsung-private.hpkg
host=/mnt/ssd/pioneer-xhci-rollback-20261012/host/objects/linux/riscv64/release/tools
package=$host/package/package
bfs=$host/bfs_shell/bfs_shell
entry=kernel_riscv64
ecam=add-ons/kernel/busses/pci/ecam
hash_is()
{
 test -f "$1" && test ! -L "$1"
 test "$(sha256sum "$1" | cut -d ' ' -f1)" = "$2"
}
hash_is "$source" 7a830442362ba511bdfc08f6d99e26df5813475861d3583a90cb62cbef1f3cea
hash_is "$candidate" "$kernel_hash"
test ! -e "$run" && test ! -L "$run"
umask 077
mkdir -m 700 "$run"
mkdir -p "$run/replacement" "$run/verify/add-ons/kernel/busses/pci"
cp "$candidate" "$run/replacement/$entry"
chmod 755 "$run/replacement/$entry"
cp "$source" "$run/samsung-private.hpkg"
"$package" list -p "$source" > "$run/original.paths"
test "$(grep -cx "$entry" "$run/original.paths")" = 1
"$package" add -1 -f -C "$run/replacement" "$run/samsung-private.hpkg" "$entry" > "$run/package-add.log" 2>&1
"$package" list -p "$run/samsung-private.hpkg" > "$run/candidate.paths"
LC_ALL=C sort "$run/original.paths" > "$run/original.sorted"
LC_ALL=C sort "$run/candidate.paths" > "$run/candidate.sorted"
cmp "$run/original.sorted" "$run/candidate.sorted"
# Linux can reject Haiku attributes after extracting complete file data.
"$package" extract -C "$run/verify" "$run/samsung-private.hpkg" "$entry" > "$run/extract-kernel.log" 2>&1 || extract_status=$?
"$package" extract -C "$run/verify" "$run/samsung-private.hpkg" "$ecam" > "$run/extract-ecam.log" 2>&1 || extract_status=$?
hash_is "$run/verify/$entry" "$kernel_hash"
hash_is "$run/verify/$ecam" 23e65e2a9a3e886fc7d1429901db6534f2cc7a9077f121d3e334f4cf839cf11d
hash_is "$source" 7a830442362ba511bdfc08f6d99e26df5813475861d3583a90cb62cbef1f3cea
image=$run/samsung-ram.bfs
truncate -s 314572800 "$image"
test -f "$image" && test ! -L "$image"
"$bfs" --initialize "$image" PioneerSMPKernel 'block_size 2048' > "$run/initialize.log" 2>&1
printf '%s\n' 'mkdir /myfs/system' 'mkdir /myfs/system/packages' \
 "cp :$run/samsung-private.hpkg /myfs/system/packages/haiku-r1~beta6_hrev99999-1-riscv64.hpkg" \
 'sync' 'checkfs -c' \
 "cp /myfs/system/packages/haiku-r1~beta6_hrev99999-1-riscv64.hpkg :$run/roundtrip.hpkg" \
 'sync' > "$run/commands"
"$bfs" -n "$image" 3< "$run/commands" 4> "$run/replies" > "$run/populate.log" 2>&1
test "$(wc -l < "$run/replies")" -eq 7
test "$(sort -u "$run/replies")" = 0
cmp "$run/samsung-private.hpkg" "$run/roundtrip.hpkg"
sha256sum "$image" "$run/samsung-private.hpkg" "$run/verify/$entry" "$run/verify/$ecam"
printf 'PRIVATE SMP RAM BUNDLE PASS: kernel replaced; package paths/root-opt-in ECAM preserved\n'
