#!/bin/sh
#54 physical STOP-audit diagnostic upgrade. NEW physically cold Haiku boot BEFORE any
# accelerator open;never overwrite a loaded module or retry UNKNOWN.
set -eu
case "${1-}" in --apply|--restore52|--restore43) mode=$1;; *) exit 2;; esac
[ "$#" = 1 ] && [ "$(uname -s)" = Haiku ] || exit 2
stage=/boot/home/pioneer-qwen38-dispatch-20261018-f
prior=/boot/home/pioneer-qwen38-dispatch-20261018-e
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
candidate=$stage/tenstorrent_pci_probe-driver54-physical-audit
client=$stage/pioneer-q38-dispatch-client54-diagnostic
inference=$stage/inference-pool-profile-252b6b9c
backup43=$stage/qualified43-backup
backup52=$stage/qualified52-backup
backup54=$stage/prior54-unqualified-backup
incoming=$stage/tenstorrent_pci_probe.incoming
old=ec0813e7939c660491ebd7f8395ed621c55462bace407774413db189789aa5d9
new=e07f3123015c0362c55fe15dc1881c08ed1135e928dc314fa590d43b6f027100
qualified43=9e098d02dc9646bebb9f8a1ae076fc68cbf834541cd8f7d09cd4142cfa43d7bd
qualified52=1de4174478be974ce024bf2161fb3a8f3e39956d5c55237ec61b334abaa20967
clientsha=7b7999d37ad7ff84c7be904e5c1e0e6b0641819a1db29481d4da119ac7974f15
inferencesha=252b6b9ceaee1d7601be4a7c2d390652cb6314a8f811e7e081781b47c2123494
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
exact "$candidate" "$new" && exact "$client" "$clientsha" && exact "$inference" "$inferencesha"
test ! -e "$incoming" && test ! -L "$incoming"
if [ "$mode" = --apply ]; then
 exact "$installed" "$old"
 exact "$prior/qualified43-backup" "$qualified43"
 exact "$prior/qualified52-backup" "$qualified52"
 for item in "$backup43" "$backup52" "$backup54"; do test ! -e "$item" && test ! -L "$item"; done
 unloaded
 cp -p "$installed" "$backup54"
 cp -p "$prior/qualified43-backup" "$backup43"
 cp -p "$prior/qualified52-backup" "$backup52"
 exact "$backup43" "$qualified43" && exact "$backup52" "$qualified52" && exact "$backup54" "$old"
 source=$candidate; expected=$old; result=$new
else
 exact "$installed" "$new" && exact "$backup43" "$qualified43" && exact "$backup52" "$qualified52" && exact "$backup54" "$old"
 unloaded
 expected=$new
 if [ "$mode" = --restore52 ]; then source=$backup52; result=$qualified52
 else source=$backup43; result=$qualified43; fi
fi
cp -p "$source" "$incoming"
chmod 755 "$incoming"
exact "$incoming" "$result" && exact "$installed" "$expected"
unloaded
mv -f "$incoming" "$installed"
sync
exact "$installed" "$result" && exact "$backup43" "$qualified43" && exact "$backup52" "$qualified52" && exact "$backup54" "$old"
test -L "$device" && test "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe
echo "UNLOADED_PHYSICAL_AUDIT54_SWITCH_$mode;SHA=$result;NO_GPU_EXECUTION_YET"
