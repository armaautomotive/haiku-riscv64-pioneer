#!/bin/sh
# Experimental54 ONLY on a NEW physically cold-recovered Haiku boot BEFORE
# any accelerator open. NEVER overwrite/force-unload a loaded module,or
# retry UNKNOWN. Restore modes require the SAME unloaded/owner guards.
set -eu
case "${1-}" in --apply|--restore52|--restore43) mode=$1;; *) exit 2;; esac
[ "$#" = 1 ] && [ "$(uname -s)" = Haiku ] || exit 2
stage=/boot/home/pioneer-qwen38-dispatch-20261017-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
candidate=$stage/tenstorrent_pci_probe-driver54
client=$stage/pioneer-q38-dispatch-client54
inference=$stage/pioneer-q38-dispatch-inference54
source43=/boot/home/pioneer-qwen38-cohort-sealed-20261017-a/qualified43-9e098d02-backup
backup43=$stage/qualified43-9e098d02-backup
backup52=$stage/qualified52-1de41744-backup
incoming=$stage/tenstorrent_pci_probe.incoming
old=1de4174478be974ce024bf2161fb3a8f3e39956d5c55237ec61b334abaa20967
new=e4642e6882845e861174e54ea4f2bc691100ca0a17730a92ea7c3330a146ee32
qualified=9e098d02dc9646bebb9f8a1ae076fc68cbf834541cd8f7d09cd4142cfa43d7bd
clientsha=38b6032090a663c6de5a06703bde7f9eea15a7bf19ada609c79645b48f3ff118
inferencesha=a175f26a922f79cf575e674175008f15eb15580b1bacc193d85271436785b509
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
  /\/Assistant|\/llama-|\/tenstorrent_haiku|\/pioneer_q38|\/pioneer-q38/ {owner++}
  END {if(owner) exit 1}
 ' || { echo STOP_POSSIBLE_COMPUTE_OWNER_NO_WRITES; return 1; }
}
test -d "$stage" && test ! -L "$stage"
test -L "$device" && test "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe
exact "$candidate" "$new" && exact "$client" "$clientsha" && exact "$inference" "$inferencesha" && exact "$source43" "$qualified"
test ! -e "$incoming" && test ! -L "$incoming"
if [ "$mode" = --apply ]; then
 exact "$installed" "$old"
 test ! -e "$backup43" && test ! -L "$backup43"
 test ! -e "$backup52" && test ! -L "$backup52"
 unloaded
 cp -p "$installed" "$backup52"
 cp -p "$source43" "$backup43"
 exact "$backup43" "$qualified" && exact "$backup52" "$old"
 source=$candidate; expected=$old; result=$new
else
 exact "$installed" "$new" && exact "$backup43" "$qualified" && exact "$backup52" "$old"
 unloaded
 expected=$new
 if [ "$mode" = --restore52 ]; then source=$backup52; result=$old
 else source=$backup43; result=$qualified; fi
fi
cp -p "$source" "$incoming"
chmod 755 "$incoming"
exact "$incoming" "$result" && exact "$installed" "$expected"
unloaded
mv -f "$incoming" "$installed"
sync
exact "$installed" "$result" && exact "$backup43" "$qualified" && exact "$backup52" "$old"
test -L "$device" && test "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe
echo "UNLOADED_PERSISTENT_DISPATCH_DRIVER_SWITCH_$mode;SHA=$result;NO_GPU_EXECUTION_YET"
