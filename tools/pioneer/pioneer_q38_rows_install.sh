#!/bin/sh
# Exact50 local batched fullK ROWS trial. ONLY on a NEW cold-recovered Haiku
# boot BEFORE opening the accelerator. NEVER replace/force-unload a loaded
# module or retry UNKNOWN. --restore43 has the SAME unloaded/owner guard.
set -eu
case "${1-}" in --apply|--restore43) mode=$1;; *) exit 2;; esac
[ "$#" = 1 ] && [ "$(uname -s)" = Haiku ] || exit 2
stage=/boot/home/pioneer-qwen38-rows-20261017-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
candidate=$stage/tenstorrent_pci_probe-driver50
client=$stage/pioneer-q38-rows-client50
source43=/boot/home/pioneer-qwen38-stream-20261017-a/qualified43-9e098d02-backup
backup43=$stage/qualified43-9e098d02-backup
backup49=$stage/local49-637db0a6-backup
incoming=$stage/tenstorrent_pci_probe.incoming
old=637db0a65bbffa3d56b1117dec4c2ae1f3bafeaf3ca1084058bd0e9173e373a1
new=ce918971bdfe6a530589a18c849008c6eefe80b0dad67f744a2a2b2f81504606
qualified=9e098d02dc9646bebb9f8a1ae076fc68cbf834541cd8f7d09cd4142cfa43d7bd
clientsha=d7101b8417589d03f4585d00ab4c21ed94d47c4a84e499cfea02b3c19229f512
exact()
{
 test -f "$1" && test ! -L "$1" || return 1
 rows_hash=$(sha256sum "$1"); rows_hash=${rows_hash%% *}
 test "$rows_hash" = "$2"
}
unloaded()
{
 rows_images=$(listimage 1) || return 1
 printf '%s\n' "$rows_images" | awk '
  /^KERNEL TEAM:$/ {headers++}
  $1 ~ /^[0-9]+$/ {images++; if($6=="kernel_riscv64") kernel++; if($6 ~ /tenstorrent|P100A/) card++}
  END {if(headers!=1 || kernel!=1 || images<30 || images>200 || card) exit 1}
 ' || { echo STOP_LOADED_OR_UNPROVEN_KERNEL_IMAGE_LIST_NO_WRITES; return 1; }
 rows_teams=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$rows_teams" | awk '
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
 test ! -e "$backup49" && test ! -L "$backup49"
 unloaded
 cp -p "$installed" "$backup49"
 cp -p "$source43" "$backup43"
 exact "$backup43" "$qualified" && exact "$backup49" "$old"
 source=$candidate; expected=$old; result=$new
else
 exact "$installed" "$new" && exact "$backup43" "$qualified" && exact "$backup49" "$old"
 unloaded
 source=$backup43; expected=$new; result=$qualified
fi
cp -p "$source" "$incoming"
chmod 755 "$incoming"
exact "$incoming" "$result" && exact "$installed" "$expected"
unloaded
mv -f "$incoming" "$installed"
sync
exact "$installed" "$result" && exact "$backup43" "$qualified" && exact "$backup49" "$old"
test -L "$device" && test "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe
echo "UNLOADED_ROWS_DRIVER_SWITCH_$mode;SHA=$result;NO_GPU_EXECUTION_YET"
