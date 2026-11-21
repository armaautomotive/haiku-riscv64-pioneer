#!/bin/bash
# One-shot guarded run for the standalone Qwen35 resident dataflow client.
set -euo pipefail
umask 077

lock=/boot/home/.pioneer-q35-dataflow-exclusive
trusted=/boot/home/pioneer-p100a-hot-reload-20261019-a
admin=$trusted/pioneer-p100a-reload
stable=$trusted/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
stableTag=2026102100010012
candidateTag=2026112100010112
stableSHA=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
model=/boot/home/pioneer-qwen35-4b-225/Qwen3.5-4B-Q4_K_M.gguf
llama=/boot/home/develop/llama-c060ca974c77/build-pioneer/bin/libllama.so
candidate=tenstorrent_pci_probe-driver60
client=pioneer-q35-dataflow

fail() { printf 'Q35_DATAFLOW_TRIAL_FAILED %s\n' "$1" >&2; exit 1; }
absent() { test ! -e "$1" && test ! -L "$1"; }
exact() {
	test -f "$1" && test ! -L "$1" && [ "$(stat -c '%u:%h' "$1")" = 0:1 ] &&
	[ "$(sha256sum "$1" | awk '{print $1}')" = "$2" ]
}
metadata() {
	local result
	result=$("$admin" --status) || return 1
	printf '%s\n' "$result"
	[ "$result" = "P100A_RELOAD driver$1 tag$2 generation$3 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]
}
idle() {
	local listing
	listing=$(ps -o Id -o Team) || return 1
	printf '%s\n' "$listing" | awk '
		$1=="Id" && $2=="Team" && NF==2 {header++; next}
		$1~/^[0-9]+$/ && NF>=2 {
			teams++
			if($2~/(^|\/)(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q35-dataflow|pioneer-native-|pioneer-dram)/) bad++
			next
		}
		NF {bad++}
		END {exit !(header==1 && teams && !bad)}'
}
health() {
	sysinfo | awk '/^64 .*running at/{c++; print} /used\/max [0-9]+ \/ 137034936320\)/{m++; print} END{exit !(c==1 && m==1)}'
}
valid_output() {
	[ "$(cat work.exit)" = 0 ] &&
	grep -Fxq 'P100A_Q35_DATAFLOW unload_clean=1 state=2 active=0 cold=0 service_closed=1' work.stdout &&
	awk '
		$1=="P100A_Q35_DATAFLOW" && $2~/^warm_run=[12]$/ {
			delete value
			for (i=2;i<=NF;i++) {split($i,p,"="); value[p[1]]=p[2]}
			run=value["warm_run"]+0
			if (value["prompt_tokens"]==5 && value["output_tokens"]+0>=2 &&
				value["output_tokens"]+0<=8 && value["prompt_seconds"]+0>0 &&
				value["decode_seconds"]+0>0 && value["decode_tokens_per_second"]+0>=100) valid[run]++
		}
		$1=="P100A_Q35_DATAFLOW" && $2~/^generated_ids=11751,13(,|$)/ {ids++}
		/P100A_Q35_DATAFLOW output_begin run=1/ {begin[1]++}
		/P100A_Q35_DATAFLOW output_end run=1/ {end[1]++}
		/P100A_Q35_DATAFLOW output_begin run=2/ {begin[2]++}
		/P100A_Q35_DATAFLOW output_end run=2/ {end[2]++}
		END {exit !(valid[1]==1 && valid[2]==1 && ids==2 && begin[1]==1 && end[1]==1 && begin[2]==1 && end[2]==1)}
	' work.stdout
}
restore() {
	exact restore.incoming "$stableSHA" && exact "$installed" "$candidateSHA" &&
	idle && metadata 60 "$candidateTag" "$candidateGeneration" || fail before_restore
	"$admin" --quiesce "$candidateTag" || fail quiesce_candidate
	exact "$installed" "$candidateSHA" && exact restore.incoming "$stableSHA" || fail restore_changed
	mv -f restore.incoming "$installed"
	sync
	exact "$installed" "$stableSHA" &&
	"$admin" --wait "$stableTag" "$(printf '%016x' "$candidateGeneration")" &&
	metadata 55 "$stableTag" "$restoredGeneration" || fail restore_wait
	printf '%s\n' STABLE041_RESTORED > state
	rmdir "$lock"
	sync
}

[ "$#" = 5 ] && [ "$(uname -s):$(id -u)" = Haiku:0 ] || exit 2
mode=$1 stage=$2 stableGeneration=$3 candidateSHA=$4 clientSHA=$5
case "$mode" in --preflight|--run|--postflight) ;; *) exit 2;; esac
[[ $stage =~ ^/boot/home/pioneer-qwen35-dataflow-0112\.[a-zA-Z0-9]+$ &&
   $stableGeneration =~ ^[1-9][0-9]{0,8}$ && $candidateSHA =~ ^[0-9a-f]{64}$ &&
   $clientSHA =~ ^[0-9a-f]{64}$ ]] || exit 2
