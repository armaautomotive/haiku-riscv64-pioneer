#!/bin/sh
# ONE separately qualified restore of already-CLOSED86 leases. NO RUN/END/reset.
# The original parent57599 exited1 on a stale generation7 check;NEVER repeat it.
set -eu
[ "$#" = 0 ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
stage=/boot/home/pioneer-native-q4-stream-restore-20261104-a
source_stage=/boot/home/pioneer-native-q4-fpu-stream-20261104-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
stable_hash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
candidate_hash=742d6d57b6a521e035d22be93e3b20afcd3dc3bc35efc0b8e12bd1a9db2f0235
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
metadata(){
 value=$("$admin" --status) || return 1
 printf '%s\n' "$value" || return 1
 [ "$value" = "P100A_RELOAD driver55 tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]
}
health(){
 value=$(sysinfo) || return 1
 printf '%s\n' "$value" || return 1
 printf '%s\n' "$value" | awk '
 /^64 .*running at/{cpus++}
 /used\/max [0-9]+ \/ 137034936320\)/{ram++}
 END{if(cpus!=1 || ram!=1)exit 1}'
}
data_exact(){
 test -f "$1" && test ! -L "$1" && [ "$(stat -c '%u:%h:%a' "$1")" = 0:1:600 ] || return 1
 digest=$(sha256sum "$1") || return 1
 [ "${digest%% *}" = "$2" ]
}
for directory in "$stage" "$source_stage";do
 test -d "$directory" && test ! -L "$directory" && [ "$(stat -c '%u:%a' "$directory")" = 0:700 ] || exit 1
done
[ "$(stat -c '%d' "$stage")" = "$(stat -c '%d' "$installed")" ] || exit 1
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] || exit 1
exact "$installed" "$candidate_hash" && exact "$stable" "$stable_hash" || exit 1
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || exit 1
exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 || exit 1
exact /boot/home/config/non-packaged/servers/HaikuAssistantQwen27 861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74 || exit 1
exact /boot/home/config/non-packaged/servers/HaikuAssistantService c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18 || exit 1
exact /boot/home/config/non-packaged/apps/HaikuAssistant 0de9141eb3955cf1c993a67c80ef648b3538ce7155c17677458487ee083a4933 || exit 1
exact "$source_stage/pioneer_native_q4_fpu_stream_physical.sh" a54de1d4ee8218e7c13be3b2fb77a4fb94f88cbecdc69b342b60ec58e36b4f14 || exit 1
exact "$source_stage/pioneer-native-q4-fpu-stream-diagnostic" 9df5d0308cbc4f14e9b2d10904df6267b02da415c4622f204244808612a22f47 || exit 1
data_exact "$source_stage/q4-stream.stdout" 7588578d8d49a8e65c62c0277fe9f4dc46f3c0186a4ef6abd2bed811b439d347 || exit 1
data_exact "$source_stage/q4-stream.stderr" e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855 || exit 1
test -f "$source_stage/q4-stream.exit" && test ! -L "$source_stage/q4-stream.exit" && [ "$(stat -c '%u:%h:%a' "$source_stage/q4-stream.exit")" = 0:1:600 ] || exit 1
test -f "$source_stage/state" && test ! -L "$source_stage/state" && [ "$(stat -c '%u:%h:%a' "$source_stage/state")" = 0:1:600 ] || exit 1
[ "$(cat "$source_stage/q4-stream.exit")" = 0 ] && [ "$(cat "$source_stage/state")" = RUNNING_Q4_STREAM_SOURCE_AUTH_AND_NINE_FULL_K_SAMPLES ] || exit 1
data_exact "$stage/observed-parent.stdout" 178f86c96b85a1df0e4bc65d97c618069d2b3b0efee0bd75b91db697b18812ff || exit 1
data_exact "$stage/observed-parent.stderr" e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855 || exit 1
test -f "$stage/observed-parent.exit" && test ! -L "$stage/observed-parent.exit" && [ "$(stat -c '%u:%h:%a' "$stage/observed-parent.exit")" = 0:1:600 ] || exit 1
[ "$(cat "$stage/observed-parent.exit")" = 1 ] || exit 1
idle && metadata 2026110400010086 9 && health || exit 1
for name in restore-once stable041.backup restore.incoming restore.state restore.pid;do
 test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
umask 077
# NATIVE_Q4_CLOSED_RESTORE_FLOW_BEGIN
mkdir "$stage/restore-once" || exit 1
printf '%s\n' "$$" > "$stage/restore.pid" || exit 1
printf '%s\n' NINE_EXACT_CLEAN_ENDS_ADMITTED_NO_GPU_EXECUTION > "$stage/restore.state" || exit 1
cp -p "$stable" "$stage/stable041.backup" || exit 1
exact "$stage/stable041.backup" "$stable_hash" || exit 1
cp -p "$stable" "$stage/restore.incoming" || exit 1
chmod 755 "$stage/restore.incoming" || exit 1
exact "$stage/restore.incoming" "$stable_hash" && exact "$installed" "$candidate_hash" && idle && metadata 2026110400010086 9 || exit 1
"$admin" --quiesce 2026110400010086 || exit 1
exact "$stage/restore.incoming" "$stable_hash" && exact "$installed" "$candidate_hash" || exit 1
mv -f "$stage/restore.incoming" "$installed" || exit 1
sync || exit 1
exact "$installed" "$stable_hash" || exit 1
"$admin" --wait 2026102100010012 0000000000000009 || exit 1
metadata 2026102100010012 10 && idle && health || exit 1
printf '%s\n' STABLE041_GENERATION10_RESTORED_AFTER_NINE_CLEAN_ENDS_ORIGINAL_PARENT_FAILED > "$stage/restore.state" || exit 1
echo CLOSED86_RESTORED_STABLE041_NO_NEW_GPU_EXECUTION_NO_RESET_NO_RETRY || exit 1
# NATIVE_Q4_CLOSED_RESTORE_FLOW_END
