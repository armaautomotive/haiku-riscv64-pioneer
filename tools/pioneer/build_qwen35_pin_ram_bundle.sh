#!/bin/sh
# Prepare a private RAM boot bundle only. Never writes target disks or boots it.
set -eu
[ "$#" = 5 ] || { echo 'usage: build_qwen35_pin_ram_bundle.sh BUILD BASE_HPKG CANDIDATE_KERNEL ROOT_ECAM NEW_OUTPUT' >&2; exit 2; }
build=$(CDPATH= cd -- "$1" && pwd)
source=$2
kernel=$3
ecam=$4
output=$5
case "$output" in /*) ;; *) exit 2;; esac
[ ! -e "$output" ] && [ ! -L "$output" ] || exit 2
for input in "$source" "$kernel" "$ecam"; do [ -f "$input" ] && [ ! -L "$input" ] || exit 2; done
hash_is() { [ "$(shasum -a 256 "$1" | awk '{print $1}')" = "$2" ]; }
hash_is "$source" 6a40c785862ab19913a7cf0b9ef64c90fd5998cda2c22c6636edc8f3463b348a
hash_is "$kernel" 6b43356be4b4f5974c555b06599b89d8008825fcbc26b40e8eb57b5edfde1372
hash_is "$ecam" 23e65e2a9a3e886fc7d1429901db6534f2cc7a9077f121d3e334f4cf839cf11d
package=$build/objects/darwin/arm64/release/tools/package/package
bfs=$build/objects/darwin/arm64/release/tools/bfs_shell/bfs_shell
[ -x "$package" ] && [ -x "$bfs" ] || exit 2
umask 077
mkdir -m 700 "$output"
mkdir -p "$output/replacement/add-ons/kernel/busses/pci" "$output/verify/add-ons/kernel/busses/pci"
cp "$source" "$output/candidate.hpkg"
cp "$kernel" "$output/replacement/kernel_riscv64"
cp "$ecam" "$output/replacement/add-ons/kernel/busses/pci/ecam"
chmod 755 "$output/replacement/kernel_riscv64" "$output/replacement/add-ons/kernel/busses/pci/ecam"
"$package" list -p "$source" > "$output/original.paths"
[ "$(grep -cx 'kernel_riscv64' "$output/original.paths")" = 1 ]
[ "$(grep -cx 'add-ons/kernel/busses/pci/ecam' "$output/original.paths")" = 1 ]
"$package" add -1 -f -C "$output/replacement" "$output/candidate.hpkg" kernel_riscv64 add-ons/kernel/busses/pci/ecam > "$output/package-add.log" 2>&1
"$package" list -p "$output/candidate.hpkg" > "$output/candidate.paths"
LC_ALL=C sort "$output/original.paths" > "$output/original.sorted"
LC_ALL=C sort "$output/candidate.paths" > "$output/candidate.sorted"
cmp "$output/original.sorted" "$output/candidate.sorted"
"$package" extract -C "$output/verify" "$output/candidate.hpkg" kernel_riscv64 > "$output/extract-kernel.log" 2>&1
"$package" extract -C "$output/verify" "$output/candidate.hpkg" add-ons/kernel/busses/pci/ecam > "$output/extract-ecam.log" 2>&1
hash_is "$output/verify/kernel_riscv64" 6b43356be4b4f5974c555b06599b89d8008825fcbc26b40e8eb57b5edfde1372
hash_is "$output/verify/add-ons/kernel/busses/pci/ecam" 23e65e2a9a3e886fc7d1429901db6534f2cc7a9077f121d3e334f4cf839cf11d
hash_is "$source" 6a40c785862ab19913a7cf0b9ef64c90fd5998cda2c22c6636edc8f3463b348a
truncate -s 314572800 "$output/candidate-ram.bfs"
"$bfs" --initialize "$output/candidate-ram.bfs" PioneerQ35Pin 'block_size 2048' > "$output/initialize.log" 2>&1
printf '%s\n' 'mkdir /myfs/system' 'mkdir /myfs/system/packages' \
 "cp :$output/candidate.hpkg /myfs/system/packages/haiku-r1~beta6_hrev99999-1-riscv64.hpkg" \
 'sync' 'checkfs -c' \
 "cp /myfs/system/packages/haiku-r1~beta6_hrev99999-1-riscv64.hpkg :$output/roundtrip.hpkg" \
 'sync' > "$output/bfs.commands"
"$bfs" -n "$output/candidate-ram.bfs" 3< "$output/bfs.commands" 4> "$output/bfs.replies" > "$output/bfs.log" 2>&1
[ "$(wc -l < "$output/bfs.replies" | tr -d ' ')" = 7 ]
[ "$(sort -u "$output/bfs.replies")" = 0 ]
cmp "$output/candidate.hpkg" "$output/roundtrip.hpkg"
shasum -a 256 "$output/candidate-ram.bfs" "$output/candidate.hpkg" "$output/verify/kernel_riscv64" "$output/verify/add-ons/kernel/busses/pci/ecam" > "$output/manifest.sha256"
cat "$output/manifest.sha256"
echo 'PASS private candidate RAM bundle and package roundtrip; NOT INSTALLED OR BOOTED'