candidateGeneration=$((stableGeneration+1))
restoredGeneration=$((candidateGeneration+1))
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || fail stage
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] || fail device_link
cd "$stage"
test "$(stat -c %d .)" = "$(stat -c %d "$installed")" || fail filesystem
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f &&
exact "$guard" d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 &&
exact "$stable" "$stableSHA" && exact "$candidate" "$candidateSHA" &&
exact "$client" "$clientSHA" || fail hash
test -f "$model" && test ! -L "$model" && [ "$(stat -c '%u:%h:%a:%s' "$model")" = 0:1:600:2740937888 ] || fail model
original=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
test -f "$original" && test ! -L "$original" && [ "$(stat -c '%u:%h:%a:%s' "$original")" = 0:1:600:18973870528 ] || fail original_model
idle && health || fail busy

if [ "$mode" = --postflight ]; then
	absent "$lock" && exact "$installed" "$stableSHA" || fail postflight_driver
	metadata 55 "$stableTag" "$restoredGeneration" || fail postflight_generation
	test "$(cat state)" = STABLE041_RESTORED || fail terminal_state
	valid_output || fail work_output_or_cleanup
	printf '%s\n' TWO_WARM_OUTPUTS_VALIDATED
	sha256sum state work.exit work.stdout work.stderr
	exit 0
fi

absent "$lock" && absent .trial-once && exact "$installed" "$stableSHA" || fail stable_or_lock
metadata 55 "$stableTag" "$stableGeneration" || fail initial_generation
[ "$mode" = --run ] || { printf '%s\n' PREFLIGHT_OK; exit 0; }

mkdir "$lock" || fail lock
mkdir .trial-once || fail once
printf '%s\n' RECOVERY_REQUIRED_UNTIL_STABLE_RESTORED > state
cp -p "$candidate" candidate.incoming
cp -p "$stable" restore.incoming
chmod 755 candidate.incoming restore.incoming
exact candidate.incoming "$candidateSHA" && exact restore.incoming "$stableSHA" &&
	idle && metadata 55 "$stableTag" "$stableGeneration" || fail before_exchange
"$admin" --quiesce "$stableTag" || fail quiesce_stable
exact "$installed" "$stableSHA" && exact candidate.incoming "$candidateSHA" || fail candidate_changed
mv -f candidate.incoming "$installed"
sync
exact "$installed" "$candidateSHA" &&
	"$admin" --wait "$candidateTag" "$(printf '%016x' "$stableGeneration")" &&
	metadata 60 "$candidateTag" "$candidateGeneration" || fail candidate_wait

result=0
./"$client" "$model" "$llama" 'The capital of France is' 8 128 > work.stdout 2> work.stderr || result=$?
printf '%s\n' "$result" > work.exit
sync

# Restore only after the candidate has released all device ownership. If that
# proof fails, keep the candidate and lock in place for evidence-led recovery.
restore
if valid_output; then
	printf '%s\n' TWO_WARM_OUTPUTS_VALIDATED
	awk '$1=="P100A_Q35_DATAFLOW" && $2~/^warm_run=[12]$/ {print}' work.stdout
else
	fail work_output_or_cleanup
fi
