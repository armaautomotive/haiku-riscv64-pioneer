#!/bin/sh
# FROZEN one fixed4-token cached Work request. No install/reset/retry.
# Run ONLY after the independent host-load/unload parent has CLOSED cleanly.
set -eu
[ "$#" = 0 ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
stage=/boot/home/pioneer-lossless-cache-20261103-a
worker=$stage/HaikuAssistantQwen27-cached-quiet-model-progress
probe=$stage/pioneer-assistant-cache-inference-four-quiet-model-progress
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
runtime=/boot/home/develop/llama-c060ca974c77/build-pioneer/bin
exact(){
 test -f "$1" && test ! -L "$1" || return 1
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555) ;; *) return 1;; esac
 digest=$(sha256sum "$1") || return 1
 [ "${digest%% *}" = "$2" ]
}
metadata(){
 current=$("$admin" --status) || return 1
 printf '%s\n' "$current"
 [ "$current" = 'P100A_RELOAD driver55 tag2026102100010012 generation2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY' ]
}
idle(){
 listing=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$listing" | awk -v self="$$" '
 $1=="Id" && $2=="Team" && NF==2 {header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {
  teams++;if($1==self)next
  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistantQwen27|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-bfp|pioneer-dram|pioneer-assistant-cache|graph-csr-debug|source-check)/)owners++
  next
 }
 NF{bad++} END{if(header!=1 || !teams || bad || owners)exit 1}'
}
health(){
 value=$(sysinfo) || return 1
 printf '%s\n' "$value"
 printf '%s\n' "$value" | awk '
 /^64 .*running at/{cpus++}
 /used\/max [0-9]+ \/ 137034936320\)/{ram++}
 END{if(cpus!=1 || ram!=1)exit 1}'
}
library(){
 exact "$runtime/$1.0.$2" "$3" || return 1
 if [ -L "$runtime/$1.0" ];then
  [ "$(readlink "$runtime/$1.0")" = "$1.0.$2" ] || return 1
 else exact "$runtime/$1.0" "$3" || return 1;fi
}
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || exit 1
exact "$worker" 861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74 || exit 1
exact "$probe" 57fe9b30221fbee9538a0ed899b51277287f9f177ca41c5ec92535d606cfec4f || exit 1
exact "$installed" 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403 || exit 1
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || exit 1
exact "$guard" d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 || exit 1
library libllama.so 2.0 0ae23a2c211af9fbafb89c3d93bdd136a4bdd574baeeaf2d622f582efcf426af || exit 1
library libggml.so 21.0 a3b81c133641bbdacde8dd1d0e0ffa7a0a1ec0c33e95c426df1db59735379c16 || exit 1
library libggml-base.so 21.0 9cb45a36558bd78aa4d4b7ed5385cbf9df32de208f9e6c9b6d064e8527dca389 || exit 1
library libggml-cpu.so 21.0 69e9ec2c43a123afbb256c3feb6313cdcebdf1b2a87278ac3cdac8a1a65273d2 || exit 1
idle && metadata && health || exit 1
# Closed host-only qualification, not a lock/state-only claim of live work.
exact "$stage/pioneer-assistant-cache-host-progress-trial-861e-5578.sh" ef69794ed6b349173e24192e6d44a076885043337a91e7f1d840dcb13a81d813 || exit 1
for suffix in state exit stdout stderr;do
 record=$stage/host-progress.$suffix
 test -f "$record" && test ! -L "$record" && [ "$(stat -c '%u:%h' "$record")" = 0:1 ] || exit 1
done
test "$(cat "$stage/host-progress.exit")" = 0 || exit 1
test "$(cat "$stage/host-progress.state")" = KNOWN_CLEAN_HOST_LOAD_UNLOAD_ALL851_VERIFIED_NO_GENERATE || exit 1
test ! -s "$stage/host-progress.stderr" || exit 1
test "$(tail -n 1 "$stage/host-progress.stdout")" = 'PASS_ACTUAL_ASSISTANT_CACHE_LOAD_UNLOAD verified_tensors851;NO_GENERATE_NO_WEIGHT_UPLOAD_NO_INFERENCE_SPEED_CLAIM' || exit 1
# INFERENCE_TRIAL_FLOW_BEGIN
for suffix in pid state stdout stderr exit;do
 test ! -e "$stage/inference-four.$suffix" && test ! -L "$stage/inference-four.$suffix" || exit 1
done
mkdir "$stage/.inference-four-once-861e-57fe" # never removed, including failure
printf '%s\n' "$$" > "$stage/inference-four.pid"
printf '%s\n' RUNNING_ONE_FIXED_FOUR_TOKEN_CACHED_WORK_REQUEST > "$stage/inference-four.state"
if LIBRARY_PATH="$runtime:/boot/system/lib" "$probe" --cached-generate-four-only > "$stage/inference-four.stdout" 2> "$stage/inference-four.stderr";then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/inference-four.exit"
tail -n 30 "$stage/inference-four.stdout"
sed -n '1,40p' "$stage/inference-four.stderr"
if [ "$result" != 0 ];then
 printf '%s\n' INFERENCE_UNPROVEN_NO_RETRY_RESET_RESTORE_OR_CLEANUP > "$stage/inference-four.state"
 exit 70
fi
if test -s "$stage/inference-four.stderr" || test "$(tail -n 1 "$stage/inference-four.stdout")" != 'PASS_ACTUAL_ASSISTANT_CACHED_FOUR_TOKEN_WORK_KNOWN_CLEAN;NOT_FULL_NATIVE_NOT_20TOKENS_SEC_OR_COMPARATIVE_SPEEDUP_PROOF';then
 printf '%s\n' CLOSED_INFERENCE_OUTPUT_UNQUALIFIED_NO_RETRY_RESET_OR_CLEANUP > "$stage/inference-four.state"
 exit 1
fi
if idle && metadata && health;then :;else
 printf '%s\n' CLOSED_INFERENCE_POSTFLIGHT_UNPROVEN_NO_RETRY_RESET_OR_CLEANUP > "$stage/inference-four.state"
 exit 1
fi
printf '%s\n' KNOWN_CLEAN_CACHED_FOUR_TOKEN_WORK_REQUEST_COMPLETED > "$stage/inference-four.state"
echo GUARDED_CACHED_FOUR_TOKEN_WORK_TRIAL_CLOSED_NOT_FULL_NATIVE_OR_SPEEDUP_PROOF
# INFERENCE_TRIAL_FLOW_END
