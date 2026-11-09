#!/bin/sh
# Isolated reference-checked head diagnostic, never Work inference.
# One attempt only. Unknown device outcome is retained, never retried/reset.
set -eu
[ "$#" = 0 ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
stage=/boot/home/pioneer-qgw8-head130-20261108-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
candidate=$stage/tenstorrent_pci_probe-head130
client=$stage/pioneer-qgw8-head130
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
candidate_hash=34b6007a22e85ae709e039bf0bff9afcaa2ca50520908b442512d45626be546f
client_hash=0fd16ec71b967dc063e8163b04c4cbee3060efe4c08cdabeca7ee22d842597f7
exact(){
 test -f "$1" && test ! -L "$1" || return 1
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:600|0:1:700|0:1:755|0:1:555) ;; *) return 1;; esac
 digest=$(sha256sum "$1") || return 1
 [ "${digest%% *}" = "$2" ]
}
idle(){
 listing=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$listing" | awk -v self="$$" '
 $1=="Id" && $2=="Team" && NF==2 {header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {
  teams++;if($1==self)next
  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-bfp|pioneer-native-model|pioneer-native-dram|pioneer-dram|pioneer-assistant-cache|graph-csr-debug|source-check)/ ||
   $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/)owners++
  next
 }
 NF{bad++} END{if(header!=1 || !teams || bad || owners)exit 1}'
}
metadata(){
 value=$("$admin" --status) || return 1
 printf '%s\n' "$value" || return 1
 [ "$value" = "P100A_RELOAD driver55 tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]
}
health(){
 value=$(sysinfo) || return 1
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
exact "$stage/cpu-hidden.bin" ce1ed4778227d6deb4bc0fd85440facf2a680c7d60b2e7d92c60e74abd73c98a || exit 1
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || exit 1
exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 || exit 1
original=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
test -f "$original" && test ! -L "$original" && [ "$(stat -c '%u:%h:%a:%s' "$original")" = 0:1:600:18973870528 ] || exit 1
idle && metadata 2026102100010012 2 && health || exit 1
for name in .trial-once stable041.backup driver.incoming preflight.stdout preflight.stderr preflight.exit head.stdout head.stderr head.exit parent.pid state;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
umask 077
mkdir "$stage/.trial-once"
printf '%s\n' "$$" > "$stage/parent.pid"
printf '%s\n' ADMITTED_BEFORE_SWAP > "$stage/state"
cp -p "$stable" "$stage/stable041.backup"
exact "$stage/stable041.backup" "$stable_hash" || exit 1
# HEAD130_FLOW_BEGIN
printf '%s\n' VERIFYING_SOURCE_NO_DEVICE > "$stage/state"
if "$client" --verify-source-only > "$stage/preflight.stdout" 2> "$stage/preflight.stderr";then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/preflight.exit"
[ "$result" = 0 ] && test ! -s "$stage/preflight.stderr" || exit 1
awk '
 NR==1 && $0=="HEAD130_AUTHENTICATING_ORIGINAL_REFERENCE_AND_PAYLOAD;CPU_ONLY_NO_DEVICE_YET" {next}
 NR==2 && $0=="HEAD_EXPORT_VERIFY authenticating inputs; CPU_ONLY_NO_DEVICE" {next}
 NR==3 && $0=="HEAD_EXPORT_VERIFY sample0 first0 payload_bytes189440 reference_words32;CPU_ONLY_NO_DEVICE" {next}
 NR==4 && $0=="HEAD_EXPORT_VERIFY sample1 first70240 payload_bytes189440 reference_words32;CPU_ONLY_NO_DEVICE" {next}
 NR==5 && $0=="HEAD_EXPORT_VERIFY sample2 first108128 payload_bytes189440 reference_words32;CPU_ONLY_NO_DEVICE" {next}
 NR==6 && $0=="PASS_HEAD_EXPORT_VERIFIED three head bands, independent tile reader, normalized input and 96 reference words;NO_DEVICE_OR_SPEED" {next}
 NR==7 && $0=="HEAD130_SOURCE_VERIFIED real_head_bands3 fixed_cpu_reference;DEBUG_NOT_WORK" {next}
 NR==8 && $0=="PASS_HEAD130_SOURCE_ONLY;NO_DEVICE_OPEN" {next}
 {bad++} END{if(NR!=8 || bad)exit 1}' "$stage/preflight.stdout" || exit 1
exchange "$stable_hash" "$candidate_hash" 2026102100010012 2026110800010130 2 3 "$candidate" || exit 1
idle && metadata 2026110800010130 3 && exact "$installed" "$candidate_hash" && exact "$client" "$client_hash" || exit 1
printf '%s\n' RUNNING_HEAD130_SOURCE_AUTH_AND_THREE_SAMPLES > "$stage/state"
echo HEAD130_SOURCE_AUTH_AND_THREE_SAMPLES_STARTED_WAIT_SAME_PARENT
if "$client" --fixed-qgw8-head-bands > "$stage/head.stdout" 2> "$stage/head.stderr";then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/head.exit"
cat "$stage/head.stdout"
cat "$stage/head.stderr"
if [ "$result" != 0 ];then
 printf '%s\n' UNPROVEN_HEAD130_NO_END_RESET_RETRY_RESTORE > "$stage/state"
 echo HEAD130_STOP_RETAIN_OWNER_NO_RETRY
 exit 70
fi
test ! -s "$stage/head.stderr" || exit 1
# HEAD130_OUTPUT_GATE_BEGIN
awk '
 NR==1 && $0=="HEAD130_AUTHENTICATING_ORIGINAL_REFERENCE_AND_PAYLOAD;CPU_ONLY_NO_DEVICE_YET" {next}
 NR==2 && $0=="HEAD_EXPORT_VERIFY authenticating inputs; CPU_ONLY_NO_DEVICE" {next}
 NR==3 && $0=="HEAD_EXPORT_VERIFY sample0 first0 payload_bytes189440 reference_words32;CPU_ONLY_NO_DEVICE" {next}
 NR==4 && $0=="HEAD_EXPORT_VERIFY sample1 first70240 payload_bytes189440 reference_words32;CPU_ONLY_NO_DEVICE" {next}
 NR==5 && $0=="HEAD_EXPORT_VERIFY sample2 first108128 payload_bytes189440 reference_words32;CPU_ONLY_NO_DEVICE" {next}
 NR==6 && $0=="PASS_HEAD_EXPORT_VERIFIED three head bands, independent tile reader, normalized input and 96 reference words;NO_DEVICE_OR_SPEED" {next}
 NR==7 && $0=="HEAD130_SOURCE_VERIFIED real_head_bands3 fixed_cpu_reference;DEBUG_NOT_WORK" {next}
 NR==8 && $0=="HEAD130_SAMPLE sample0 first0 exact_words64 clean_end1 generation3;DEBUG_NOT_WORK" {next}
 NR==9 && $0=="HEAD130_SAMPLE sample1 first70240 exact_words64 clean_end1 generation3;DEBUG_NOT_WORK" {next}
 NR==10 && $0=="HEAD130_SAMPLE sample2 first108128 exact_words64 clean_end1 generation3;DEBUG_NOT_WORK" {next}
 NR==11 && $0=="PASS_HEAD130 three real-head bands exact192 clean_end3;NOT_FULL_MODEL_QUALITY_OR_SPEED" {next}
 {bad++} END{if(NR!=11 || bad)exit 1}' "$stage/head.stdout" || exit 1
# HEAD130_OUTPUT_GATE_END
metadata 2026110800010130 3 || exit 1
printf '%s\n' KNOWN_CLOSED_HEAD130_BANDS_EXACT192 > "$stage/state"
exchange "$candidate_hash" "$stable_hash" 2026110800010130 2026102100010012 3 4 "$stable" || exit 1
exact "$installed" "$stable_hash" && metadata 2026102100010012 4 && idle && health || exit 1
printf '%s\n' STABLE041_RESTORED_HEAD130_BANDS_EXACT192_PASSED > "$stage/state"
echo HEAD130_KNOWN_CLEAN_NO_FULL_MODEL_SPEED_CLAIM
# HEAD130_FLOW_END
