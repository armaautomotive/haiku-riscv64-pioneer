#!/bin/bash
# Restore stable041 after the Tag90 fault and a complete board power cycle.
set -euo pipefail
umask 077

stage=/boot/home/pioneer-qwen35-work-290.T90run1
lock=/boot/home/.pioneer-q38-head-csr-exclusive
trusted=/boot/home/pioneer-p100a-hot-reload-20261019-a
admin=$trusted/pioneer-p100a-reload
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
stable=$trusted/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
stableTag=2026102100010012
candidateTag=2026112000010090
stableSHA=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
candidateSHA=77fbcbf749af2c5c463715c21d7dc92d88593e444dbb091fc7cf9703ed81d6af
clientSHA=d8da309cdf9675c3e4e3610d2b6e6b99e0d7a519b091af26dadcf49d227d1c5d

fail() { printf 'Q35_WORK_RECOVERY_FAILED %s\n' "$1" >&2; exit 1; }
exact() {
	test -f "$1" && test ! -L "$1" && [ "$(stat -c '%u:%h' "$1")" = 0:1 ] &&
	[ "$(sha256sum "$1" | awk '{print $1}')" = "$2" ]
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
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || fail stage
cd "$stage"
exact "$admin" b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f || fail admin
exact "$guard" d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229 || fail guard
exact "$stable" "$stableSHA" || fail trusted_stable
exact tenstorrent_pci_probe-driver55 "$candidateSHA" || fail candidate
exact pioneer-q35-work "$clientSHA" || fail client
exact restore.incoming "$stableSHA" || fail stable_backup
exact work.exit 4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865 || fail work_exit
exact work.stdout 5efddcdccf5bc19343a43f7efb3ba3271cf08b69bf19f14e6750bfd87ed64f81 || fail work_stdout
exact work.stderr fb28c0ad84902e031e875fc157369dcc955440ec5d0493f7475d4169f35ddf0a || fail work_stderr
exact state fbd1473a9abb5073bbd33c4b496a571ea742253449bef0484ab678f9efd1c9c4 || fail initial_state
[ "$(cat state)" = RECOVERY_REQUIRED_UNTIL_STABLE_RESTORED ] || fail failure_state
[ "$(cat work.exit)" = 1 ] || fail work_result
test ! -e state.pre-recovery && test ! -L state.pre-recovery || fail prior_state_archive
test ! -e failure.state && test ! -L failure.state || fail prior_failure_archive

test -f /boot/home/pioneer-qwen35-4b-225/Qwen3.5-4B-Q4_K_M.gguf &&
	test ! -L /boot/home/pioneer-qwen35-4b-225/Qwen3.5-4B-Q4_K_M.gguf &&
	[ "$(stat -c '%u:%h:%a:%s' /boot/home/pioneer-qwen35-4b-225/Qwen3.5-4B-Q4_K_M.gguf)" = 0:1:600:2740937888 ] || fail qwen4b
test -f /boot/home/models/Qwen3.8-27B-Q4_K_M.gguf &&
	test ! -L /boot/home/models/Qwen3.8-27B-Q4_K_M.gguf &&
	[ "$(stat -c '%u:%h:%a:%s' /boot/home/models/Qwen3.8-27B-Q4_K_M.gguf)" = 0:1:600:18973870528 ] || fail qwen27b
idle && health || fail busy_or_health
test -d "$lock" && test ! -L "$lock" && [ "$(stat -c '%u:%a' "$lock")" = 0:700 ] || fail lock
test -z "$(find "$lock" -mindepth 1 -maxdepth 1 -print)" || fail lock_not_empty
exact "$installed" "$candidateSHA" || fail installed_candidate
expect_status "P100A_RELOAD driver64 tag${candidateTag} generation1 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY"

"$admin" --quiesce "$candidateTag" || fail quiesce_candidate
expect_status "P100A_RELOAD driver64 tag${candidateTag} generation1 quiesced1 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY"
exact "$installed" "$candidateSHA" && exact restore.incoming "$stableSHA" || fail changed_during_quiesce
mv -f restore.incoming "$installed"
sync
exact "$installed" "$stableSHA" || fail stable_installed
"$admin" --wait "$stableTag" 0000000000000001 || fail stable_reload
expect_status 'P100A_RELOAD driver55 tag2026102100010012 generation2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY'

cp -p state state.pre-recovery
cp -p state failure.state
printf '%s\n' STABLE041_RESTORED > state.next
chmod 600 state.next
mv -f state.next state
sync
rmdir "$lock" || fail lock_cleanup
test ! -e "$lock" && test ! -L "$lock" || fail lock_remains
printf 'STABLE041_RESTORED tag=%s generation=2 cold0 active_owner0 guard1 lock=absent\n' "$stableTag"
