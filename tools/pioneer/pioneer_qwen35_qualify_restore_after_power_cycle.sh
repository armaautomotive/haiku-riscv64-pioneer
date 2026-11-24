#!/bin/bash
# Restore stable041 only after an authorized full power cycle of the failed qualification.
set -euo pipefail
umask 077

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
candidateTag=2026112300030001
candidateSHA=a0c75c115d9763be52fcb3b27ab47bb0ca58fd613e64de920ef2ec8544195ace
stableSHA=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
adminSHA=b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
guardSHA=adbc86495803654471b83330ea8f2591f0433f8d71e083d6e9f790685bc985a4

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
			if($2~/(^|\/)(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer_assistant|pioneer[_-]qwen27|pioneer-q35-dataflow|pioneer-native-|pioneer-dram)/) bad++
			next
		}
		NF {bad++}
		END {exit !(header==1 && teams && !bad)}'
}
health() {
	sysinfo | awk '/^64 .*running at/{c++; print} /used\/max [0-9]+ \/ 137034936320\)/{m++; print} END{exit !(c==1 && m==1)}'
}

[ "$#" = 1 ] && [ "$(uname -s):$(id -u)" = Haiku:0 ] || exit 2
stage=$1
case "$stage" in
	/boot/home/pioneer-q35-recovery-qualify-20261123|/boot/home/pioneer-q35-recovery-qualify-20261123-b) ;;
	/boot/home/pioneer-q35-recovery-qualify-20261123-c)
		candidateSHA=cabebd7d42b60106c6d97216353400bbfd76a58b0f982b24389715dbc95f3e70 ;;
	/boot/home/pioneer-q35-recovery-qualify-20261123-d)
		candidateSHA=1326f14e7d2f5997b2f2048e2308fc51747b46b949e6cd7417a78a474b61b292 ;;
	/boot/home/pioneer-q35-recovery-qualify-20261123-e)
		candidateSHA=bdbcc8dc0d9823c2e249dba7eb508289be2332d70b78fe5174c3966f8841c265 ;;
	/boot/home/pioneer-q35-recovery-qualify-20261123-f)
		candidateSHA=138e21ff9cd5ecc6d3c734dac835189815164532af276df8984b6550d312e60c ;;
	/boot/home/pioneer-q35-recovery-qualify-20261123-g)
		candidateSHA=698b972450841061a27d33d4d7853a708092ede1539315788fff5afa9d4700f9 ;;
	/boot/home/pioneer-q35-recovery-qualify-20261123-h)
		candidateSHA=e685a5aa888a6b0ddaeb61245c0455a4dc6181b30cc5d005e87c15d05ab698bb ;;
	/boot/home/pioneer-q35-recovery-qualify-20261123-i)
		candidateSHA=034d147f06c2192371e00c447e5e340844ccd92683a054c370eb3a716aa03c27 ;;
	/boot/home/pioneer-q35-recovery-qualify-20261123-j)
		candidateSHA=f3b8951e9d6c47a94d298a004a9fbb8833870ee42013fcfd4632ce0817123aa8 ;;
	/boot/home/pioneer-q35-recovery-qualify-20261123-k)
		candidateTag=2026112300030002
		candidateSHA=638b3a74449e0574ccfe11e0ba078632f1c7f19c3e61d0ef64326ab04a2b5c15 ;;
	/boot/home/pioneer-q35-recovery-qualify-20261123-trace)
		candidateTag=2026112400030022
		candidateSHA=19d22e32d4665ffc1dd9ac367284e09daa7505e3b3b12e073c74f3437e5c6d45 ;;
	/boot/home/pioneer-q35-risc-sample-20261124-af)
		candidateTag=2026112400030023
		candidateSHA=ef3ec14e432cd71f0c58b5c30ca4c56c4fc67fd32dc74b41ac0e37973918b893 ;;
	/boot/home/pioneer-q35-fleet-inspect-20261123-l)
		candidateTag=2026112300030003
		candidateSHA=ed99af51ccb5b6c63a6463aea98ff08588226a58afdec846985cf04efecc5844 ;;
	/boot/home/pioneer-q35-fleet-inspect-20261123-m)
		candidateTag=2026112300030004
		candidateSHA=fb891e48d4b32b8aeb83a2c5327a9bdc0cac7e9d2e9b2d51fe99da80f4905df6 ;;
	/boot/home/pioneer-q35-role-stage-20261123-n)
		candidateTag=2026112300030005
		candidateSHA=2940a8c1f306067fc3170da96f3a0ec02da9320286b980caf9fbf8d122f46b2e ;;
	/boot/home/pioneer-q35-prestack-20261123-o)
		candidateTag=2026112300030006
		candidateSHA=e5f5a55347fe9438381626bf26b284827c9e6e524d8c30b32d7f19824d935818 ;;
	/boot/home/pioneer-q35-prestack-20261123-p)
		candidateTag=2026112300030007
		candidateSHA=29721ab1e68fb6cc247319ae7fe07a160e008b746db5574af05dfbb5e74457f5 ;;
	/boot/home/pioneer-q35-prestack-20261123-q)
		candidateTag=2026112300030008
		candidateSHA=5d5585c35d16e5fe5e19d6595ee7b1757d8a2225abb756beb5cb082536b83267 ;;
	/boot/home/pioneer-q35-prestack-20261123-r)
		candidateTag=2026112300030009
		candidateSHA=043707b207d895f523d21178c890432f4e8dafbdb56c0684642001849c8cc7e5 ;;
	/boot/home/pioneer-q35-prestack-20261123-s)
		candidateTag=2026112300030010
		candidateSHA=2776db17e4ea8be9e1ddc96395904cc5e6a935eb0706abf800eae8a1bbd8e2fa ;;
	/boot/home/pioneer-q35-local-restore-20261123-t)
		candidateTag=2026112300030011
		candidateSHA=28f938ef92e680ee3d51092b379af828925cdaa939538ee511597c8a452d13e1 ;;
	/boot/home/pioneer-q35-role-service-20261123-u)
		candidateTag=2026112300030012
		candidateSHA=eb035341f505dc5c406e0e45946fd87b379b857ba58b5ba6fb3b2b1e503fb08a ;;
	/boot/home/pioneer-q35-role-fault-20261123-v)
		candidateTag=2026112300030013
		candidateSHA=a5c45df13f5a4ba8c352167b99f308a4b81f03930a930e325415a4cc3af41e22 ;;
	/boot/home/pioneer-q35-supported-span-20261123-w)
		candidateTag=2026112300030014
		candidateSHA=7fee6e7322394343c54acb628b1ab5d3fcff938cffb873d8e9979a76821f36f2 ;;
	/boot/home/pioneer-q35-supported-span-20261123-x)
		candidateTag=2026112300030015
		candidateSHA=e0cd6d579fbb2622ec37dc70567c55602b2e5dabd2ec85afafcc51c920213949 ;;
	/boot/home/pioneer-q35-supported-span-20261124-y)
		candidateTag=2026112300030016
		candidateSHA=2c43af6bbe0e709ae20c03f4e48e3d0fb33a13f39e53662537e14a530b4b6475 ;;
	/boot/home/pioneer-q35-supported-span-20261124-z)
		candidateTag=2026112300030017
		candidateSHA=dde75fc1700b9c542ffc6b21aef8d6920d7ce27603ce984fe041782b21732011 ;;
	/boot/home/pioneer-q35-model-preload-20261124-aa)
		candidateTag=2026112300030018
		candidateSHA=0dcabe55d701e1288ad97fb3a8ac61500b9d1b09b804582c36303de644973109 ;;
	/boot/home/pioneer-q35-role-fleet-20261124-ab)
		candidateTag=2026112300030019
		candidateSHA=df25adf3c02ad8fe30a01233ea84797e1b6ad6dec21d9123c57b0a1be0d944e4 ;;
	/boot/home/pioneer-q35-role-fleet-20261124-ac)
		candidateTag=2026112300030020
		candidateSHA=89ac5cfae3d116b8b2ffdaced5f1e56e672c877918a309d11a8bb48736ae8e95 ;;
	/boot/home/pioneer-q35-role-fleet-20261124-ad)
		candidateTag=2026112300030021
		candidateSHA=8e95744222cc234bc46d85500a8dbcfd6f0795e3f68ad6a6c1a4b2a3539f3ee9 ;;
	*) exit 2;;
