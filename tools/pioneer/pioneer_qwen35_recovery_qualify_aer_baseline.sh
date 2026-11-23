#!/bin/bash
# One-shot, empty-session recovery qualification. No model or inference ioctl.
set -euo pipefail
umask 077

stage=/boot/home/pioneer-q35-recovery-qualify-20261123-h
trusted=/boot/home/pioneer-p100a-hot-reload-20261019-a
admin=$trusted/pioneer-p100a-reload
stable=$trusted/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
lock=/boot/home/.pioneer-q35-dataflow-exclusive
stableTag=2026102100010012
candidateTag=2026112300030001
stableSHA=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
oldGuardSHA=d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229
newGuardSHA=adbc86495803654471b83330ea8f2591f0433f8d71e083d6e9f790685bc985a4
adminSHA=b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
candidateSHA=e685a5aa888a6b0ddaeb61245c0455a4dc6181b30cc5d005e87c15d05ab698bb
clientSHA=1ffd7dd54584ac13a0f768b5446405973fbf18b5d6fc829c9ca2e58f16b43c1c
model4=/boot/home/pioneer-qwen35-4b-225/Qwen3.5-4B-Q4_K_M.gguf
model27=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf

fail() { printf 'Q35_RECOVERY_QUALIFY_FAILED %s\n' "$1" >&2; exit 1; }
absent() { test ! -e "$1" && test ! -L "$1"; }
exact() {
    test -f "$1" && test ! -L "$1" && [ "$(stat -c '%u:%h' "$1")" = 0:1 ] &&
        [ "$(sha256sum "$1" | awk '{print $1}')" = "$2" ]
}
idle() {
    # Haiku includes the awk command's arguments in its own ps team name.
    # Keep the program on one line so it cannot create a spurious ps row.
    ps -o Id -o Team | awk '$1=="Id" && $2=="Team" && NF==2 {header++; next} $1~/^[0-9]+$/ && NF>=2 {teams++; if($2~/(^|\/)(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q35-dataflow|pioneer-native-|pioneer-dram)/) bad++; next} NF {bad++} END {exit !(header==1 && teams && !bad)}'
}
models() {
    test -f "$model4" && test ! -L "$model4" &&
        [ "$(stat -c '%u:%h:%a:%s' "$model4")" = 0:1:600:2740937888 ] &&
        test -f "$model27" && test ! -L "$model27" &&
        [ "$(stat -c '%u:%h:%a:%s' "$model27")" = 0:1:600:18973870528 ]
}
metadata() {
    local result
    result=$("$admin" --status) || return 1
    printf '%s\n' "$result"
    [ "$result" = "P100A_RELOAD driver$1 tag$2 generation$3 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY" ]
}

[ "$#" = 2 ] && [ "$(uname -s):$(id -u)" = Haiku:0 ] || exit 2
mode=$1 generation=$2
[[ $generation =~ ^[1-9][0-9]{0,8}$ ]] || exit 2
case "$mode" in --prepare-existing-guard|--run) ;; *) exit 2;; esac
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || fail stage
cd "$stage"
test "$(stat -c %d .)" = "$(stat -c %d "$installed")" || fail filesystem
exact "$admin" "$adminSHA" && exact "$stable" "$stableSHA" &&
    exact candidate "$candidateSHA" && exact client "$clientSHA" &&
    exact new.guard "$newGuardSHA" && models && idle || fail preflight
test -L /boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe &&
    [ "$(readlink /boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe)" = ../../bin/tenstorrent_pci_probe ] || fail device

if [ "$mode" = --prepare-existing-guard ]; then
    # v2 was loaded during the prior qualification and the subsequent fresh
    # Haiku boot; the old on-disk guard remains backed up in guard.previous.
    absent "$lock" && exact guard.previous "$oldGuardSHA" &&
        exact "$installed" "$stableSHA" && exact "$guard" "$newGuardSHA" &&
        metadata 55 "$stableTag" "$generation" || fail initial_state
    mkdir "$lock" || fail lock
    printf '%s\n' "$stage" > "$lock/owner"
    idle && metadata 55 "$stableTag" "$generation" || fail changed_after_lock
    printf '%s\n' GUARD_READY > state
    sync
    printf '%s\n' GUARD_READY
    exit 0
