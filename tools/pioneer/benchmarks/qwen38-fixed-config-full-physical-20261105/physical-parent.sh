#!/bin/sh
# ONE full-card fixed-config-Q4 tag103 rate trial;fixed generation34 admission. UNKNOWN stops;NO retry/reset/force-unload/auto restore.
set -eu
[ "$#" = 1 ] && [ "$1" = --rate-32 ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
umask 077
stage=/boot/home/pioneer-q38-full-fixed-config-q4-20261105-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
candidate=$stage/tenstorrent_pci_probe-full-fixed-config-q4-103
worker=$stage/HaikuAssistantQwen27FullOffload-fixed-config-q4
client=$stage/pioneer-q38-full-offload-rate-smoke
readonly stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
readonly candidate_hash=608ae0a091b944c3e9ae96d08bdc4aa74a3bd059c56ee65a414445f29b1ced20
readonly worker_hash=c50acf30e31890f0abf65bd061f2cdee65e3ec31aa30d43dcfabcfad9d870bff
readonly client_hash=3988d149f607deca9c41c83b538b161a5c448af0b37a3d6253c5f5e62d1759db
readonly stable_tag=2026102100010012 native_tag=2026110500010103
readonly stable_generation=34 native_generation=35 restored_generation=36
clean_tail(){
 [ "$#" = 1 ] || return 1
 printf '%s\n' "$1" | awk '
 {rows++;if($0 ~ /^PASS_FULL_OFFLOAD_RATE_JOB clean_worker_exit1 rate_available[01] output_tokens([0-9]|[12][0-9]|3[0-2]);CPU_INFERENCE_CONTEXTS0;POST_PROMPT_COUNT_WINDOW_NOT_BROAD_QUALITY_OR_ISOLATED_LATENCY$/)okay++}
 END{if(rows!=1 || okay!=1)exit 1}'
} # final DATA shape only;actual child0+clean worker exit still required
exact(){
 test -f "$1" && test ! -L "$1" || return 1
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555) ;; *) return 1;; esac
 digest=$(sha256sum "$1") || return 1
 [ "${digest%% *}" = "$2" ]
}
metadata(){
 [ "$#" = 1 ] || return 1
 case "$1" in
  stable-before) set -- "$stable_tag" "$stable_generation";;
  native) set -- "$native_tag" "$native_generation";;
  stable-after) set -- "$stable_tag" "$restored_generation";;
  *) return 1;;
 esac
 value=$("$admin" --status) || return 1
 printf '%s\n' "$value"
 [ "$value" = "P100A_RELOAD driver55 tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]
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
health(){
 value=$(sysinfo) || return 1
 printf '%s\n' "$value"
 printf '%s\n' "$value" | awk '
 /^64 .*running at/{cpus++}
 /used\/max [0-9]+ \/ 137034936320\)/{ram++}
 END{if(cpus!=1 || ram!=1)exit 1}'
}
exchange(){
 [ "$#" = 3 ] || return 1
 case "$1:$2" in
  stable-before:native)
   oldhash=$stable_hash;newhash=$candidate_hash;oldtag=$stable_tag;newtag=$native_tag;oldgeneration=$stable_generation;;
  native:stable-after)
   oldhash=$candidate_hash;newhash=$stable_hash;oldtag=$native_tag;newtag=$stable_tag;oldgeneration=$native_generation;;
  *) return 1;;
 esac
 replacement=$3
 exact "$installed" "$oldhash" && exact "$replacement" "$newhash" && idle && metadata "$1" || return 1
 test ! -e "$stage/driver.incoming" && test ! -L "$stage/driver.incoming" || return 1
 cp -p "$replacement" "$stage/driver.incoming" || return 1
 chmod 755 "$stage/driver.incoming" || return 1
 exact "$stage/driver.incoming" "$newhash" && exact "$installed" "$oldhash" && idle || return 1
 "$admin" --quiesce "$oldtag" || return 1
 exact "$stage/driver.incoming" "$newhash" && exact "$installed" "$oldhash" || return 1
 mv -f "$stage/driver.incoming" "$installed" || return 1
 sync || return 1
 exact "$installed" "$newhash" || return 1
 previous=$(printf '%016x' "$oldgeneration") || return 1
 "$admin" --wait "$newtag" "$previous" || return 1
 metadata "$2"
}
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || exit 1
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] || exit 1
[ "$(stat -c '%d' "$stage")" = "$(stat -c '%d' "$installed")" ] || exit 1
exact "$stable" "$stable_hash" && exact "$installed" "$stable_hash" && exact "$candidate" "$candidate_hash" || exit 1
exact "$worker" "$worker_hash" && exact "$client" "$client_hash" || exit 1
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || exit 1
exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 || exit 1
exact /boot/home/config/non-packaged/servers/HaikuAssistantQwen27 861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74 || exit 1
exact /boot/home/config/non-packaged/servers/HaikuAssistantService c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18 || exit 1
exact /boot/home/config/non-packaged/apps/HaikuAssistant 0de9141eb3955cf1c993a67c80ef648b3538ce7155c17677458487ee083a4933 || exit 1
runtime=/boot/home/develop/llama-c060ca974c77/build-pioneer/bin
exact "$runtime/libllama.so.0.2.0" 0ae23a2c211af9fbafb89c3d93bdd136a4bdd574baeeaf2d622f582efcf426af || exit 1
exact "$runtime/libggml.so.0.21.0" a3b81c133641bbdacde8dd1d0e0ffa7a0a1ec0c33e95c426df1db59735379c16 || exit 1
exact "$runtime/libggml-base.so.0.21.0" 9cb45a36558bd78aa4d4b7ed5385cbf9df32de208f9e6c9b6d064e8527dca389 || exit 1
exact "$runtime/libggml-cpu.so.0.21.0" 69e9ec2c43a123afbb256c3feb6313cdcebdf1b2a87278ac3cdac8a1a65273d2 || exit 1
original=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
test -f "$original" && test ! -L "$original" && [ "$(stat -c '%u:%h:%a:%s' "$original")" = 0:1:600:18973870528 ] || exit 1
# Whole SAME-open SHA/all851 layout authenticate inside the worker BEFORE
# device metadata,BEGIN,upload and execution. No host model products enter.
idle && metadata stable-before && health || exit 1
for name in .trial-once stable041.backup driver.incoming rate.stdout rate.stderr rate.exit parent.pid state;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
# FULL_OFFLOAD_PHYSICAL_FLOW_BEGIN
mkdir "$stage/.trial-once" # spent forever,even after failure
printf '%s\n' "$$" > "$stage/parent.pid"
cp -p "$stable" "$stage/stable041.backup"
exact "$stage/stable041.backup" "$stable_hash" || exit 1
printf '%s\n' BACKUP_VERIFIED_NO_EXCHANGE > "$stage/state"
exchange stable-before native "$candidate" || exit 1
exact "$worker" "$worker_hash" && exact "$client" "$client_hash" && idle && metadata native || exit 1
printf '%s\n' FULL_CARD_RATE32_ACTIVE_DO_NOT_INTERVENE > "$stage/state"
if LIBRARY_PATH="$runtime:/boot/system/lib" "$client" --full-card-rate-32 > "$stage/rate.stdout" 2> "$stage/rate.stderr";then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/rate.exit"
[ "$result" = 0 ] && test ! -s "$stage/rate.stderr" || exit 1
last=$(tail -1 "$stage/rate.stdout") || exit 1
clean_tail "$last" || exit 1
printf '%s\n' FULL_CARD_RATE32_CLEAN_DEVICE_AND_WORKER_END > "$stage/state"
exact "$installed" "$candidate_hash" && idle && metadata native || exit 1
exchange native stable-after "$stage/stable041.backup" || exit 1
exact "$installed" "$stable_hash" && idle && metadata stable-after && health || exit 1
printf '%s\n' FULL_CARD_RATE32_CLOSED_STABLE_RESTORED > "$stage/state"
printf '%s\n' PASS_FULL_CARD_RATE32_STABLE_RESTORED_NO_ASSISTANT_INSTALL_OR_SPEED_CLAIM
# FULL_OFFLOAD_PHYSICAL_FLOW_END
