#!/bin/sh
# Explicit one-shot, no accelerator access, network operations or reboot.
set -eu
exec > /dev/dprintf 2>&1
base=/boot/home/pioneer-memory-20261014
test "$(uname -s):$(uname -m)" = Haiku:riscv64
test "$(id -u)" = 0
test -d "$base" && test ! -L "$base" || exit 1
test -f "$base/armed-full-v1" && test ! -L "$base/armed-full-v1" || exit 1
test "$(cat "$base/armed-full-v1")" = full128-smp64-serial-v1
test ! -e "$base/consumed-full-v1" && test ! -L "$base/consumed-full-v1" || exit 1
test "$(sysinfo -cpu | cut -d ' ' -f1)" = 64
reported=$(sysinfo -mem | awk '/used\/max/ {gsub(/[()]/, ""); print $NF}')
case "$reported" in ''|*[!0-9]*) exit 1;; esac
test "$reported" -gt 128849018880
test -f "$base/pioneer_memory_smoke-v2" && test ! -L "$base/pioneer_memory_smoke-v2" || exit 1
cd "$base"
test "$(sha256sum pioneer_memory_smoke-v2 | cut -d ' ' -f1)" = b1773fd1cdc23d1074c864ab8bd9b8b365cd08b1b699e44e02792cc33786b963
mv armed-full-v1 consumed-full-v1
sync
printf 'PIONEER-FULL-MEMORY: BEGIN reported=%s expected-cpus=64 sample-mib=64\n' "$reported"
result=0
./pioneer_memory_smoke-v2 --mib=64 --expect-cpus=64 || result=$?
printf 'PIONEER-FULL-MEMORY: END status=%s\n' "$result"
exit "$result"
