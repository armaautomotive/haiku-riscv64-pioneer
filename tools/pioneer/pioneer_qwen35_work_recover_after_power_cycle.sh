#!/bin/bash
# Restore stable041 after the tag228 Work fault and a complete board power cycle.
set -euo pipefail
umask 077

stage=/boot/home/pioneer-qwen35-work-228.k4m9Tr
lock=/boot/home/.pioneer-q38-head-csr-exclusive
trusted=/boot/home/pioneer-p100a-hot-reload-20261019-a
admin=$trusted/pioneer-p100a-reload
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
stable=$trusted/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
stableTag=2026102100010012
candidateTag=2026111800010028
stableSHA=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
candidateSHA=04ddf2e6a69f7d19aa1f2ac0a932031f3b60cbebc44c42856589fc9503aafe0c

fail() { printf 'Q35_WORK_RECOVERY_STOP %s\n' "$1" >&2; exit 1; }
exact() {
	[ -f "$1" ] && [ ! -L "$1" ] && [ "$(stat -c '%u:%h' "$1")" = 0:1 ] &&
	printf '%s  %s\n' "$2" "$1" | sha256sum -c -
}
expect_status() {
	local expected=$1 actual
	actual=$("$admin" --status) || fail status_query
	printf '%s\n' "$actual"
	[ "$actual" = "$expected" ] || fail driver_status
}
idle() {
	local listing
	listing=$(ps -o Id -o Team) || return 1
	printf '%s\n' "$listing" | awk -v self="$$" '
		$1=="Id" && $2=="Team" && NF==2 {header++; next}
		$1~/^[0-9]+$/ && NF>=2 {
			teams++
			if($1!=self && $2~/(^|\/)(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native-|pioneer-dram)/) bad++
			next
		}
		NF {bad++}
		END {exit !(header==1 && teams && !bad)}'
}
health() {
	sysinfo | awk '/^64 .*running at/{c++; print} /used\/max [0-9]+ \/ 137034936320\)/{m++; print} END{exit !(c==1 && m==1)}'
}

[ "$#" = 0 ] && [ "$(uname -s):$(id -u)" = Haiku:0 ] || exit 2
[ -d "$stage" ] && [ ! -L "$stage" ] && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || fail stage
cd "$stage"
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || fail admin
exact "$guard" d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 || fail guard
exact "$stable" "$stableSHA" || fail trusted_stable
exact tenstorrent_pci_probe-driver55 "$candidateSHA" || fail candidate
exact pioneer-q35-work c588a5cafd5a76e09ddf71a00139da783e6b9754850914e9881d9a86694cf513 || fail client
exact pioneer_qwen35_work_trial.sh 75241081bf61b82f2355e992899e7c908dd267dc890e972490419968d9d5bb88 || fail trial_script
exact work.stdout 31fa9a7a510fc4fd9e22565f9675cf3f5ef1b9a43c5757d11a3de933447f714e || fail work_stdout
exact work.stderr 018c7727a3f633606fdb27653f5d22ee4cc52022471a0daf5e8b7681b8cad1fd || fail work_stderr
exact restore.incoming "$stableSHA" || fail stable_backup
[ "$(cat state)" = RECOVERY_REQUIRED_UNTIL_STABLE_RESTORED ] || fail failure_state
[ "$(cat work.exit)" = 1 ] || fail work_exit
[ ! -e failure.state ] && [ ! -L failure.state ] || fail failure_state_exists

test -f /boot/home/pioneer-qwen35-4b-225/Qwen3.5-4B-Q4_K_M.gguf &&
	[ "$(stat -c '%u:%h:%a:%s' /boot/home/pioneer-qwen35-4b-225/Qwen3.5-4B-Q4_K_M.gguf)" = 0:1:600:2740937888 ] || fail qwen4b
test -f /boot/home/models/Qwen3.8-27B-Q4_K_M.gguf &&
	[ "$(stat -c '%u:%h:%a:%s' /boot/home/models/Qwen3.8-27B-Q4_K_M.gguf)" = 0:1:600:18973870528 ] || fail qwen27b
idle && health || fail busy_or_health
[ -d "$lock" ] && [ ! -L "$lock" ] && [ "$(stat -c '%u:%a' "$lock")" = 0:700 ] || fail lock
[ -z "$(find "$lock" -mindepth 1 -maxdepth 1 -print)" ] || fail lock_not_empty
exact "$installed" "$candidateSHA" || fail installed_candidate
expect_status 'P100A_RELOAD driver60 tag2026111800010028 generation1 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY'

"$admin" --quiesce "$candidateTag" || fail quiesce_candidate
expect_status 'P100A_RELOAD driver60 tag2026111800010028 generation1 quiesced1 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY'
exact "$installed" "$candidateSHA" && exact restore.incoming "$stableSHA" || fail changed_during_quiesce
mv -f restore.incoming "$installed"
sync
exact "$installed" "$stableSHA" || fail stable_installed
"$admin" --wait "$stableTag" 0000000000000001 || fail stable_reload
expect_status 'P100A_RELOAD driver55 tag2026102100010012 generation2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY'

cp -p state failure.state
[ "$(cat failure.state)" = RECOVERY_REQUIRED_UNTIL_STABLE_RESTORED ] || fail failed_state_copy
printf '%s\n' STABLE041_RESTORED > state.next
chmod 600 state.next
mv -f state.next state
sync
rmdir "$lock" || fail lock_cleanup
[ ! -e "$lock" ] && [ ! -L "$lock" ] || fail lock_remains
printf 'STABLE041_RESTORED tag=%s generation=2 cold0 active_owner0 guard1 lock=absent\n' "$stableTag"
