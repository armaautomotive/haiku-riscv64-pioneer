#!/bin/sh
# ONE exact driver45 trial. NEVER replaces a loaded driver, force unloads,
# retries an uncertain transfer, or resets the card. --restore43 requires
# a NEW cold-recovered Haiku boot whose card driver is still UNLOADED.
set -eu
case "${1-}" in --apply|--restore43) mode=$1;; *) exit 2;; esac
[ "$#" = 1 ] && [ "$(uname -s)" = Haiku ] || exit 2
stage=/boot/home/pioneer-qwen38-bulk-20261017-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
candidate=$stage/tenstorrent_pci_probe-driver45
client=$stage/pioneer-q38-banks-client45
backup=$stage/qualified43-9e098d02-backup
incoming=$stage/tenstorrent_pci_probe.incoming
old=9e098d02dc9646bebb9f8a1ae076fc68cbf834541cd8f7d09cd4142cfa43d7bd
new=f7d80213a47164aa0c63a2b855e8254c8ea4704058946fd2a3e95a4324325148
clientsha=ab9a76153e913328adb412c9c44330d407cb7e519882ac02f6a6234f65d02925
exact()
{
 test -f "$1" && test ! -L "$1" || return 1
 q38_hash=$(sha256sum "$1"); q38_hash=${q38_hash%% *}
 test "$q38_hash" = "$2"
}
unloaded()
{
 q38_images=$(listimage 1) || return 1
 printf '%s\n' "$q38_images" | awk '
  /^KERNEL TEAM:$/ {headers++}
  $1 ~ /^[0-9]+$/ {images++; if($6=="kernel_riscv64") kernel++; if($6 ~ /tenstorrent|P100A/) card++}
  END {if(headers!=1 || kernel!=1 || images<30 || images>200 || card) exit 1}
 ' || { echo STOP_LOADED_OR_UNPROVEN_KERNEL_IMAGE_LIST_NO_WRITES; return 1; }
 q38_teams=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$q38_teams" | awk '
  /\/Assistant|\/llama-|\/tenstorrent_haiku|\/pioneer_q38_banks|\/pioneer-q38-banks/ {owner++}
  END {if(owner) exit 1}
 ' || { echo STOP_POSSIBLE_COMPUTE_OWNER_NO_WRITES; return 1; }
}
test -d "$stage" && test ! -L "$stage"
test -L "$device" && test "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe
exact "$candidate" "$new" && exact "$client" "$clientsha"
test ! -e "$incoming" && test ! -L "$incoming"
if [ "$mode" = --apply ]; then
 exact "$installed" "$old"
 test ! -e "$backup" && test ! -L "$backup"
 unloaded
 cp -p "$installed" "$backup"
 exact "$backup" "$old"
 source=$candidate; expected=$old; result=$new
else
 exact "$installed" "$new" && exact "$backup" "$old"
 unloaded
 source=$backup; expected=$new; result=$old
fi
cp -p "$source" "$incoming"
chmod 755 "$incoming"
exact "$incoming" "$result" && exact "$installed" "$expected"
unloaded # repeated immediately BEFORE the exact atomic replacement
mv -f "$incoming" "$installed"
sync
exact "$installed" "$result" && exact "$backup" "$old"
test -L "$device" && test "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe
echo "UNLOADED_DRIVER_SWITCH_$mode;SHA=$result;NO_GPU_EXECUTION_YET"
