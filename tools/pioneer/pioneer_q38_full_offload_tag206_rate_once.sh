#!/bin/bash
# One full-card 32-token tag206 trial; restore stable041 only from an idle,
# non-cold state. A quarantined or owned card is left untouched for recovery.
set -eu
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
umask 077
stage=/boot/home/pioneer-q38-full-tag206-20261116-c
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
candidateSource=/boot/home/pioneer-q38-gdn-l1-tag206-20261116-a/tenstorrent_pci_probe-gdn-l1-206
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
worker=/boot/home/pioneer-q38-readiness-tag206-20261116-b/readiness-diagnostic-tag206
client=$stage/pioneer-q38-full-offload-rate-smoke
runtime=/boot/home/develop/llama-c060ca974c77/build-pioneer/bin
readonly stableHash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
readonly candidateHash=1c2ad62853c4617690956e083da8c364d27c5f1399fdc9509f8c6f781684e475
readonly workerHash=92f39ed2633499fc286f3bcd398e437c6cc2ff9cacd5f16cf298dfb4972c153b
readonly clientHash=5c9bd483cb850a92d290733ee6dec9252e94135d75e6ae7511ac87d4a56659eb
readonly adminHash=b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
readonly guardHash=d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229
exact(){
	test -f "$1" && test ! -L "$1" || return 1
	case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555|0:1:600) ;; *) return 1;; esac
	digest=$(sha256sum "$1") || return 1
	[ "${digest%% *}" = "$2" ]
}
metadata(){
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
	 if($0 ~ /(^|[\/[[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-dram|graph-csr-debug|source-check)/ ||
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
restore(){
	[ -f "$stage/candidate-active" ] || return 0
	metadata 2026111500010206 9 >/dev/null && idle || return 1
	cp -p "$stage/stable041.backup" "$stage/restore.incoming" || return 1
	chmod 755 "$stage/restore.incoming" || return 1
	exact "$stage/restore.incoming" "$stableHash" && exact "$installed" "$candidateHash" && idle || return 1
	"$admin" --quiesce 2026111500010206 || return 1
	exact "$stage/restore.incoming" "$stableHash" && exact "$installed" "$candidateHash" || return 1
	mv -f "$stage/restore.incoming" "$installed" || return 1
	sync || return 1
	rm -f "$stage/candidate-active"
	exact "$installed" "$stableHash" || return 1
	"$admin" --wait 2026102100010012 0000000000000009 || return 1
	metadata 2026102100010012 10 && idle || return 1
	printf '%s\n' RATE32_STABLE041_GEN10_RESTORED > "$stage/state"
}
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || exit 1
test ! -e "$stage/.trial-once" && test ! -L "$stage/.trial-once" || exit 1
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] || exit 1
exact "$stable" "$stableHash" && exact "$installed" "$stableHash" && exact "$candidateSource" "$candidateHash" || exit 1
exact "$worker" "$workerHash" && exact "$client" "$clientHash" || exit 1
exact "$admin" "$adminHash" && exact /boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard "$guardHash" || exit 1
exact "$runtime/libllama.so.0.2.0" 0ae23a2c211af9fbafb89c3d93bdd136a4bdd574baeeaf2d622f582efcf426af || exit 1
exact "$runtime/libggml.so.0.21.0" a3b81c133641bbdacde8dd1d0e0ffa7a0a1ec0c33e95c426df1db59735379c16 || exit 1
exact "$runtime/libggml-base.so.0.21.0" 9cb45a36558bd78aa4d4b7ed5385cbf9df32de208f9e6c9b6d064e8527dca389 || exit 1
exact "$runtime/libggml-cpu.so.0.21.0" 69e9ec2c43a123afbb256c3feb6313cdcebdf1b2a87278ac3cdac8a1a65273d2 || exit 1
original=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
test -f "$original" && test ! -L "$original" && [ "$(stat -c '%u:%h:%a:%s' "$original")" = 0:1:600:18973870528 ] || exit 1
idle && metadata 2026102100010012 8 && health || exit 1
for name in stable041.backup candidate.incoming restore.incoming state rate.stdout rate.stderr rate.exit; do
	test ! -e "$stage/$name" && test ! -L "$stage/$name" || exit 1
done
cp -p "$stable" "$stage/stable041.backup"
exact "$stage/stable041.backup" "$stableHash" || exit 1
mkdir "$stage/.trial-once"
printf '%s\n' TAG206_FULL_RATE32_TRIAL_STARTED > "$stage/state"
trap 'restore || echo RATE32_RESTORE_REQUIRED_CARD_MAY_BE_OWNED_OR_QUARANTINED >&2' EXIT HUP INT TERM
cp -p "$candidateSource" "$stage/candidate.incoming"
chmod 755 "$stage/candidate.incoming"
exact "$stage/candidate.incoming" "$candidateHash" && idle || exit 1
"$admin" --quiesce 2026102100010012 || exit 1
exact "$stage/candidate.incoming" "$candidateHash" && exact "$installed" "$stableHash" || exit 1
mv -f "$stage/candidate.incoming" "$installed" || exit 1
sync || exit 1
exact "$installed" "$candidateHash" || exit 1
touch "$stage/candidate-active"
"$admin" --wait 2026111500010206 0000000000000008 || exit 1
metadata 2026111500010206 9 && idle || exit 1
printf '%s\n' RATE32_ACTIVE_NO_CPU_INFERENCE_OR_FALLBACK > "$stage/state"
if LIBRARY_PATH="$runtime:/boot/system/lib" "$client" --full-card-rate-32 > "$stage/rate.stdout" 2> "$stage/rate.stderr"; then result=0; else result=$?; fi
printf '%s\n' "$result" > "$stage/rate.exit"
if [ "$result" != 0 ]; then
	printf '%s\n' RATE32_CLIENT_FAILED_NO_AUTOMATIC_RESTORE_UNLESS_CARD_IS_IDLE > "$stage/state"
	exit 1
fi
last=$(tail -1 "$stage/rate.stdout") || exit 1
printf '%s\n' "$last" | awk '
{rows++;if($0 ~ /^PASS_FULL_OFFLOAD_RATE_JOB clean_worker_exit1 rate_available[01] output_tokens32;CPU_INFERENCE_CONTEXTS0;POST_PROMPT_COUNT_WINDOW_NOT_BROAD_QUALITY_OR_ISOLATED_LATENCY$/)okay++}
END{if(rows!=1 || okay!=1)exit 1}' || exit 1
awk '!/^READINESS_Q4_CACHE_FRAGMENT [0-9]+\/[0-9]+$/{bad++} END{if(bad)exit 1}' "$stage/rate.stderr" || exit 1
printf '%s\n' RATE32_CLEAN_WORKER_EXIT > "$stage/state"
metadata 2026111500010206 9 && idle || exit 1
restore
trap - EXIT HUP INT TERM
health
printf '%s\n' PASS_TAG206_RATE32_CLEAN_RESTORED_NO_CPU_INFERENCE
