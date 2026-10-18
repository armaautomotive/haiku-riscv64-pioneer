#!/bin/sh
# Exact52 boundary-code-sealed cohort trial. ONLY on a NEW cold-recovered Haiku
# boot BEFORE opening the accelerator. NEVER replace/force-unload a loaded
# module or retry UNKNOWN. --restore43 has the SAME unloaded/owner guard.
set -eu
case "${1-}" in --apply|--restore43) mode=$1;; *) exit 2;; esac
[ "$#" = 1 ] && [ "$(uname -s)" = Haiku ] || exit 2
stage=/boot/home/pioneer-qwen38-cohort-sealed-20261017-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
candidate=$stage/tenstorrent_pci_probe-driver52
client=$stage/pioneer-q38-cohort-sealed-client52
source43=/boot/home/pioneer-qwen38-cohort-20261017-a/qualified43-9e098d02-backup
backup43=$stage/qualified43-9e098d02-backup
backup51=$stage/local51-ab7b913f-backup
incoming=$stage/tenstorrent_pci_probe.incoming
old=ab7b913ff230053f6d7c872c9f73ef48e4499bc594f2ae26dde41ee72fdf0ec7
new=1de4174478be974ce024bf2161fb3a8f3e39956d5c55237ec61b334abaa20967
qualified=9e098d02dc9646bebb9f8a1ae076fc68cbf834541cd8f7d09cd4142cfa43d7bd
clientsha=c5f50b0336ea934888dc150e5083015e517bf125ec23b1899e15fc3d8848b019
exact()
{
 test -f "$1" && test ! -L "$1" || return 1
 cohort_hash=$(sha256sum "$1"); cohort_hash=${cohort_hash%% *}
 test "$cohort_hash" = "$2"
}
unloaded()
{
 cohort_images=$(listimage 1) || return 1
 printf '%s\n' "$cohort_images" | awk '
  /^KERNEL TEAM:$/ {headers++}
  $1 ~ /^[0-9]+$/ {images++; if($6=="kernel_riscv64") kernel++; if($6 ~ /tenstorrent|P100A/) card++}
  END {if(headers!=1 || kernel!=1 || images<30 || images>200 || card) exit 1}
 ' || { echo STOP_LOADED_OR_UNPROVEN_KERNEL_IMAGE_LIST_NO_WRITES; return 1; }
 cohort_teams=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$cohort_teams" | awk '
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
 test ! -e "$backup51" && test ! -L "$backup51"
 unloaded
 cp -p "$installed" "$backup51"
 cp -p "$source43" "$backup43"
 exact "$backup43" "$qualified" && exact "$backup51" "$old"
 source=$candidate; expected=$old; result=$new
else
 exact "$installed" "$new" && exact "$backup43" "$qualified" && exact "$backup51" "$old"
 unloaded
 source=$backup43; expected=$new; result=$qualified
fi
cp -p "$source" "$incoming"
chmod 755 "$incoming"
exact "$incoming" "$result" && exact "$installed" "$expected"
unloaded
mv -f "$incoming" "$installed"
sync
exact "$installed" "$result" && exact "$backup43" "$qualified" && exact "$backup51" "$old"
test -L "$device" && test "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe
echo "UNLOADED_COHORT_SEALED_DRIVER_SWITCH_$mode;SHA=$result;NO_GPU_EXECUTION_YET"
