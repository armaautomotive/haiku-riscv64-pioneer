#!/bin/bash
# One fresh candidate layer split profile raw-Q4 trial. Bound to stable generation 2.
set -euo pipefail
umask 077
stage=/boot/home/pioneer-q38-layer-split-candidate-213
pairLock=/boot/home/.pioneer-q38-head-csr-exclusive
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
candidate=$stage/tenstorrent_pci_probe-native
stable=/boot/home/pioneer-p100a-hot-reload-20261019-a/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
worker=$stage/HaikuAssistantQwen27FullOffload
client=$stage/pioneer-q38-full-offload-rate-smoke
runtime=/boot/home/develop/llama-c060ca974c77/build-pioneer/bin
readonly stableTag=2026102100010012 candidateTag=2026111700010213
readonly stableHash=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
readonly candidateHash=4b6f208da0ed2960d7d3e9a6617f3e354737b9ec4b7b7c4cf0c62d25a3fbd9ec

fail(){ printf 'REFUSED_LAYER_SPLIT_RATE %s\n' "$1" >&2; exit 1; }
exact(){
	local digest
	test -f "$1" && test ! -L "$1" || return 1
	case "$(stat -c '%u:%h:%a' "$1")" in 0:1:700|0:1:755|0:1:555|0:1:600) ;; *) return 1;; esac
	digest=$(sha256sum "$1") || return 1
	[ "${digest%% *}" = "$2" ]
}
metadata(){
	local value
	value=$("$admin" --status) || return 1
	printf '%s\n' "$value"
	[ "$value" = "P100A_RELOAD driver55 tag$1 generation$2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]
}
idle(){
	local listing
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
	sysinfo | awk '
	/^64 .*running at/{cpus++;print}
	/used\/max [0-9]+ \/ 137034936320\)/{ram++;print}
	END{if(cpus!=1 || ram!=1)exit 1}'
}
paths(){
	test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ]
}
inputs(){
	paths || fail stage
	[ "$(stat -c '%d' "$stage")" = "$(stat -c '%d' "$installed")" ] || fail different_volume
	test ! -e "$pairLock" && test ! -L "$pairLock" || fail pair_lock_present
	for name in .trial-once stable041.backup candidate.incoming restore.incoming state rate.stdout rate.stderr rate.exit; do
		test ! -e "$stage/$name" && test ! -L "$stage/$name" || fail "spent_or_dirty:$name"
	done
	test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] || fail device_link
	exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || fail admin
	exact "$guard" d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 || fail guard
	exact "$stable" "$stableHash" && exact "$installed" "$stableHash" || fail stable_driver
	exact "$candidate" "$candidateHash" || fail candidate_driver
	exact "$worker" 5c650f81142f5fc38c8f6ac24cd62ef9c4136b65144a750c47a8c8a01af148f6 || fail worker
	exact "$client" ec90ba4575b78457c64d72a43846346b64923dc16e7aee1e1b6a808361a37889 || fail client
	exact "$runtime/libllama.so.0.2.0" 0ae23a2c211af9fbafb89c3d93bdd136a4bdd574baeeaf2d622f582efcf426af || fail llama_library
	exact "$runtime/libggml.so.0.21.0" a3b81c133641bbdacde8dd1d0e0ffa7a0a1ec0c33e95c426df1db59735379c16 || fail ggml_library
	exact "$runtime/libggml-base.so.0.21.0" 9cb45a36558bd78aa4d4b7ed5385cbf9df32de208f9e6c9b6d064e8527dca389 || fail ggml_base_library
	exact "$runtime/libggml-cpu.so.0.21.0" 69e9ec2c43a123afbb256c3feb6313cdcebdf1b2a87278ac3cdac8a1a65273d2 || fail ggml_cpu_abi_library
	local original=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
	test -f "$original" && test ! -L "$original" && [ "$(stat -c '%u:%h:%a:%s' "$original")" = 0:1:600:18973870528 ] || fail original_model
	# The worker authenticates the full source before any BEGIN.
	idle && metadata "$stableTag" 2 && health || fail idle_generation_or_health
}
mark(){ printf '%s\n' "$1" > "$stage/state"; }
restore(){
	# No swap, reset or extra device command after ownership/quarantine doubt.
	paths && exact "$installed" "$candidateHash" && idle && metadata "$candidateTag" 3 || return 1
	test ! -e "$stage/restore.incoming" && test ! -L "$stage/restore.incoming" || return 1
	cp -p "$stage/stable041.backup" "$stage/restore.incoming" || return 1
	chmod 755 "$stage/restore.incoming" || return 1
	exact "$stage/restore.incoming" "$stableHash" && exact "$installed" "$candidateHash" && idle || return 1
	"$admin" --quiesce "$candidateTag" || return 1
	exact "$stage/restore.incoming" "$stableHash" && exact "$installed" "$candidateHash" || return 1
	mv -f "$stage/restore.incoming" "$installed" && sync || return 1
	exact "$installed" "$stableHash" && "$admin" --wait "$stableTag" 0000000000000003 || return 1
	metadata "$stableTag" 4 && idle && health || return 1
	mark STABLE041_GEN4_RESTORED
}
cleanup(){
	local result=$?
	trap - EXIT HUP INT TERM
	if [ "$exchange" = 1 ]; then
		if ! restore; then
			mark RESTORE_NOT_CONFIRMED_DO_NOT_RETRY_OR_RESET
			printf '%s\n' LAYER_SPLIT_RATE_RESTORE_NOT_CONFIRMED >&2
			exit 1
		fi
	fi
	rmdir "$pairLock" || fail pair_lock_release
	exit "$result"
}

