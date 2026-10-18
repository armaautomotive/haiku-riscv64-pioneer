#!/bin/sh
# Exact49 local fullK STREAM trial. ONLY on a NEW cold-recovered Haiku
# boot BEFORE opening the accelerator. NEVER replace/force-unload a loaded
# module or retry UNKNOWN. --restore43 has the SAME unloaded/owner guard.
set -eu
case "${1-}" in --apply|--restore43) mode=$1;; *) exit 2;; esac
[ "$#" = 1 ] && [ "$(uname -s)" = Haiku ] || exit 2
stage=/boot/home/pioneer-qwen38-stream-20261017-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
candidate=$stage/tenstorrent_pci_probe-driver49
client=$stage/pioneer-q38-stream-client49
source43=/boot/home/pioneer-qwen38-block-20261017-a/qualified43-9e098d02-backup
backup43=$stage/qualified43-9e098d02-backup
backup48=$stage/local48-ef865831-backup
incoming=$stage/tenstorrent_pci_probe.incoming
old=ef86583131ef4d81eeb6199cb68ce773302e6ca090722ba67e78d43866765b4c
new=637db0a65bbffa3d56b1117dec4c2ae1f3bafeaf3ca1084058bd0e9173e373a1
qualified=9e098d02dc9646bebb9f8a1ae076fc68cbf834541cd8f7d09cd4142cfa43d7bd
clientsha=49bc095f00065c75fed62ac31bfe7f0cb502a1a8e06ef2564e030dcd0aa0beba
exact()
{
 test -f "$1" && test ! -L "$1" || return 1
 stream_hash=$(sha256sum "$1"); stream_hash=${stream_hash%% *}
 test "$stream_hash" = "$2"
}
unloaded()
{
 stream_images=$(listimage 1) || return 1
 printf '%s\n' "$stream_images" | awk '
  /^KERNEL TEAM:$/ {headers++}
  $1 ~ /^[0-9]+$/ {images++; if($6=="kernel_riscv64") kernel++; if($6 ~ /tenstorrent|P100A/) card++}
  END {if(headers!=1 || kernel!=1 || images<30 || images>200 || card) exit 1}
 ' || { echo STOP_LOADED_OR_UNPROVEN_KERNEL_IMAGE_LIST_NO_WRITES; return 1; }
 stream_teams=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$stream_teams" | awk '
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
 test ! -e "$backup48" && test ! -L "$backup48"
 unloaded
 cp -p "$installed" "$backup48"
 cp -p "$source43" "$backup43"
 exact "$backup43" "$qualified" && exact "$backup48" "$old"
 source=$candidate; expected=$old; result=$new
else
 exact "$installed" "$new" && exact "$backup43" "$qualified" && exact "$backup48" "$old"
 unloaded
 source=$backup43; expected=$new; result=$qualified
fi
cp -p "$source" "$incoming"
chmod 755 "$incoming"
exact "$incoming" "$result" && exact "$installed" "$expected"
unloaded
mv -f "$incoming" "$installed"
sync
exact "$installed" "$result" && exact "$backup43" "$qualified" && exact "$backup48" "$old"
test -L "$device" && test "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe
echo "UNLOADED_STREAM_DRIVER_SWITCH_$mode;SHA=$result;NO_GPU_EXECUTION_YET"
