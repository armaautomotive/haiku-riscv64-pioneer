#!/bin/sh
# ONE specific unloaded-driver installation;no force unload/reload/reset.
# Run ONLY in fresh Haiku AFTER controlled cold recovery and model checks.
# Never call a P100A client before the zero-loaded-image prewrite guards.
set -eu
[ "$#" = 1 ] && [ "$1" = --apply ] || exit 2
[ "$(uname -s)" = Haiku ] || exit 1
stage=/boot/home/pioneer-qwen38-banks-20261017-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
candidate=$stage/tenstorrent_pci_probe-driver44
client=$stage/pioneer-qwen38-banks-client-20261017-b
backup=$stage/qualified43-9e098d02-backup
incoming=$stage/tenstorrent_pci_probe.install44
old=9e098d02dc9646bebb9f8a1ae076fc68cbf834541cd8f7d09cd4142cfa43d7bd
new=ca6c7fdf71658f7b53ee8c30a0ead6097d9dabc45a1343f548bc2e67544c1e67
clientsha=d5b2a74231f4c946420a50a189081793976443d8f80fc5ade89b8f866340654c
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
  /\/Assistant|\/llama-|\/tenstorrent_haiku|\/pioneer_q38_banks/ {owner++}
  END {if(owner) exit 1}
 ' || { echo STOP_POSSIBLE_COMPUTE_OWNER_NO_WRITES; return 1; }
}
test -d "$stage" && test ! -L "$stage"
test -L "$device" && test "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe
exact "$installed" "$old" && exact "$candidate" "$new" && exact "$client" "$clientsha"
test ! -e "$backup" && test ! -L "$backup"
test ! -e "$incoming" && test ! -L "$incoming"
unloaded
cp -p "$installed" "$backup"
exact "$backup" "$old"
cp -p "$candidate" "$incoming"
chmod 755 "$incoming"
exact "$incoming" "$new" && exact "$installed" "$old"
unloaded # repeat immediately BEFORE the exact atomic replacement
mv -f "$incoming" "$installed"
sync
exact "$installed" "$new" && exact "$backup" "$old"
test -L "$device" && test "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe
echo UNLOADED_DRIVER44_INSTALLED_QUALIFIED43_PRESERVED_NO_GPU_EXECUTION_YET
