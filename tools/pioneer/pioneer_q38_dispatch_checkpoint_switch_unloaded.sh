#!/bin/sh
#54 STOP-checkpoint diagnostic upgrade. NEW physically cold Haiku boot BEFORE any
# accelerator open;never overwrite a loaded module or retry UNKNOWN.
set -eu
case "${1-}" in --apply|--restore52|--restore43) mode=$1;; *) exit 2;; esac
[ "$#" = 1 ] && [ "$(uname -s)" = Haiku ] || exit 2
stage=/boot/home/pioneer-qwen38-dispatch-20261018-e
prior=/boot/home/pioneer-qwen38-dispatch-20261018-d
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
candidate=$stage/tenstorrent_pci_probe-driver54-checkpoint
client=$stage/pioneer-q38-dispatch-client54-diagnostic
inference=$stage/inference-pool-profile-252b6b9c
backup43=$stage/qualified43-backup
backup52=$stage/qualified52-backup
backup54=$stage/prior54-unqualified-backup
incoming=$stage/tenstorrent_pci_probe.incoming
old=34feba3cdd7d356c81fd51a6e561ca012cbb73f708a4258a5be4202041068e27
new=ec0813e7939c660491ebd7f8395ed621c55462bace407774413db189789aa5d9
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
  /\/Assistant|\/llama-|\/tenstorrent_haiku|\/pioneer_q38|\/pioneer-q38|\/pioneer-qwen38/ {owner++}
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
 for item in "$backup43" "$backup52" "$backup54"; do test ! -e "$item" && test ! -L "$item" || exit 1; done
 unloaded || exit 1
 cp -p "$installed" "$backup54"
 cp -p "$prior/qualified43-backup" "$backup43"
 cp -p "$prior/qualified52-backup" "$backup52"
 exact "$backup43" "$qualified43" && exact "$backup52" "$qualified52" && exact "$backup54" "$old" || exit 1
 source=$candidate; expected=$old; result=$new
else
 exact "$installed" "$new" && exact "$backup43" "$qualified43" && exact "$backup52" "$qualified52" && exact "$backup54" "$old" || exit 1
 unloaded || exit 1
 expected=$new
 if [ "$mode" = --restore52 ]; then source=$backup52; result=$qualified52
 else source=$backup43; result=$qualified43; fi
fi
cp -p "$source" "$incoming"
chmod 755 "$incoming"
exact "$incoming" "$result" && exact "$installed" "$expected" || exit 1
unloaded || exit 1
mv -f "$incoming" "$installed"
sync
exact "$installed" "$result" && exact "$backup43" "$qualified43" && exact "$backup52" "$qualified52" && exact "$backup54" "$old" || exit 1
test -L "$device" && test "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe || exit 1
echo "UNLOADED_CHECKPOINT54_SWITCH_$mode;SHA=$result;NO_GPU_EXECUTION_YET"
