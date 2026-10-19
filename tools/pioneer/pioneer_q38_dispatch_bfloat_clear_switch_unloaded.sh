#!/bin/sh
#56 lossless BF16/physical-clear candidate;requires qualified55 stage-t.
# NEW physically cold Haiku boot BEFORE accelerator open. Never replace a
# loaded module or retry UNKNOWN. Preserve ALL five qualified fallbacks.
set -eu
case "${1-}" in --apply|--restore55|--restore59|--restore54|--restore52|--restore43) mode=$1;; *) exit 2;; esac
[ "$#" = 1 ] && [ "$(uname -s)" = Haiku ] || exit 2
stage=/boot/home/pioneer-qwen38-bfloat-clear-20261018-u
prior=/boot/home/pioneer-qwen38-vector-clear-20261018-t
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
candidate=$stage/tenstorrent_pci_probe-driver56-bfloat-clear
client=$stage/pioneer-q38-bfloat-client56-clear
inference=$stage/inference-bfloat-08e48e80
backup43=$stage/qualified43-backup
backup52=$stage/qualified52-backup
backup54=$stage/qualified54-frontend-backup
backup55=$stage/qualified55-vector-clear-backup
backup59=$stage/qualified59-physical-clear-backup
incoming=$stage/tenstorrent_pci_probe.incoming
old=9ecb313bd669df9b6e80100586a0bc3b092a490c20ea0bf426ef41ab27689aea
new=151c4e386e66d8e31067e5b999a675f6861e1ee1b7d2de0c9f33bb57765b0829
qualified59=f51b2193eafb93ea5c03a58654f60b345312bbd33c539f96b22403d64b83c564
qualified54=93f2c0db52db526ed395783550752f5126aeaa401e16fa7fb296cd544c326e67
qualified43=9e098d02dc9646bebb9f8a1ae076fc68cbf834541cd8f7d09cd4142cfa43d7bd
qualified52=1de4174478be974ce024bf2161fb3a8f3e39956d5c55237ec61b334abaa20967
clientsha=9520d6b85b6d3901c3940ff0e5c9697af002a05643f8474ab093c7a7e0bb8181
inferencesha=08e48e80c7e8fc2b528a1b1a06660e59fb3ab8b89f50fe1b143df58f3c0233ae
exact()
{
 test -f "$1" && test ! -L "$1" || return 1
 dispatch_hash=$(sha256sum "$1"); dispatch_hash=${dispatch_hash%% *}
 test "$dispatch_hash" = "$2"
}
unloaded()
{
 dispatch_images=$(listimage 1) || return 1
 printf '%s\n' "$dispatch_images" | awk '
  /^KERNEL TEAM:$/ {headers++}
  $1 ~ /^[0-9]+$/ {images++; if($6=="kernel_riscv64") kernel++; if($6 ~ /tenstorrent|P100A/) card++}
  END {if(headers!=1 || kernel!=1 || images<30 || images>200 || card) exit 1}
 ' || { echo STOP_LOADED_OR_UNPROVEN_KERNEL_IMAGE_LIST_NO_WRITES; return 1; }
 dispatch_teams=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$dispatch_teams" | awk '
  /\/Assistant|\/llama-|\/tenstorrent_haiku|\/inference-|\/pioneer_q38|\/pioneer-q38|\/pioneer-qwen38/ {owner++}
  END {if(owner) exit 1}
 ' || { echo STOP_POSSIBLE_COMPUTE_OWNER_NO_WRITES; return 1; }
}
test -d "$stage" && test ! -L "$stage" || exit 1
test -L "$device" && test "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe || exit 1
exact "$candidate" "$new" && exact "$client" "$clientsha" && exact "$inference" "$inferencesha" || exit 1
test ! -e "$incoming" && test ! -L "$incoming" || exit 1
if [ "$mode" = --apply ]; then
 exact "$installed" "$old"
 exact "$prior/qualified43-backup" "$qualified43"
 exact "$prior/qualified52-backup" "$qualified52"
 exact "$prior/qualified54-frontend-backup" "$qualified54"
 exact "$prior/qualified59-physical-clear-backup" "$qualified59"
 for item in "$backup43" "$backup52" "$backup54" "$backup55" "$backup59"; do test ! -e "$item" && test ! -L "$item" || exit 1; done
 unloaded || exit 1
 cp -p "$installed" "$backup55"
 cp -p "$prior/qualified54-frontend-backup" "$backup54"
 cp -p "$prior/qualified43-backup" "$backup43"
 cp -p "$prior/qualified52-backup" "$backup52"
 cp -p "$prior/qualified59-physical-clear-backup" "$backup59"
 exact "$backup43" "$qualified43" && exact "$backup52" "$qualified52" && exact "$backup54" "$qualified54" && exact "$backup55" "$old" && exact "$backup59" "$qualified59" || exit 1
 source=$candidate; expected=$old; result=$new
else
 exact "$installed" "$new" && exact "$backup43" "$qualified43" && exact "$backup52" "$qualified52" && exact "$backup54" "$qualified54" && exact "$backup55" "$old" && exact "$backup59" "$qualified59" || exit 1
 unloaded || exit 1
 expected=$new
 if [ "$mode" = --restore55 ]; then source=$backup55; result=$old
 elif [ "$mode" = --restore59 ]; then source=$backup59; result=$qualified59
 elif [ "$mode" = --restore54 ]; then source=$backup54; result=$qualified54
 elif [ "$mode" = --restore52 ]; then source=$backup52; result=$qualified52
 else source=$backup43; result=$qualified43; fi
fi
cp -p "$source" "$incoming"
chmod 755 "$incoming"
exact "$incoming" "$result" && exact "$installed" "$expected" || exit 1
unloaded || exit 1
mv -f "$incoming" "$installed"
sync
exact "$installed" "$result" && exact "$backup43" "$qualified43" && exact "$backup52" "$qualified52" && exact "$backup54" "$qualified54" && exact "$backup55" "$old" && exact "$backup59" "$qualified59" || exit 1
test -L "$device" && test "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe || exit 1
echo "UNLOADED_BFLOAT_CLEAR56_SWITCH_$mode;SHA=$result;NO_GPU_EXECUTION_YET"
