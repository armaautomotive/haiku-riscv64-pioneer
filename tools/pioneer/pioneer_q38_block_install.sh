#!/bin/sh
# Exact48 local COMPLETE-BLOCK trial. ONLY on a NEW cold-recovered Haiku
# boot BEFORE opening the accelerator. NEVER replace/force-unload a loaded
# module or retry UNKNOWN. --restore43 has the SAME unloaded/owner guard.
set -eu
case "${1-}" in --apply|--restore43) mode=$1;; *) exit 2;; esac
[ "$#" = 1 ] && [ "$(uname -s)" = Haiku ] || exit 2
stage=/boot/home/pioneer-qwen38-block-20261017-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
candidate=$stage/tenstorrent_pci_probe-driver48
client=$stage/pioneer-q38-block-client48
source43=/boot/home/pioneer-qwen38-affine-20261017-a/qualified43-9e098d02-backup
backup43=$stage/qualified43-9e098d02-backup
backup47=$stage/local47-aab1ea67-backup
incoming=$stage/tenstorrent_pci_probe.incoming
old=aab1ea675a39bebf54d57a1e38c6aad1db00d11a97c596b4ca3c1a8b15b0b69e
new=ef86583131ef4d81eeb6199cb68ce773302e6ca090722ba67e78d43866765b4c
qualified=9e098d02dc9646bebb9f8a1ae076fc68cbf834541cd8f7d09cd4142cfa43d7bd
clientsha=90ad1ef8b8c638f481c1b0ba58d18accfab4a212d7bb0f5609d47a1200ea3d3d
exact()
{
 test -f "$1" && test ! -L "$1" || return 1
 block_hash=$(sha256sum "$1"); block_hash=${block_hash%% *}
 test "$block_hash" = "$2"
}
unloaded()
{
 block_images=$(listimage 1) || return 1
 printf '%s\n' "$block_images" | awk '
  /^KERNEL TEAM:$/ {headers++}
  $1 ~ /^[0-9]+$/ {images++; if($6=="kernel_riscv64") kernel++; if($6 ~ /tenstorrent|P100A/) card++}
  END {if(headers!=1 || kernel!=1 || images<30 || images>200 || card) exit 1}
 ' || { echo STOP_LOADED_OR_UNPROVEN_KERNEL_IMAGE_LIST_NO_WRITES; return 1; }
 block_teams=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$block_teams" | awk '
  /\/Assistant|\/llama-|\/tenstorrent_haiku|\/pioneer_q38|\/pioneer-q38/ {owner++}
  END {if(owner) exit 1}
 ' || { echo STOP_POSSIBLE_COMPUTE_OWNER_NO_WRITES; return 1; }
}
test -d "$stage" && test ! -L "$stage"
test -L "$device" && test "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe
exact "$candidate" "$new" && exact "$client" "$clientsha" && exact "$source43" "$qualified"
test ! -e "$incoming" && test ! -L "$incoming"
if [ "$mode" = --apply ]; then
 exact "$installed" "$old"
 test ! -e "$backup43" && test ! -L "$backup43"
 test ! -e "$backup47" && test ! -L "$backup47"
 unloaded
 cp -p "$installed" "$backup47"
 cp -p "$source43" "$backup43"
 exact "$backup43" "$qualified" && exact "$backup47" "$old"
 source=$candidate; expected=$old; result=$new
else
 exact "$installed" "$new" && exact "$backup43" "$qualified" && exact "$backup47" "$old"
 unloaded
 source=$backup43; expected=$new; result=$qualified
fi
cp -p "$source" "$incoming"
chmod 755 "$incoming"
exact "$incoming" "$result" && exact "$installed" "$expected"
unloaded
mv -f "$incoming" "$installed"
sync
exact "$installed" "$result" && exact "$backup43" "$qualified" && exact "$backup47" "$old"
test -L "$device" && test "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe
echo "UNLOADED_BLOCK_DRIVER_SWITCH_$mode;SHA=$result;NO_GPU_EXECUTION_YET"