fi

[ -d "$lock" ] && [ ! -L "$lock" ] && [ "$(cat "$lock/owner")" = "$stage" ] &&
    [ "$(cat state)" = GUARD_READY ] && absent .qualification-once &&
    exact guard.previous "$oldGuardSHA" && exact "$guard" "$newGuardSHA" &&
    exact "$installed" "$stableSHA" && metadata 55 "$stableTag" "$generation" || fail post_boot_state
mkdir .qualification-once || fail once
printf '%s\n' RESTORE_REQUIRED > state
cp -p candidate candidate.incoming
cp -p "$stable" restore.incoming
chmod 755 candidate.incoming restore.incoming
exact candidate.incoming "$candidateSHA" && exact restore.incoming "$stableSHA" &&
    idle && metadata 55 "$stableTag" "$generation" || fail before_swap
"$admin" --quiesce "$stableTag" || fail quiesce_stable
exact "$installed" "$stableSHA" && exact candidate.incoming "$candidateSHA" || fail swap_changed
mv -f candidate.incoming "$installed"
sync
candidateGeneration=$((generation+1))
exact "$installed" "$candidateSHA" &&
    "$admin" --wait "$candidateTag" "$(printf '%016x' "$generation")" &&
    metadata 60 "$candidateTag" "$candidateGeneration" || fail candidate_load
printf '%s\n' QUALIFICATION_RUNNING > state
sync
set +e
./client --qualify-recovery > qualify.stdout 2> qualify.stderr
result=$?
set -e
printf '%s\n' "$result" > qualify.exit
sync
if [ "$result" -ne 0 ]; then
    # A dirty/unknown pre-reset PCI status is rejected before quarantine or
    # any reset. Restore only if the same generation and no owner are proven.
    if grep -Eq '^P100A_Q35_RECOVERY attempt=1 stage=0 detail=0x00000000 status=-2147442676 generation_or_aer=0x[0-9a-f]{16} pre_pcie=0x[0-9a-f]{8}$' qualify.stderr &&
        exact "$installed" "$candidateSHA" && exact restore.incoming "$stableSHA" &&
        idle && metadata 60 "$candidateTag" "$candidateGeneration"; then
        "$admin" --quiesce "$candidateTag" || fail quiesce_rejected_candidate
        exact "$installed" "$candidateSHA" && exact restore.incoming "$stableSHA" || fail rejected_restore_changed
        mv -f restore.incoming "$installed"
        sync
        exact "$installed" "$stableSHA" &&
            "$admin" --wait "$stableTag" "$(printf '%016x' "$candidateGeneration")" &&
            metadata 55 "$stableTag" "$((candidateGeneration+1))" && models && idle || fail rejected_restore
        printf '%s\n' PRE_RESET_PCI_REJECTED_STABLE_RESTORED > state
        rm "$lock/owner"
        rmdir "$lock"
        sync
        fail pre_reset_pci_rejected
    fi
    printf '%s\n' QUALIFICATION_FAILED_RETAIN_FOR_INSPECTION > state
    sync
    fail qualification
fi
afterReset=$((candidateGeneration+1))
grep -Fxq 'P100A_Q35_RECOVERY_QUALIFY reset_completed=1' qualify.stderr &&
    exact "$installed" "$candidateSHA" && exact restore.incoming "$stableSHA" &&
    idle && metadata 60 "$candidateTag" "$afterReset" || fail qualification_postflight
"$admin" --quiesce "$candidateTag" || fail quiesce_candidate
exact "$installed" "$candidateSHA" && exact restore.incoming "$stableSHA" || fail restore_changed
mv -f restore.incoming "$installed"
sync
exact "$installed" "$stableSHA" &&
    "$admin" --wait "$stableTag" "$(printf '%016x' "$afterReset")" &&
    metadata 55 "$stableTag" "$((afterReset+1))" && models && idle || fail restore
printf '%s\n' RECOVERY_QUALIFIED_STABLE_RESTORED > state
rm "$lock/owner"
rmdir "$lock"
sync
printf '%s\n' RECOVERY_QUALIFIED_STABLE_RESTORED
