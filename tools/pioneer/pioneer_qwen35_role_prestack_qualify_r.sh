#!/bin/bash
# One-shot B/NC pre-stack startup and cancellation. No TRISC release, model upload or inference.
set -euo pipefail
umask 077

stage=/boot/home/pioneer-q35-prestack-20261123-r
trusted=/boot/home/pioneer-p100a-hot-reload-20261019-a
admin=$trusted/pioneer-p100a-reload
stable=$trusted/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
lock=/boot/home/.pioneer-q35-dataflow-exclusive
model4=/boot/home/pioneer-qwen35-4b-225/Qwen3.5-4B-Q4_K_M.gguf
model27=/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf
stableTag=2026102100010012
candidateTag=2026112300030009
stableSHA=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
guardSHA=adbc86495803654471b83330ea8f2591f0433f8d71e083d6e9f790685bc985a4
adminSHA=b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
candidateSHA=043707b207d895f523d21178c890432f4e8dafbdb56c0684642001849c8cc7e5
clientSHA=3000c7d74eafd055b1257e7d4499cbcf4ba503ac468518a5390259e56255dd37

fail() { printf 'Q35_FLEET_INSPECTION_FAILED %s\n' "$1" >&2; exit 1; }
absent() { test ! -e "$1" && test ! -L "$1"; }
exact() {
    test -f "$1" && test ! -L "$1" && [ "$(stat -c '%u:%h' "$1")" = 0:1 ] &&
        [ "$(sha256sum "$1" | awk '{print $1}')" = "$2" ]
}
idle() {
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

[ "$#" = 1 ] && [ "$(uname -s):$(id -u)" = Haiku:0 ] || exit 2
generation=$1
[[ $generation =~ ^[1-9][0-9]{0,8}$ ]] || exit 2
test -d "$stage" && test ! -L "$stage" && [ "$(stat -c '%u:%a' "$stage")" = 0:700 ] || fail stage
cd "$stage"
test "$(stat -c %d .)" = "$(stat -c %d "$installed")" || fail filesystem
exact "$admin" "$adminSHA" && exact "$stable" "$stableSHA" && exact "$installed" "$stableSHA" &&
    exact "$guard" "$guardSHA" && exact candidate "$candidateSHA" && exact client "$clientSHA" &&
    models && idle && absent "$lock" && absent .qualification-once &&
    test -L "$device" && [ "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe ] &&
    metadata 55 "$stableTag" "$generation" || fail preflight
mkdir "$lock" || fail lock
printf '%s\n' "$stage" > "$lock/owner"
idle && metadata 55 "$stableTag" "$generation" || fail changed_after_lock
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
./client --qualify-role-prestack > qualify.stdout 2> qualify.stderr
result=$?
set -e
printf '%s\n' "$result" > qualify.exit
sync
if ! exact "$installed" "$candidateSHA" || ! exact restore.incoming "$stableSHA" ||
    ! idle || ! metadata 60 "$candidateTag" "$candidateGeneration"; then
    printf '%s\n' QUALIFICATION_FAILED_RETAIN_FOR_INSPECTION > state
    sync
    fail retained_or_unknown
fi
if [ "$result" -eq 0 ]; then
    if ! grep -Eq '^P100A_Q35_FLEET_INSPECTION replied=1 status=0 stage=4 core=120 l2cpu=0x[0-9a-f]{6}0[0-9a-f] restore_failed=0 prepared=120$' qualify.stderr ||
        ! grep -Fxq 'P100A_Q35_FLEET_INSPECTION clean_end=1' qualify.stderr ||
        ! grep -Fxq 'P100A_Q35_ROLE_STAGE replied=1 status=0 stage=9 captured=1 staged=1 restored=1 mappings_closed=1' qualify.stderr; then
        result=1
        printf '%s\n' INVALID_SUCCESS_RECEIPT >> qualify.stderr
    fi
fi
"$admin" --quiesce "$candidateTag" || fail quiesce_candidate
exact "$installed" "$candidateSHA" && exact restore.incoming "$stableSHA" || fail restore_changed
mv -f restore.incoming "$installed"
sync
exact "$installed" "$stableSHA" &&
    "$admin" --wait "$stableTag" "$(printf '%016x' "$candidateGeneration")" &&
    metadata 55 "$stableTag" "$((candidateGeneration+1))" && models && idle || fail restore
if [ "$result" -eq 0 ]; then
    printf '%s\n' ROLE_PRESTACK_PASSED_STABLE_RESTORED > state
else
    printf '%s\n' ROLE_PRESTACK_FAILED_STABLE_RESTORED > state
fi
rm "$lock/owner"
rmdir "$lock"
sync
[ "$result" -eq 0 ] || fail inspection_failed_cleanly
printf '%s\n' ROLE_PRESTACK_PASSED_STABLE_RESTORED
