#!/bin/sh
# One-shot serial test, armed only after the binary and this script are verified.
# No network, accelerator access, firmware operations or automatic reboot.
set -eu
exec > /dev/dprintf 2>&1
base=/boot/home/pioneer-smp-20261014
test "$(uname -s):$(uname -m)" = Haiku:riscv64
test "$(id -u)" = 0
test -d "$base" && test ! -L "$base" || exit 1
test -f "$base/armed" && test ! -L "$base/armed" || exit 1
test "$(cat "$base/armed")" = smp64-serial-v1
test ! -e "$base/consumed-v1" && test ! -L "$base/consumed-v1" || exit 1
# Loading the job during a single-core staging boot must not consume its arm.
test "$(sysinfo -cpu | cut -d ' ' -f1)" = 64
test -f "$base/pioneer_smp_smoke" && test ! -L "$base/pioneer_smp_smoke" || exit 1
cd "$base"
test "$(sha256sum pioneer_smp_smoke | cut -d ' ' -f1)" = cbabe84a18808d4610e56c829f48e01b798a9da9f51e9b86d932fc414df3e139
# Consume and flush the one-shot before executing any worker. Failed/panicking
# runs do not recur on a fallback boot. The CLI independently requires 64 CPUs.
mv armed consumed-v1
sync
printf 'PIONEER-SMP-SMOKE: BEGIN expected=64\n'
result=0
./pioneer_smp_smoke --expect-cpus=64 || result=$?
printf 'PIONEER-SMP-SMOKE: END status=%s\n' "$result"
exit "$result"
