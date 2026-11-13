#!/bin/sh
# One isolated host LOAD/UNLOAD qualification. Never Generate, install or retry.
set -eu
[ "$#" = 0 ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
stage=/boot/home/pioneer-batched-cache-20261112-a
runtime=/boot/home/develop/llama-c060ca974c77/build-pioneer/bin
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
driver=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
exact(){
 test -f "$1" && test ! -L "$1" || return 1
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555) ;; *) return 1;; esac
 digest=$(sha256sum "$1") || return 1
 [ "${digest%% *}" = "$2" ]
}
library(){
 exact "$runtime/$1.0.$2" "$3" || return 1
 if [ -L "$runtime/$1.0" ];then [ "$(readlink "$runtime/$1.0")" = "$1.0.$2" ]
 else exact "$runtime/$1.0" "$3";fi
}
metadata(){
 current=$("$admin" --status) || return 1
 printf '%s\n' "$current"
 [ "$current" = 'P100A_RELOAD driver55 tag2026102100010012 generation40 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY' ]
}
health(){
 sysinfo | awk '/^64 .*running at/{cpus++} /used\/max [0-9]+ \/ 137034936320\)/{ram++} END{if(cpus!=1 || ram!=1)exit 1}'
}
idle(){
 listing=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$listing" | awk -v self="$$" '
 $1=="Id" && $2=="Team" && NF==2 {header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {teams++;if($1==self)next
  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistantQwen27|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-bfp|pioneer-dram|pioneer-assistant-cache|graph-csr-debug|source-check)/){print "PREFLIGHT_OWNER",$0 > "/dev/stderr";owners++}
  next}
 NF{print "PREFLIGHT_BAD",$0 > "/dev/stderr";bad++} END{if(header!=1 || !teams || bad || owners){print "PREFLIGHT_COUNTS",header,teams,bad,owners > "/dev/stderr";exit 1}}'
}
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || exit 1
exact "$stage/worker" dd3b41e09451f34569129915b4b476a6ab4a4b34a212c1d34a6f5676a29e35b7 || exit 1
exact "$stage/probe" b3c03ab9fa2d555f1039fc698504b144f724729fb581d14317e99d387d536e34 || exit 1
exact "$driver" 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403 || exit 1
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || exit 1
library libllama.so 2.0 0ae23a2c211af9fbafb89c3d93bdd136a4bdd574baeeaf2d622f582efcf426af || exit 1
library libggml.so 21.0 a3b81c133641bbdacde8dd1d0e0ffa7a0a1ec0c33e95c426df1db59735379c16 || exit 1
library libggml-base.so 21.0 9cb45a36558bd78aa4d4b7ed5385cbf9df32de208f9e6c9b6d064e8527dca389 || exit 1
library libggml-cpu.so 21.0 69e9ec2c43a123afbb256c3feb6313cdcebdf1b2a87278ac3cdac8a1a65273d2 || exit 1
idle && metadata && health || exit 1
for suffix in pid state stdout stderr exit;do test ! -e "$stage/host.$suffix" && test ! -L "$stage/host.$suffix" || exit 1;done
mkdir "$stage/.host-once"
printf '%s\n' "$$" > "$stage/host.pid"
printf '%s\n' RUNNING_HOST_ONLY_NO_GENERATE > "$stage/host.state"
if LIBRARY_PATH="$runtime:/boot/system/lib" "$stage/probe" --load-unload-only > "$stage/host.stdout" 2> "$stage/host.stderr";then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/host.exit"
if [ "$result" != 0 ];then printf '%s\n' HOST_UNPROVEN_NO_RETRY > "$stage/host.state";exit 70;fi
if test -s "$stage/host.stderr" || test "$(tail -n 1 "$stage/host.stdout")" != 'PASS_ACTUAL_ASSISTANT_CACHE_LOAD_UNLOAD verified_tensors851;NO_GENERATE_NO_WEIGHT_UPLOAD_NO_INFERENCE_SPEED_CLAIM';then
 printf '%s\n' HOST_OUTPUT_UNQUALIFIED_NO_RETRY > "$stage/host.state";exit 1
fi
if idle && metadata && health;then :;else printf '%s\n' HOST_POSTFLIGHT_UNPROVEN_NO_RETRY > "$stage/host.state";exit 1;fi
printf '%s\n' HOST_CLOSED_ALL851_VERIFIED_NO_GENERATE > "$stage/host.state"
echo BATCHED_HOST_LOAD_CLOSED_NO_GENERATE
