#!/bin/sh
# ONE fixed real-weight timed BFP4 GDDR K5120 trial. Unproven results quarantine ownership:
# never retry, issue END, reset, query GPU or restore after a failed client.
set -eu
[ "$#" = 0 ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
stage=/boot/home/pioneer-native-bfp4-dram120-20261106-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
candidate=$stage/tenstorrent_pci_probe-native-bfp4-dram120
client=$stage/pioneer-native-bfp4-dram-model-diagnostic
gate=$stage/pioneer_native_bfp4_dram_output.awk
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
candidate_hash=98e28838a606fd00f32cdee8b027894ad9e7e3a8a49021c5371d9eb44aff239f
client_hash=369d3bd742c59303a9c19fcb1886bb673844bb6656693a49b6be5ea65e766f57
gate_hash=e557a368b74cb0867c7f6c9e1f5f1d6b2abae8107afdd49ab838a537f49f5ce8
exact(){
 test -f "$1" && test ! -L "$1" || return 1
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555) ;; *) return 1;; esac
 digest=$(sha256sum "$1") || return 1
 [ "${digest%% *}" = "$2" ]
}
idle(){
 listing=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$listing" | awk -v self="$$" '
 $1=="Id" && $2=="Team" && NF==2 {header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {
  teams++;if($1==self)next
  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-bfp|pioneer-dram|pioneer-assistant-cache|graph-csr-debug|source-check)/ ||
   $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/)owners++
  next
 }
 NF{bad++} END{if(header!=1 || !teams || bad || owners)exit 1}'
}
metadata(){
 value=$("$admin" --status) || return 1
 printf '%s\n' "$value"
 [ "$value" = "P100A_RELOAD driver55 tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]
}
health(){
 value=$(sysinfo) || return 1
 printf '%s\n' "$value"
 printf '%s\n' "$value" | awk '/^64 .*running at/{cpus++} /used\/max [0-9]+ \/ 137034936320\)/{ram++} END{if(cpus!=1 || ram!=1)exit 1}'
}
exchange(){
 oldhash=$1;newhash=$2;oldtag=$3;newtag=$4;oldgeneration=$5;newgeneration=$6;replacement=$7
 exact "$installed" "$oldhash" && exact "$replacement" "$newhash" && idle || return 1
 metadata "$oldtag" "$oldgeneration" || return 1
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
test -f "$gate" && test ! -L "$gate" && [ "$(stat -c '%u:%h:%a' "$gate")" = 0:1:644 ] || exit 1
digest=$(sha256sum "$gate")
[ "${digest%% *}" = "$gate_hash" ] || exit 1
# BFP4_TIMED_PRIOR_GATE_BEGIN
prior=/boot/home/pioneer-native-bfp4-timed-119-20261106-a
test -d "$prior" && test ! -L "$prior" && [ "$(stat -c '%u:%a' "$prior")" = 0:700 ] || exit 1
for name in state band5120.stdout band5120.stderr band5120.exit;do
 test -f "$prior/$name" && test ! -L "$prior/$name" && [ "$(stat -c '%u:%h' "$prior/$name")" = 0:1 ] || exit 1
done
[ "$(cat "$prior/state")" = STABLE041_RESTORED_TIMED_BFP4_EXACT64_PASSED ] || exit 1
[ "$(cat "$prior/band5120.exit")" = 0 ] && test ! -s "$prior/band5120.stderr" || exit 1
digest=$(sha256sum "$prior/band5120.stdout")
[ "${digest%% *}" = df10a67284455c879fb0ee9661fd21bdd0007cd99c0d3686b2171c940843a3f6 ] || exit 1
# BFP4_TIMED_PRIOR_GATE_END
idle && metadata 2026102100010012 16 && health || exit 1
for name in .trial-once stable041.backup driver.incoming band5120.stdout band5120.stderr band5120.exit parent.pid state;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
mkdir "$stage/.trial-once" # NEVER removed, including failure
printf '%s\n' "$$" > "$stage/parent.pid"
printf '%s\n' ADMITTED_BEFORE_SWAP > "$stage/state"
cp -p "$stable" "$stage/stable041.backup"
exact "$stage/stable041.backup" "$stable_hash" || exit 1
# BFP4_FLOW_BEGIN
exchange "$stable_hash" "$candidate_hash" 2026102100010012 2026110600010120 16 17 "$candidate" || exit 1
printf '%s\n' NATIVE_INSTALLED_NO_GPU_STARTED > "$stage/state"
idle && metadata 2026110600010120 17 && exact "$installed" "$candidate_hash" && exact "$client" "$client_hash" || exit 1
printf '%s\n' RUNNING_THREE_TIMED_BFP4_GDDR_BANDS > "$stage/state"
if "$client" --fixed-bfp4-gddr-learned-bands > "$stage/band5120.stdout" 2> "$stage/band5120.stderr";then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/band5120.exit"
cat "$stage/band5120.stdout"
cat "$stage/band5120.stderr"
if [ "$result" != 0 ];then
 printf '%s\n' UNPROVEN_BFP4_GDDR_BANDS_NO_END_RESET_RETRY_RESTORE > "$stage/state"
 echo BFP4_STOP_RETAIN_OWNER_NO_RETRY
 exit 70
fi
test ! -s "$stage/band5120.stderr" || exit 1
# BFP4_OUTPUT_GATE_BEGIN
awk -v generation=17 -f "$gate" "$stage/band5120.stdout" || exit 1
# BFP4_OUTPUT_GATE_END
metadata 2026110600010120 17 || exit 1
printf '%s\n' KNOWN_CLOSED_BFP4_GDDR_BANDS_EXACT192 > "$stage/state"
exchange "$candidate_hash" "$stable_hash" 2026110600010120 2026102100010012 17 18 "$stable" || exit 1
exact "$installed" "$stable_hash" && metadata 2026102100010012 18 && idle && health || exit 1
printf '%s\n' STABLE041_RESTORED_BFP4_GDDR_BANDS_EXACT192_PASSED > "$stage/state"
echo BFP4_GDDR_KNOWN_CLEAN_NO_MODEL_SPEED_CLAIM
# BFP4_FLOW_END
