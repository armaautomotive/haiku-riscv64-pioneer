#!/bin/sh
# ONE fixed synthetic tag95 arithmetic test. Unknown means stop,NO automatic
# END/reset/retry/cold-clear/restore. No model/performance claim from this.
set -eu
[ "$#" = 1 ] && [ "$1" = --fixed-exact-q4-once ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
umask 077
stage=/boot/home/pioneer-native-exact-q4-20261105-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
candidate=$stage/tenstorrent_pci_probe-native-exact-q4-95
client=$stage/pioneer-native-exact-q4-diagnostic
readonly stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
readonly candidate_hash=734d64f3daaebda9b67a10909d5a2302a025dae93b3d51ae7ec31ab53914b6a7
readonly client_hash=d3db09a76b66184b033f301bcfc561d4df2469882a364c63f382fa87f9b9171f
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
  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-dram|graph-csr-debug|source-check)/ ||
   $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/)owners++
  next
 }
 NF{bad++} END{if(header!=1 || !teams || bad || owners)exit 1}'
}
metadata(){
 [ "$#" = 2 ] || return 1
 current=$("$admin" --status) || return 1
 printf '%s\n' "$current"
 [ "$current" = "P100A_RELOAD driver55 tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]
}
health(){
 value=$(sysinfo) || return 1
 printf '%s\n' "$value"
 printf '%s\n' "$value" | awk '
 /^64 .*running at/{cpus++}
 /used\/max [0-9]+ \/ 137034936320\)/{ram++}
 END{if(cpus!=1 || ram!=1)exit 1}'
}
exchange(){
 [ "$#" = 7 ] || return 1
 oldhash=$1;newhash=$2;oldtag=$3;newtag=$4;oldgeneration=$5;newgeneration=$6;replacement=$7
 exact "$installed" "$oldhash" && exact "$replacement" "$newhash" && idle && metadata "$oldtag" "$oldgeneration" || return 1
 test ! -e "$stage/driver.incoming" && test ! -L "$stage/driver.incoming" || return 1
 cp -p "$replacement" "$stage/driver.incoming" || return 1
 chmod 755 "$stage/driver.incoming" || return 1
 exact "$stage/driver.incoming" "$newhash" && exact "$installed" "$oldhash" && exact "$client" "$client_hash" && idle || return 1
 "$admin" --quiesce "$oldtag" || return 1
 exact "$stage/driver.incoming" "$newhash" && exact "$installed" "$oldhash" || return 1
 mv -f "$stage/driver.incoming" "$installed" || return 1
 sync || return 1
 exact "$installed" "$newhash" || return 1
 previous=$(printf '%016x' "$oldgeneration") || return 1
 "$admin" --wait "$newtag" "$previous" || return 1
 metadata "$newtag" "$newgeneration"
}
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || exit 1
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] || exit 1
[ "$(stat -c '%d' "$stage")" = "$(stat -c '%d' "$installed")" ] || exit 1
exact "$stable" "$stable_hash" && exact "$installed" "$stable_hash" && exact "$candidate" "$candidate_hash" && exact "$client" "$client_hash" || exit 1
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || exit 1
exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 || exit 1
exact /boot/home/config/non-packaged/servers/HaikuAssistantQwen27 861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74 || exit 1
exact /boot/home/config/non-packaged/servers/HaikuAssistantService c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18 || exit 1
exact /boot/home/config/non-packaged/apps/HaikuAssistant 0de9141eb3955cf1c993a67c80ef648b3538ce7155c17677458487ee083a4933 || exit 1
idle && metadata 2026102100010012 20 && health || exit 1
for name in .trial-once stable041.backup driver.incoming fixed.stdout fixed.stderr fixed.exit parent.pid state;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
# EXACT_Q4_PHYSICAL_FLOW_BEGIN
mkdir "$stage/.trial-once" # SPENT FOREVER,including every failure
printf '%s\n' "$$" > "$stage/parent.pid"
cp -p "$stable" "$stage/stable041.backup"
exact "$stage/stable041.backup" "$stable_hash" || exit 1
printf '%s\n' BACKUP_VERIFIED_NO_EXCHANGE > "$stage/state"
exchange "$stable_hash" "$candidate_hash" 2026102100010012 2026110500010095 20 21 "$candidate" || exit 1
idle && metadata 2026110500010095 21 && exact "$installed" "$candidate_hash" && exact "$client" "$client_hash" || exit 1
printf '%s\n' FIXED_EXACT_Q4_ACTIVE_DO_NOT_INTERVENE > "$stage/state"
if "$client" --fixed-exact-q4 > "$stage/fixed.stdout" 2> "$stage/fixed.stderr";then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/fixed.exit"
cat "$stage/fixed.stdout"
cat "$stage/fixed.stderr"
if [ "$result" != 0 ];then
 printf '%s\n' UNKNOWN_EXACT_Q4_NO_END_RESET_RETRY_RESTORE > "$stage/state"
 exit 70
fi
awk '
 $0 ~ /^PASS_NATIVE_EXACT_Q4_FIXED_KERNEL rows16 dual_jobs2 exact_words32 padding_words32 polls[1-9][0-9]* generation21;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_FULLK_AFFINE_MODEL_OR_SPEED_CLAIM$/ {good++;next}
 NF{bad++} END{if(good!=1 || bad)exit 1}' "$stage/fixed.stdout" || exit 1
test ! -s "$stage/fixed.stderr" || exit 1
printf '%s\n' EXACT_Q4_CLEAN_DEVICE_AND_CLIENT_END > "$stage/state"
exact "$installed" "$candidate_hash" && idle && metadata 2026110500010095 21 || exit 1
exchange "$candidate_hash" "$stable_hash" 2026110500010095 2026102100010012 21 22 "$stage/stable041.backup" || exit 1
exact "$installed" "$stable_hash" && idle && metadata 2026102100010012 22 && health || exit 1
printf '%s\n' EXACT_Q4_CLOSED_STABLE22_RESTORED > "$stage/state"
printf '%s\n' PASS_EXACT_Q4_STABLE22_RESTORED_NO_MODEL_SPEED_CLAIM
# EXACT_Q4_PHYSICAL_FLOW_END