esac
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || fail stage
test -d "$lock" && test ! -L "$lock" && [ "$(stat -c '%u:%a' "$lock")" = 0:700 ] || fail lock
test -d "$stage/.qualification-once" && test ! -L "$stage/.qualification-once" || fail trial_marker
[ "$(cat "$lock/owner")" = "$stage" ] || fail lock_owner
case "$(cat "$stage/state")" in
	QUALIFICATION_FAILED_RETAIN_FOR_INSPECTION) ;;
	# A hung client cannot write the retained marker. The trace and fleet
	# trials admit their running marker only after the candidate-generation
	# and host-health checks below establish a fresh post-power-cycle boot.
	QUALIFICATION_RUNNING) [ "$stage" = /boot/home/pioneer-q35-recovery-qualify-20261123-trace ] || [ "$stage" = /boot/home/pioneer-q35-role-fleet-20261124-ab ] || [ "$stage" = /boot/home/pioneer-q35-role-fleet-20261124-ac ] || [ "$stage" = /boot/home/pioneer-q35-role-fleet-20261124-ad ] || [ "$stage" = /boot/home/pioneer-q35-risc-sample-20261124-af ] || fail state ;;
	# The trace trial records this before swapping drivers. A power loss
	# between the swap and client start may leave the candidate installed.
	# The candidate hash and fresh-boot generation checks below still apply.
	RESTORE_REQUIRED) [ "$stage" = /boot/home/pioneer-q35-recovery-qualify-20261123-trace ] || [ "$stage" = /boot/home/pioneer-q35-risc-sample-20261124-af ] || fail state ;;
	*) fail state ;;
esac
test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] || fail device_link
exact "$admin" "$adminSHA" && exact "$guard" "$guardSHA" || fail trusted_helpers
exact "$stable" "$stableSHA" && exact "$stage/restore.incoming" "$stableSHA" || fail stable_backup
exact "$installed" "$candidateSHA" && exact "$stage/candidate" "$candidateSHA" || fail candidate
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
rm "$lock/owner"
rmdir "$lock" || fail lock_release
sync
printf '%s\n' 'P100A_DATAFLOW_RECOVERY_AFTER_POWER_CYCLE_OK generation=2'
