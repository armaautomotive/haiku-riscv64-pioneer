set -eu
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
stage=/boot/home/pioneer-native-compact-q4-stream-20261105-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
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
  if($0 ~ /(^|[\/[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-dram|pioneer-assistant-cache|graph-csr-debug|source-check)/ ||
   $0 ~ /pioneer-p100a-(dram|batched-output|wide-gdn|token)/)owners++
  next
 }
 NF{bad++} END{if(header!=1 || !teams || bad || owners)exit 1}'
}
health(){
 value=$(sysinfo) || return 1
 printf '%s\n' "$value" || return 1
 printf '%s\n' "$value" | awk '
 /^64 .*running at/{cpus++}
 /used\/max [0-9]+ \/ 137034936320\)/{ram++}
 END{if(cpus!=1 || ram!=1)exit 1}'
}
[ "$(cat "$stage/parent.exit")" = 0 ]
[ "$(cat "$stage/compact-q4-stream.exit")" = 0 ]
[ "$(cat "$stage/state")" = STABLE041_RESTORED_COMPACT_Q4_STREAM_EXACT576_PASSED ]
test ! -s "$stage/parent.stderr" && test ! -s "$stage/compact-q4-stream.stderr"
test -d "$stage/.trial-once" && test ! -L "$stage/.trial-once"
[ "$(cat "$stage/parent.pid")" = 32948 ] && [ "$(cat "$stage/wrapper.pid")" = 32941 ]
listing=$(ps -o Id -o Team)
printf '%s\n' "$listing" | awk '$1==32948 || $1==32941 {live++} END{if(live)exit 1}'
idle
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || exit 1
exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 || exit 1
exact /boot/home/config/non-packaged/servers/HaikuAssistantQwen27 861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74 || exit 1
exact /boot/home/config/non-packaged/servers/HaikuAssistantService c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18 || exit 1
exact /boot/home/config/non-packaged/apps/HaikuAssistant 0de9141eb3955cf1c993a67c80ef648b3538ce7155c17677458487ee083a4933 || exit 1
exact "$stable" "$stable_hash"
exact "$installed" "$stable_hash"
value=$("$admin" --status)
printf '%s\n' "$value"
[ "$value" = "P100A_RELOAD driver55 tag2026102100010012 generation38 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]
health
printf '%s\n' INDEPENDENT_COMPACT104_POSTFLIGHT_STABLE041_GEN38_PARENT_AND_WRAPPER_GONE_CPU64_FULL_RAM_ASSISTANT_UNCHANGED_SPENT