[ "$#" -eq 1 ] || exit 2
case "$1" in --preflight|--run) ;; *) exit 2;; esac
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2
inputs
if [ "$1" = --preflight ]; then
	printf '%s\n' PASS_LAYER_SPLIT_RATE_PREFLIGHT_NO_DRIVER_CHANGE
	exit 0
fi
mkdir "$pairLock" || fail pair_lock
exchange=0
trap cleanup EXIT
trap 'exit 1' HUP INT TERM
mkdir "$stage/.trial-once" || fail lock
mark STAGING_LAYER_SPLIT
cp -p "$stable" "$stage/stable041.backup"
cp -p "$candidate" "$stage/candidate.incoming"
chmod 755 "$stage/candidate.incoming"
exact "$stage/stable041.backup" "$stableHash" && exact "$stage/candidate.incoming" "$candidateHash" || fail staged_hash
exact "$installed" "$stableHash" && idle && metadata "$stableTag" 2 || fail before_exchange
exchange=1
"$admin" --quiesce "$stableTag" || fail stable_quiesce
exact "$stage/candidate.incoming" "$candidateHash" && exact "$installed" "$stableHash" || fail pre_replace_hash
mv -f "$stage/candidate.incoming" "$installed" && sync || fail candidate_replace
exact "$installed" "$candidateHash" && "$admin" --wait "$candidateTag" 0000000000000002 || fail candidate_wait
idle && metadata "$candidateTag" 3 || fail candidate_generation
mark RUNNING_LAYER_SPLIT_NO_CPU_INFERENCE
if LIBRARY_PATH="$runtime:/boot/system/lib" "$client" --full-card-rate-32 > "$stage/rate.stdout" 2> "$stage/rate.stderr"; then
	result=0
else
	result=$?
fi
printf '%s\n' "$result" > "$stage/rate.exit"
[ "$result" = 0 ] || fail rate_client
tail -1 "$stage/rate.stdout" | grep -Eq '^PASS_FULL_OFFLOAD_RATE_JOB clean_worker_exit1 rate_available1 output_tokens32;CPU_INFERENCE_CONTEXTS0;POST_PROMPT_COUNT_WINDOW_NOT_BROAD_QUALITY_OR_ISOLATED_LATENCY$' || fail closed_rate_marker
test ! -s "$stage/rate.stderr" || fail nonempty_worker_stderr
awk '/^FULL_OFFLOAD_PROGRESS id2 value4294967295 Full-card layer split profile valid1 committed62 window_start31 last_position61 /{headers++}
 /^FULL_OFFLOAD_PROGRESS id2 value4294967295 Full-card layer split profile bucket[0-9]+ /{buckets++}
 END{if(headers!=1 || buckets!=14)exit 1}' "$stage/rate.stdout" || fail missing_closed_v4_timing
# A closed client is not a physical pass without stable-driver restoration
# and independent validation of the entire token/count transcript.
printf '%s\n' LAYER_SPLIT_RATE_CLIENT_CLOSED_CLEANLY
