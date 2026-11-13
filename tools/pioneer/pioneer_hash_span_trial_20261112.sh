#!/bin/sh
# One bounded public Work transport comparison. No model/inference. On an
# uncertain result: CLOSE ONLY, never reset/retry/exchange/restore blindly.
set -eu
[ "$#" = 1 ] && [ "$1" = --fixed-hash-span-once ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
umask 077
stage=/boot/home/pioneer-hash-span-20261112-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
candidate=$stage/candidate
client=$stage/public-span-client
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
candidate_hash=3be6fe0bbfe7574d4cd3fd774e6d914497bc4cb9df19c31220e1afcfef998389
client_hash=ab1485d6b4261b73897df752aef956482b0f5b438201c77c0addbb02167f656d
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
 $1 ~ /^[0-9]+$/ && NF>=2 {teams++;if($1==self)next
  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-dram|graph-csr-debug|source-check|public-span-client)/)owners++
  next}
 NF{bad++} END{if(header!=1 || !teams || bad || owners)exit 1}'
}
metadata(){
 current=$("$admin" --status) || return 1
 printf '%s\n' "$current"
 [ "$current" = "P100A_RELOAD driver55 tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]
}
health(){
 sysinfo | awk '/^64 .*running at/{cpus++} /used\/max [0-9]+ \/ 137034936320\)/{ram++} END{if(cpus!=1 || ram!=1)exit 1}'
}
exchange(){
 oldhash=$1;newhash=$2;oldtag=$3;newtag=$4;oldgen=$5;newgen=$6;replacement=$7
 exact "$installed" "$oldhash" && exact "$replacement" "$newhash" && exact "$client" "$client_hash" && idle && metadata "$oldtag" "$oldgen" || return 1
 test ! -e "$stage/driver.incoming" && test ! -L "$stage/driver.incoming" || return 1
 cp -p "$replacement" "$stage/driver.incoming" && chmod 755 "$stage/driver.incoming" || return 1
 exact "$stage/driver.incoming" "$newhash" && exact "$installed" "$oldhash" && idle || return 1
 "$admin" --quiesce "$oldtag" || return 1
 exact "$stage/driver.incoming" "$newhash" && exact "$installed" "$oldhash" || return 1
 mv -f "$stage/driver.incoming" "$installed" && sync && exact "$installed" "$newhash" || return 1
 previous=$(printf '%016x' "$oldgen") || return 1
 "$admin" --wait "$newtag" "$previous" || return 1
 metadata "$newtag" "$newgen"
}
check_output(){
 test ! -s "$1.stderr" && [ "$(cat "$1.exit")" = 0 ] || return 1
 awk '
 /^PUBLIC_SPAN_TRANSPORT_ONLY;NO_MODEL_OR_INFERENCE;FAILURE_CLOSE_ONLY_NO_END_RETRY_RESET$/ {header++;next}
 /^PUBLIC_SPAN bank[0-6] bytes[0-9]+ pages[0-9]+ nonce[0-9]+ seconds[0-9.]+ known_held1 cpu_transport_checked0;NOT_MODEL_OR_TOKEN_RATE$/ {jobs++;next}
 /^PUBLIC_WORK_SPAN_DEBUG_READBACK matched_bytes7512064;EVERY_ORIGINAL_BYTE_COMPARED_DEBUG_CLIENT_ONLY$/ {readback++;next}
 /^PASS_PUBLIC_SPAN_TRANSPORT jobs21 bytes7512064 upload_seconds[0-9.]+ host_MB_per_second[0-9.]+ clean_end1;NOT_PURE_PCIE_BANDWIDTH_OR_LLM_THROUGHPUT$/ {passed++;next}
 {bad++} END{if(header!=1 || jobs!=21 || readback!=1 || passed!=1 || bad)exit 1}' "$1.stdout"
}
run_trial(){
 name=$1;tag=$2;gen=$3;hash=$4
 exact "$installed" "$hash" && exact "$client" "$client_hash" && idle && metadata "$tag" "$gen" || return 1
 printf '%s\n' "RUNNING_${name}_NO_INTERVENTION" > "$stage/state"
 if "$client" --public-work-span-transport > "$stage/$name.stdout" 2> "$stage/$name.stderr";then result=0;else result=$?;fi
 printf '%s\n' "$result" > "$stage/$name.exit"
 if [ "$result" != 0 ] || ! check_output "$stage/$name";then
  printf '%s\n' "UNPROVEN_${name}_NO_RETRY_RESET_OR_RESTORE" > "$stage/state";return 1
 fi
 idle && metadata "$tag" "$gen" || return 1
 printf '%s\n' "CLOSED_${name}_KNOWN_CLEAN" > "$stage/state"
}
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || exit 1
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] || exit 1
[ "$(stat -c '%d' "$stage")" = "$(stat -c '%d' "$installed")" ] || exit 1
exact "$stable" "$stable_hash" && exact "$installed" "$stable_hash" && exact "$candidate" "$candidate_hash" && exact "$client" "$client_hash" || exit 1
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || exit 1
exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 || exit 1
idle && metadata 2026102100010012 40 && health || exit 1
for name in .trial-once stable.backup driver.incoming baseline.stdout baseline.stderr baseline.exit candidate-run.stdout candidate-run.stderr candidate-run.exit parent.pid state;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
mkdir "$stage/.trial-once"
printf '%s\n' "$$" > "$stage/parent.pid"
cp -p "$stable" "$stage/stable.backup" && exact "$stage/stable.backup" "$stable_hash" || exit 1
printf '%s\n' BACKUP_VERIFIED > "$stage/state"
run_trial baseline 2026102100010012 40 "$stable_hash" || exit 70
exchange "$stable_hash" "$candidate_hash" 2026102100010012 2026111200010002 40 41 "$candidate" || exit 71
run_trial candidate-run 2026111200010002 41 "$candidate_hash" || exit 72
exchange "$candidate_hash" "$stable_hash" 2026111200010002 2026102100010012 41 42 "$stage/stable.backup" || exit 73
exact "$installed" "$stable_hash" && idle && metadata 2026102100010012 42 && health || exit 74
printf '%s\n' CLOSED_STABLE_RESTORED > "$stage/state"
echo PASS_BOUNDED_HASH_SPAN_TRIAL_STABLE_RESTORED_NO_MODEL_SPEED_CLAIM
