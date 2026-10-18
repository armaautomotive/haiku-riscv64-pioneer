#!/bin/sh
# Exact46 local-panel trial. Run ONLY on a NEW cold-recovered Haiku boot,
# BEFORE opening the accelerator. NEVER replaces/force-unloads a loaded
# module or retries UNKNOWN completion. --restore43 has the SAME guard.
set -eu
case "${1-}" in --apply|--restore43) mode=$1;; *) exit 2;; esac
[ "$#" = 1 ] && [ "$(uname -s)" = Haiku ] || exit 2
stage=/boot/home/pioneer-qwen38-panel-20261017-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
candidate=$stage/tenstorrent_pci_probe-driver46
client=$stage/pioneer-q38-panel-client46
source43=/boot/home/pioneer-qwen38-bulk-20261017-a/qualified43-9e098d02-backup
backup43=$stage/qualified43-9e098d02-backup
backup45=$stage/storage45-f7d80213-backup
incoming=$stage/tenstorrent_pci_probe.incoming
old=f7d80213a47164aa0c63a2b855e8254c8ea4704058946fd2a3e95a4324325148
new=3e5e3e616bbdd64953b60537c3351958182f44d64abe1eda17e3505df2113162
qualified=9e098d02dc9646bebb9f8a1ae076fc68cbf834541cd8f7d09cd4142cfa43d7bd
clientsha=2aeb5b4d57869f1a9d26641cff6ba0f4eaebe29926d7108042df1fcf6b776f61
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
 test ! -e "$backup45" && test ! -L "$backup45"
 unloaded
 cp -p "$installed" "$backup45"
 cp -p "$source43" "$backup43"
 exact "$backup43" "$qualified" && exact "$backup45" "$old"
 source=$candidate; expected=$old; result=$new
else
 exact "$installed" "$new" && exact "$backup43" "$qualified" && exact "$backup45" "$old"
 unloaded
 source=$backup43; expected=$new; result=$qualified
fi
cp -p "$source" "$incoming"
chmod 755 "$incoming"
exact "$incoming" "$result" && exact "$installed" "$expected"
unloaded
mv -f "$incoming" "$installed"
sync
exact "$installed" "$result" && exact "$backup43" "$qualified" && exact "$backup45" "$old"
test -L "$device" && test "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe
echo "UNLOADED_PANEL_DRIVER_SWITCH_$mode;SHA=$result;NO_GPU_EXECUTION_YET"
