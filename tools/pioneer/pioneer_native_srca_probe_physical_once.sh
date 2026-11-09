#!/bin/sh
# Tag135 ONE fixed synthetic PV test. Failed/unproven states retain resources.
# No retry, reset, force unload, cold clear or automatic rollback on failure.
set -eu
[ "$#" = 0 ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
stage=/boot/home/pioneer-srca-probe-20261109-g14
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
candidate=$stage/tenstorrent_pci_probe-native-srca-135
client=$stage/pioneer-native-srca-probe-diagnostic
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
candidate_hash=fd07d0a51cc4efe764a1125f4babfe8b32866b5c15c87f1e4b2f197550bd0430
client_hash=93bff45b96d1c31633445dd39beb67bd2f600ca9dc3499c5f573607a5f2f789a
exact(){
 test -f "$1" && test ! -L "$1" || return 1
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555) ;; *) return 1;; esac
 value=$(sha256sum "$1") || return 1
 [ "${value%% *}" = "$2" ]
}
idle(){
 listing=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$listing" | awk -v self="$$" '
 $1=="Id" && $2=="Team" && NF==2 {header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {
  teams++;if($1==self)next
  if($0 ~ /(^|[\/[:space:]])(Assistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native|pioneer-dram|graph-csr-debug|source-check)/ ||
   $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/) owners++
  next
 }
 NF {bad++}
 END{if(header!=1 || !teams || bad || owners)exit 1}'
}
metadata(){
 current=$("$admin" --status) || return 1
 printf '%s\n' "$current"
 expected="P100A_RELOAD driver55 tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY"
 [ "$current" = "$expected" ]
}
exchange(){
 oldhash=$1;newhash=$2;oldtag=$3;newtag=$4;oldgeneration=$5;newgeneration=$6;replacement=$7
 exact "$installed" "$oldhash" && exact "$replacement" "$newhash" && idle || return 1
 metadata "$oldtag" "$oldgeneration" || return 1
 test ! -e "$stage/driver.incoming" && test ! -L "$stage/driver.incoming" || return 1
 cp -p "$replacement" "$stage/driver.incoming"
 chmod 755 "$stage/driver.incoming"
 exact "$stage/driver.incoming" "$newhash" && exact "$installed" "$oldhash" && exact "$client" "$client_hash" && idle || return 1
 "$admin" --quiesce "$oldtag" || return 1
 exact "$stage/driver.incoming" "$newhash" && exact "$installed" "$oldhash" || return 1
 mv -f "$stage/driver.incoming" "$installed"
 sync
 exact "$installed" "$newhash" || return 1
 previous=$(printf '%016x' "$oldgeneration")
 "$admin" --wait "$newtag" "$previous" || return 1
 metadata "$newtag" "$newgeneration"
}
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || exit 1
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] || exit 1
[ "$(stat -c '%d' "$stage")" = "$(stat -c '%d' "$installed")" ] || exit 1
exact "$stable" "$stable_hash" && exact "$installed" "$stable_hash" && exact "$candidate" "$candidate_hash" && exact "$client" "$client_hash" || exit 1
exact "$guard" d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 || exit 1
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || exit 1
idle && metadata 2026102100010012 14 || exit 1
for name in .trial-once stable041.backup driver.incoming fixed.stdout fixed.stderr fixed.exit; do test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1;done
mkdir "$stage/.trial-once" # never remove or reuse, including failure
printf '%s\n' "$$" > "$stage/parent.pid"
printf '%s\n' ADMITTED_BEFORE_SWAP > "$stage/state"
cp -p "$stable" "$stage/stable041.backup"
exact "$stage/stable041.backup" "$stable_hash" || exit 1
exchange "$stable_hash" "$candidate_hash" 2026102100010012 2026110900010135 14 15 "$candidate" || exit 1
printf '%s\n' SRCA_INSTALLED_NO_GPU_STARTED > "$stage/state"
idle && metadata 2026110900010135 15 && exact "$installed" "$candidate_hash" && exact "$client" "$client_hash" || exit 1
printf '%s\n' RUNNING_FIXED_SRCA > "$stage/state"
if "$client" --fixed-srca-probe > "$stage/fixed.stdout" 2> "$stage/fixed.stderr"; then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/fixed.exit"
sed -n '1,40p' "$stage/fixed.stdout"
sed -n '1,65p' "$stage/fixed.stderr"
if [ "$result" != 0 ]; then
 printf '%s\n' 'UNKNOWN_FIXED_SRCA;NO_END_RESET_RETRY_RESTORE' > "$stage/state"
 echo SRCA_TRIAL_STOP_UNKNOWN_LEAVE_RESOURCES_AND_FILES_UNCHANGED
 exit 70
fi
awk '
 $1=="PASS_NATIVE_SRCA_PROBE_FIXED" && $2=="old_shift32" && $3=="corrected32" && $4 ~ /^polls[1-9][0-9]*$/ &&
 $5=="generation15;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_MODEL_OR_SPEED_CLAIM" && NF==5 {good++;next}
 NF{bad++} END{if(good!=1 || bad)exit 1}' "$stage/fixed.stdout" || exit 1
test ! -s "$stage/fixed.stderr" || exit 1
metadata 2026110900010135 15 || exit 1
printf '%s\n' KNOWN_CLOSED_FIXED_SRCA > "$stage/state"
exchange "$candidate_hash" "$stable_hash" 2026110900010135 2026102100010012 15 16 "$stable" || exit 1
exact "$installed" "$stable_hash" && metadata 2026102100010012 16 && idle || exit 1
sysinfo -cpu
sysinfo -mem
printf '%s\n' STABLE041_RESTORED_FIXED_SRCA_PASSED > "$stage/state"
echo SRCA_TRIAL_KNOWN_CLEAN_STABLE041_RESTORED_NO_MODEL_SPEED_CLAIM
