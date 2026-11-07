#!/bin/sh
# ONE upload/release-only trial on the pinned idle stable driver. This does not
# launch the model. Failure means NO further GPU query, END, retry or reset.
set -eu
PATH=/boot/system/bin:/bin
export PATH
[ "$#" = 0 ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
umask 077
stage=/boot/home/pioneer-hybrid-upload-test.KtEQ8Xj6
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
cache4=/boot/home/pioneer-hybrid-bfp4-20261107-cache
cache8=/boot/home/pioneer-hybrid-bfp8-20261107-cache
client=$stage/upload-test
gate=$stage/upload-output.awk
exact(){
 test -f "$1" && test ! -L "$1" || return 1
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555) ;; *) return 1;; esac
 digest=$(sha256sum "$1") || return 1
 [ "${digest%% *}" = "$2" ]
}
private_dir(){ test -d "$1" && test ! -L "$1" && [ "$(stat -c '%u:%a' "$1")" = 0:700 ]; }
exact_data(){
 test -f "$1" && test ! -L "$1" || return 1
 case "$(stat -c '%u:%h:%a' "$1")" in 0:1:600|0:1:644|0:1:444) ;; *) return 1;; esac
 digest=$(sha256sum "$1") || return 1
 [ "${digest%% *}" = "$2" ]
}
metadata(){
 value=$("$admin" --status) || return 1
 [ "$value" = 'P100A_RELOAD driver55 tag2026102100010012 generation20 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY' ]
}
idle(){
 listing=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$listing" | awk -v self="$$" '
 $1=="Id" && $2=="Team" && NF==2 {header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2 {
  teams++;if($1==self)next
  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-dram|graph-csr-debug|source-check)/ ||
   $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/ || $0 ~ /(upload-test|preflight-test)/)owners++
  next
 }
 NF{bad++} END{if(header!=1 || !teams || bad || owners)exit 1}'
}
health(){
 value=$(sysinfo) || return 1
 printf '%s\n' "$value" | awk '/^64 .*running at/{cpus++} /used\/max [0-9]+ \/ 137034936320\)/{ram++} END{if(cpus!=1 || ram!=1)exit 1}'
}
private_dir "$stage" && private_dir "$cache4" && private_dir "$cache8" || exit 1
for name in .upload-once parent.pid upload.stdout upload.stderr upload.exit state.running state.failed state.clean;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
exact "$client" 560fc11a161ce54867dbe84c557a4a94d3513f4227d94903cef4e2096558e11c || exit 1
exact "$installed" 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403 || exit 1
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || exit 1
exact "$guard" d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 || exit 1
exact_data "$gate" cef4455c58280294c5e434f453052e67d679685526087029fda87cc3c319af97 || exit 1
idle && metadata && health || exit 1
mkdir "$stage/.upload-once" # permanent one-shot admission, including failure
printf '%s\n' "$$" > "$stage/parent.pid"
printf '%s\n' CHILD_RUNNING_OWNERSHIP_UNKNOWN > "$stage/state.running"
if "$client" --upload-verified-diagnostic "$cache4" "$cache8" > "$stage/upload.stdout" 2> "$stage/upload.stderr";then result=0;else result=$?;fi
printf '%s\n' "$result" > "$stage/upload.exit"
if [ "$result" != 0 ] || test -s "$stage/upload.stderr" \
 || ! exact_data "$gate" cef4455c58280294c5e434f453052e67d679685526087029fda87cc3c319af97 \
 || ! awk -f "$gate" "$stage/upload.stdout";then
 printf '%s\n' UNPROVEN_NO_GPU_QUERY_END_RETRY_OR_RESET > "$stage/state.failed"
 exit 70
fi
# Only the independently closed-success path may query the stable driver.
if ! exact "$client" 560fc11a161ce54867dbe84c557a4a94d3513f4227d94903cef4e2096558e11c \
 || ! exact "$installed" 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403 \
 || ! exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f \
 || ! exact "$guard" d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 \
 || ! exact_data "$gate" cef4455c58280294c5e434f453052e67d679685526087029fda87cc3c319af97 \
 || ! idle || ! metadata || ! health;then
 printf '%s\n' UNPROVEN_POSTFLIGHT_NO_RETRY_OR_RESET > "$stage/state.failed"
 exit 71
fi
printf '%s\n' UPLOAD_RELEASE_VERIFIED_NO_MODEL_EXECUTION > "$stage/state.clean"
echo HYBRID_UPLOAD_ONE_SHOT_CLOSED_NO_MODEL_OR_TOKEN_RATE
