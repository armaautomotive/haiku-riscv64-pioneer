#!/bin/sh
# ONE fixed application-only cache deployment. NO GPU/driver/OS/power action.
# No restart/kill/automatic rollback. Failure preserves backups and spent token.
set -eu
[ "$#" = 1 ] && [ "$1" = --apply ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
umask 077
stage=/boot/home/pioneer-assistant-cache-installed-20261103-a
qualified=/boot/home/pioneer-lossless-cache-20261103-a
base=/boot/home/config/non-packaged/servers
worker=$base/HaikuAssistantQwen27
service=$base/HaikuAssistantService
newservice=$stage/Assistant-cached-qualifiedService
newworker=$qualified/HaikuAssistantQwen27-cached-quiet-model-progress
worker_old=ae0e5ddc075bb26df0b77ab6c1bcf0d81a7e67b8fc60d2666d7768c7e953fc75
worker_new=861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74
service_old=ed3a7d00f448e49ffc91f975d813141612333ca7c3e4b0d8701abbd38ca93735
service_new=c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
exact(){
 test -f "$1" && test ! -L "$1" || return 1
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555) ;; *) return 1;; esac
 digest=$(sha256sum "$1") || return 1
 [ "${digest%% *}" = "$2" ]
}
metadata(){
 value=$("$admin" --status) || return 1
 printf '%s\n' "$value"
 [ "$value" = 'P100A_RELOAD driver55 tag2026102100010012 generation2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY' ]
}
idle(){
 listing=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$listing" | awk -v self="$$" '
 $1=="Id" && $2=="Team" && NF==2 {header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {
  teams++;if($1==self)next
  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-bfp|pioneer-dram|pioneer-assistant-cache|graph-csr-debug|source-check)/)owners++
  next
 }
 NF{bad++} END{if(header!=1 || !teams || bad || owners)exit 1}'
}
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || exit 1
exact "$worker" "$worker_old" && exact "$service" "$service_old" || exit 1
exact "$newworker" "$worker_new" && exact "$newservice" "$service_new" || exit 1
exact /boot/home/config/non-packaged/apps/HaikuAssistant 0de9141eb3955cf1c993a67c80ef648b3538ce7155c17677458487ee083a4933 || exit 1
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || exit 1
exact /boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403 || exit 1
exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 || exit 1
runtime=/boot/home/develop/llama-c060ca974c77/build-pioneer/bin
library(){
 exact "$runtime/$1.0.$2" "$3" || return 1
 if [ -L "$runtime/$1.0" ];then [ "$(readlink "$runtime/$1.0")" = "$1.0.$2" ] || return 1
 else exact "$runtime/$1.0" "$3" || return 1;fi
}
library libllama.so 2.0 0ae23a2c211af9fbafb89c3d93bdd136a4bdd574baeeaf2d622f582efcf426af || exit 1
library libggml.so 21.0 a3b81c133641bbdacde8dd1d0e0ffa7a0a1ec0c33e95c426df1db59735379c16 || exit 1
library libggml-base.so 21.0 9cb45a36558bd78aa4d4b7ed5385cbf9df32de208f9e6c9b6d064e8527dca389 || exit 1
library libggml-cpu.so 21.0 69e9ec2c43a123afbb256c3feb6313cdcebdf1b2a87278ac3cdac8a1a65273d2 || exit 1
exact "$qualified/pioneer-assistant-cache-inference-trial-861e-57fe.sh" 0266fe6b0da6285801777dc12c4876058e79e7e68f97065d289e0253dd512e7e || exit 1
for suffix in exit state stdout stderr;do
 record=$qualified/inference-four.$suffix
 test -f "$record" && test ! -L "$record" && [ "$(stat -c '%u:%h' "$record")" = 0:1 ] || exit 1
done
[ "$(cat "$qualified/inference-four.exit")" = 0 ] || exit 1
[ "$(cat "$qualified/inference-four.state")" = KNOWN_CLEAN_CACHED_FOUR_TOKEN_WORK_REQUEST_COMPLETED ] || exit 1
test ! -s "$qualified/inference-four.stderr" || exit 1
digest=$(sha256sum "$qualified/inference-four.stdout")
[ "${digest%% *}" = 316e1655edfac771e3edbda7f7a2408385ec44676c0d0a7e443d730ec38d6a2a ] || exit 1
idle && metadata || exit 1
for target in "$worker" "$service";do
 test ! -e "$target.p100a-lossless-20261103-new" && test ! -L "$target.p100a-lossless-20261103-new" || exit 1
done
test ! -e "$stage/before" && test ! -L "$stage/before" || exit 1
# CACHE_INSTALL_FLOW_BEGIN -- actual flow is tested with fake file endpoints.
mkdir "$stage/.install-once" # intentionally never removed,including failure
mkdir -m 700 "$stage/before"
cp -p "$worker" "$stage/before/HaikuAssistantQwen27"
cp -p "$service" "$stage/before/HaikuAssistantService"
exact "$stage/before/HaikuAssistantQwen27" "$worker_old" && exact "$stage/before/HaikuAssistantService" "$service_old" || exit 1
cp -p "$newworker" "$worker.p100a-lossless-20261103-new"
cp -p "$newservice" "$service.p100a-lossless-20261103-new"
chmod 755 "$worker.p100a-lossless-20261103-new" "$service.p100a-lossless-20261103-new"
exact "$worker.p100a-lossless-20261103-new" "$worker_new" && exact "$service.p100a-lossless-20261103-new" "$service_new" || exit 1
exact "$worker" "$worker_old" && exact "$service" "$service_old" && idle && metadata || exit 1
printf '%s\n' BACKED_UP_BOTH_PREPARED_NO_REPLACEMENT > "$stage/state"
# No app is running. Worker first preserves the old service's protocol subset.
mv "$worker.p100a-lossless-20261103-new" "$worker"
exact "$worker" "$worker_new" || exit 1
printf '%s\n' CACHED_WORKER_INSTALLED_SERVICE_PENDING > "$stage/state"
mv "$service.p100a-lossless-20261103-new" "$service"
exact "$worker" "$worker_new" && exact "$service" "$service_new" && idle && metadata || exit 1
printf '%s\n' CACHED_WORKER_AND_BOUNDED_LOAD_SERVICE_INSTALLED_NOT_STARTED > "$stage/state"
echo PASS_APPLICATION_ONLY_CACHE_INSTALL_BACKUPS_RETAINED_NO_SERVICE_START_OR_GPU_DRIVER_OS_MODEL_SETTINGS_CHANGE
# CACHE_INSTALL_FLOW_END
