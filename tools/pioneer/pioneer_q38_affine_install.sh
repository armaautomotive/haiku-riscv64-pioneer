#!/bin/sh
# Exact47 local-affine trial. Run ONLY on a NEW cold-recovered Haiku boot,
# BEFORE opening the accelerator. NEVER replaces/force-unloads a loaded
# module or retries UNKNOWN completion. --restore43 has the SAME guard.
set -eu
case "${1-}" in --apply|--restore43) mode=$1;; *) exit 2;; esac
[ "$#" = 1 ] && [ "$(uname -s)" = Haiku ] || exit 2
stage=/boot/home/pioneer-qwen38-affine-20261017-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
candidate=$stage/tenstorrent_pci_probe-driver47
client=$stage/pioneer-q38-affine-client47
source43=/boot/home/pioneer-qwen38-panel-20261017-a/qualified43-9e098d02-backup
backup43=$stage/qualified43-9e098d02-backup
backup46=$stage/local46-3e5e3e61-backup
incoming=$stage/tenstorrent_pci_probe.incoming
old=3e5e3e616bbdd64953b60537c3351958182f44d64abe1eda17e3505df2113162
new=aab1ea675a39bebf54d57a1e38c6aad1db00d11a97c596b4ca3c1a8b15b0b69e
qualified=9e098d02dc9646bebb9f8a1ae076fc68cbf834541cd8f7d09cd4142cfa43d7bd
clientsha=8f40b41317aa8d99a2d171949b5b036114ddc98eb3b6470cb7edd6f452fc723f
exact()
{
 test -f "$1" && test ! -L "$1" || return 1
 panel_hash=$(sha256sum "$1"); panel_hash=${panel_hash%% *}
 test "$panel_hash" = "$2"
}
unloaded()
{
 panel_images=$(listimage 1) || return 1
 printf '%s\n' "$panel_images" | awk '
  /^KERNEL TEAM:$/ {headers++}
  $1 ~ /^[0-9]+$/ {images++; if($6=="kernel_riscv64") kernel++; if($6 ~ /tenstorrent|P100A/) card++}
  END {if(headers!=1 || kernel!=1 || images<30 || images>200 || card) exit 1}
 ' || { echo STOP_LOADED_OR_UNPROVEN_KERNEL_IMAGE_LIST_NO_WRITES; return 1; }
 panel_teams=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$panel_teams" | awk '
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
 test ! -e "$backup46" && test ! -L "$backup46"
 unloaded
 cp -p "$installed" "$backup46"
 cp -p "$source43" "$backup43"
 exact "$backup43" "$qualified" && exact "$backup46" "$old"
 source=$candidate; expected=$old; result=$new
else
 exact "$installed" "$new" && exact "$backup43" "$qualified" && exact "$backup46" "$old"
 unloaded
 source=$backup43; expected=$new; result=$qualified
fi
cp -p "$source" "$incoming"
chmod 755 "$incoming"
exact "$incoming" "$result" && exact "$installed" "$expected"
unloaded
mv -f "$incoming" "$installed"
sync
exact "$installed" "$result" && exact "$backup43" "$qualified" && exact "$backup46" "$old"
test -L "$device" && test "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe
echo "UNLOADED_PANEL_DRIVER_SWITCH_$mode;SHA=$result;NO_GPU_EXECUTION_YET"

