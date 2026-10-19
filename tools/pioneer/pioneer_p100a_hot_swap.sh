#!/bin/sh
# Guarded development A/B exchange,NOT physical PCIe hotplug or cold recovery.
# Both candidates retain the qualified55 matrix code. No force/reset/unload
# command and NO automatic rollback after an unproven replacement.
set -eu
[ "$#" = 1 ] && [ "$(uname -s)" = Haiku ] || exit 2
case "$1" in --bootstrap|--apply-a|--apply-b) mode=$1;; *) exit 2;; esac
stage=/boot/home/pioneer-p100a-hot-reload-20261019-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
client=$stage/pioneer-p100a-reload
candidatea=$stage/tenstorrent_pci_probe-hot-a
candidateb=$stage/tenstorrent_pci_probe-hot-b
guardsource=$stage/pioneer_p100a_reload_guard
incoming=$stage/driver.incoming
guardincoming=$stage/guard.incoming
original=9ecb313bd669df9b6e80100586a0bc3b092a490c20ea0bf426ef41ab27689aea
hasha=77d0f396433b229eb5937b633e1a57f183b85612ae1f7503b2b5bbd48e261679
hashb=67f2b14bd8a0a17c934e51a4952949223e1e0bddbb3e7a29f2664cb0a9f568c9
guardsha=d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229
clientsha=b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
taga=2026101900010001
tagb=2026101900010002
exact()
{
 test -f "$1" && test ! -L "$1" || return 1
 reload_hash=$(sha256sum "$1"); reload_hash=${reload_hash%% *}
 test "$reload_hash" = "$2"
}
unloaded()
{
 reload_images=$(listimage 1) || return 1
 printf '%s\n' "$reload_images" | awk '
  /^KERNEL TEAM:$/ {headers++}
  $1 ~ /^[0-9]+$/ {images++; if($6=="kernel_riscv64") kernel++; if($6 ~ /tenstorrent|P100A/) card++}
  END {if(headers!=1 || kernel!=1 || images<30 || images>200 || card) exit 1}
 ' || { echo STOP_BOOTSTRAP_REQUIRES_FRESH_UNLOADED_DRIVER; return 1; }
 reload_teams=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$reload_teams" | awk '
  /\/Assistant|\/llama-|\/tenstorrent_haiku|\/inference-|\/pioneer_q38|\/pioneer-q38|\/pioneer-qwen38/ {owner++}
  END {if(owner) exit 1}
 ' || { echo STOP_BOOTSTRAP_POSSIBLE_COMPUTE_OWNER; return 1; }
}
backup()
{
 if test -e "$1" || test -L "$1"; then exact "$1" "$2"
 else cp -p "$installed" "$1"; exact "$1" "$2"; fi
}
test -d "$stage" && test ! -L "$stage" || exit 1
test -L "$device" && test "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe || exit 1
exact "$candidatea" "$hasha" && exact "$candidateb" "$hashb" && exact "$guardsource" "$guardsha" && exact "$client" "$clientsha" || exit 1
test ! -e "$incoming" && test ! -L "$incoming" && test ! -e "$guardincoming" && test ! -L "$guardincoming" || exit 1
if [ "$mode" = --bootstrap ]; then
 # Only this first migration requires a fresh boot. NEVER use it to erase
 # a failure latch in an old driver that lacks the new handshake.
 exact "$installed" "$original" && unloaded || exit 1
 backup "$stage/qualified55-before-hot-reload" "$original"
 if test -e "$guard" || test -L "$guard"; then exact "$guard" "$guardsha" || exit 1
 else
  cp -p "$guardsource" "$guardincoming"; chmod 755 "$guardincoming"
  exact "$guardincoming" "$guardsha" && unloaded || exit 1
  mkdir -p /boot/home/config/non-packaged/add-ons/kernel/generic
  mv "$guardincoming" "$guard"
 fi
 source=$candidatea; expected=$original; result=$hasha
else
 exact "$guard" "$guardsha" || exit 1
 if [ "$mode" = --apply-b ]; then source=$candidateb; expected=$hasha; result=$hashb; oldtag=$taga; newtag=$tagb
 else source=$candidatea; expected=$hashb; result=$hasha; oldtag=$tagb; newtag=$taga; fi
 exact "$installed" "$expected" || exit 1
 metadata=$("$client" --status) || exit 1
 printf '%s\n' "$metadata"
 generation=$(printf '%s\n' "$metadata" | awk -v tag="tag$oldtag" '
  $1=="P100A_RELOAD" && $2=="driver55" && $3==tag && $5=="quiesced0" && $6=="cold0" && $7=="active_owner0" && $8=="open_count1" && $9 ~ /^guard1;/ {
   value=$4; sub(/^generation/,"",value); if(value ~ /^[1-9][0-9]*$/) {print value; good++}}
  END {if(good!=1)exit 1}
 ') || exit 1
 generationhex=$(printf '%016x' "$generation")
 backup "$stage/qualified-reload-$oldtag" "$expected"
fi
cp -p "$source" "$incoming"; chmod 755 "$incoming"
exact "$incoming" "$result" && exact "$installed" "$expected" && exact "$guard" "$guardsha" || exit 1
if [ "$mode" = --bootstrap ]; then unloaded || exit 1
else "$client" --quiesce "$oldtag" || exit 1; fi
exact "$incoming" "$result" && exact "$installed" "$expected" || exit 1
# The staging file is OUTSIDE watched drivers/bin. A single rename INTO
# that directory generates Haiku's normal legacy-driver update event.
mv -f "$incoming" "$installed"
sync
exact "$installed" "$result" && exact "$guard" "$guardsha" || exit 1
if [ "$mode" = --bootstrap ]; then
 echo BOOTSTRAP_HOT_RELOAD55_A_INSTALLED_NO_GPU_EXECUTION
else
 "$client" --wait "$newtag" "$generationhex" || { echo STOP_NOT_VERIFIED_NO_AUTOMATIC_ROLLBACK_OR_RESET; exit 1; }
 echo P100A_HOT_SWAP_COMPLETE_NO_REBOOT
fi
