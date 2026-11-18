#!/bin/bash
# Dedicated tag225 trial. A separate terminal check must confirm restoration.
set -euo pipefail
umask 077

stage=/boot/home/pioneer-qwen35-4b-225
lock=/boot/home/.pioneer-q38-head-csr-exclusive
trusted=/boot/home/pioneer-p100a-hot-reload-20261019-a
admin=$trusted/pioneer-p100a-reload
stable=$trusted/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
stableTag=2026102100010012
candidateTag=2026111700010225
stableSHA=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403

fail() { printf 'Q35_PROJECTION_TRIAL_REFUSED %s\n' "$1" >&2; exit 1; }
absent() { test ! -e "$1" && test ! -L "$1"; }
exact() {
	test -f "$1" && test ! -L "$1" && [ "$(stat -c '%u:%h' "$1")" = 0:1 ] &&
	printf '%s  %s\n' "$2" "$1" | sha256sum -c -
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
	printf '%s\n' "$listing" | awk -v self="$$" '
		$1=="Id" && $2=="Team" && NF==2 {header++; next}
		$1~/^[0-9]+$/ && NF>=2 {
			teams++
			if($1!=self && $0~/(^|[\/[[:space:]])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-dram)/) bad++
			next
		}
		NF {bad++}
		END {exit !(header==1 && teams && !bad)}'
}
health() {
	sysinfo | awk '/^64 .*running at/{c++; print} /used\/max [0-9]+ \/ 137034936320\)/{m++; print} END{exit !(c==1 && m==1)}'
}

[ "$#" = 4 ] && [ "$(uname -s):$(id -u)" = Haiku:0 ] || exit 2
mode=$1
stableGeneration=$2
candidateSHA=$3
clientSHA=$4
case "$mode" in --preflight|--run|--postflight) ;; *) exit 2;; esac
[[ $stableGeneration =~ ^[1-9][0-9]{0,8}$ && $candidateSHA =~ ^[0-9a-f]{64}$ && $clientSHA =~ ^[0-9a-f]{64}$ ]] || exit 2
candidateGeneration=$((stableGeneration+1))
restoredGeneration=$((candidateGeneration+1))
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || fail stage
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] || fail device_link
cd "$stage"
test "$(stat -c %d .)" = "$(stat -c %d "$installed")" || fail filesystem
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f &&
exact "$guard" d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 &&
exact "$stable" "$stableSHA" && exact "$installed" "$stableSHA" &&
exact tenstorrent_pci_probe-driver50 "$candidateSHA" &&
exact pioneer-q35-projection-validate "$clientSHA" || fail hash
model=Qwen3.5-4B-Q4_K_M.gguf
test -f "$model" && test ! -L "$model" && [ "$(stat -c '%u:%h:%a:%s' "$model")" = 0:1:600:2740937888 ] || fail model
original=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
test -f "$original" && test ! -L "$original" && [ "$(stat -c '%u:%h:%a:%s' "$original")" = 0:1:600:18973870528 ] || fail original_model
absent "$lock" && idle && health || fail busy

if [ "$mode" = --postflight ]; then
	metadata 55 "$stableTag" "$restoredGeneration" || fail postflight_generation
	test "$(cat state)" = STABLE041_RESTORED && test "$(cat projection.exit)" = 0 && test ! -s projection.stderr || fail terminal_state
	grep -Fx 'Q35_PROJECTION_VALIDATION cases=8 clean_end=1' projection.stdout || fail closure
	sha256sum state projection.exit projection.stdout projection.stderr
	exit 0
fi

metadata 55 "$stableTag" "$stableGeneration" || fail initial_generation
for file in .trial-once state projection.stdout projection.stderr projection.exit candidate.incoming restore.incoming; do
	absent "$file" || fail "stage_file_$file"
done
[ "$mode" = --run ] || exit 0

mkdir "$lock" || fail lock
mkdir .trial-once || fail once
printf '%s\n' RECOVERY_REQUIRED_UNTIL_STABLE_RESTORED > state
cp -p tenstorrent_pci_probe-driver50 candidate.incoming
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
metadata 50 "$candidateTag" "$candidateGeneration" || fail candidate_wait

result=0
./pioneer-q35-projection-validate "$model" > projection.stdout 2> projection.stderr || result=$?
printf '%s\n' "$result" > projection.exit
[ "$result" = 0 ] && test ! -s projection.stderr &&
grep -Fxq 'Q35_PROJECTION_VALIDATION cases=8 clean_end=1' projection.stdout || fail projection_or_end

exact restore.incoming "$stableSHA" && exact "$installed" "$candidateSHA" &&
idle && metadata 50 "$candidateTag" "$candidateGeneration" || fail before_restore
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
