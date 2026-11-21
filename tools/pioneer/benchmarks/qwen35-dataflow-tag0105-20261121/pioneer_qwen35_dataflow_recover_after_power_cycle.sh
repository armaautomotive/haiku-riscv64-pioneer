#!/bin/bash
# Restore stable041 only after a fresh Haiku boot has reset tag0105's cold latch.
set -euo pipefail
umask 077

stage=/boot/home/pioneer-qwen35-dataflow-0105.d
lock=/boot/home/.pioneer-q35-dataflow-exclusive
trusted=/boot/home/pioneer-p100a-hot-reload-20261019-a
admin=$trusted/pioneer-p100a-reload
stable=$trusted/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
model=/boot/home/pioneer-qwen35-4b-225/Qwen3.5-4B-Q4_K_M.gguf
original=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf

stableTag=2026102100010012
candidateTag=2026112100010105
candidateSHA=f4c120e7c00c77d72db492c116468309847521ca067eb2c849469327851991d4
stableSHA=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
adminSHA=b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
guardSHA=d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229

fail() { printf 'Q35_DATAFLOW_RECOVERY_FAILED %s\n' "$1" >&2; exit 1; }
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
			if($2~/(^|\/)(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-dram)/) bad++
			next
		}
		NF {bad++}
		END {exit !(header==1 && teams && !bad)}'
}
health() {
	sysinfo | awk '/^64 .*running at/{c++; print} /used\/max [0-9]+ \/ 137034936320\)/{m++; print} END{exit !(c==1 && m==1)}'
}

[ "$#" = 0 ] && [ "$(uname -s):$(id -u)" = Haiku:0 ] || exit 2
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || fail stage
test -d "$lock" && test ! -L "$lock" && [ "$(stat -c '%u:%a' "$lock")" = 0:700 ] || fail lock
test -d "$stage/.trial-once" && test ! -L "$stage/.trial-once" || fail trial_marker
test "$(cat "$stage/state")" = RECOVERY_REQUIRED_UNTIL_STABLE_RESTORED || fail state
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] || fail device_link
exact "$admin" "$adminSHA" && exact "$guard" "$guardSHA" || fail trusted_helpers
exact "$stable" "$stableSHA" && exact "$stage/restore.incoming" "$stableSHA" || fail stable_backup
exact "$installed" "$candidateSHA" && exact "$stage/tenstorrent_pci_probe-driver60" "$candidateSHA" || fail candidate
test -f "$model" && test ! -L "$model" && [ "$(stat -c '%u:%h:%a:%s' "$model")" = 0:1:600:2740937888 ] || fail model
test -f "$original" && test ! -L "$original" && [ "$(stat -c '%u:%h:%a:%s' "$original")" = 0:1:600:18973870528 ] || fail original_model
idle && health || fail busy_or_unhealthy
metadata 60 "$candidateTag" 1 || fail candidate_generation

"$admin" --quiesce "$candidateTag" || fail quiesce_candidate
exact "$installed" "$candidateSHA" && exact "$stage/restore.incoming" "$stableSHA" || fail restore_changed
mv -f "$stage/restore.incoming" "$installed"
sync
exact "$installed" "$stableSHA" || fail stable_hash_after_swap
"$admin" --wait "$stableTag" 0000000000000001 || fail stable_wait
metadata 55 "$stableTag" 2 || fail stable_generation

printf '%s\n' STABLE041_RESTORED > "$stage/state"
sync
rmdir "$lock" || fail lock_release
sync
printf '%s\n' 'P100A_DATAFLOW_RECOVERY_AFTER_POWER_CYCLE_OK generation=2'
