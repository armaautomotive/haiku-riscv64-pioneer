#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-full-memory-boot.XXXXXXXX")
trap 'rm -f "$temporary/run/armed-full-v1" "$temporary/run/consumed-full-v1" "$temporary/run/pioneer_memory_smoke-v2" "$temporary/token" "$temporary/start.sh" "$temporary/serial"; rmdir "$temporary/run" "$temporary"' EXIT HUP INT TERM
mkdir "$temporary/run"
cp "$directory/pioneer_memory_smoke_stub.sh" "$temporary/run/pioneer_memory_smoke-v2"
chmod 700 "$temporary/run/pioneer_memory_smoke-v2"
hash=$(shasum -a 256 "$temporary/run/pioneer_memory_smoke-v2" | cut -d ' ' -f1)
sed -e "s|/boot/home/pioneer-memory-20261014|$temporary/run|g" \
 -e "s|/dev/dprintf|$temporary/serial|g" \
 -e "s|test \"\$(uname -s):\$(uname -m)\" = Haiku:riscv64|:|" \
 -e 's|test "$(id -u)" = 0|:|' \
 -e 's|sysinfo -cpu|printf "%s" "${MEMORY_BOOT_TEST_CPUS:-64}"|' \
 -e 's|sysinfo -mem|printf "%s" "${MEMORY_BOOT_TEST_INFO:-100 bytes free (used/max 10 / 136289968128)}"|' \
 -e "s|b1773fd1cdc23d1074c864ab8bd9b8b365cd08b1b699e44e02792cc33786b963|$hash|" \
 "$directory/../pioneer_full_memory_boot_smoke.sh" > "$temporary/start.sh"
export MEMORY_BOOT_TEST_BASE="$temporary/run"
for scenario in missing invalid symlink consumed lowcpu lowram threshold badinfo badhash success failure; do
 rm -f "$temporary/run/armed-full-v1" "$temporary/run/consumed-full-v1"
 printf 'full128-smp64-serial-v1\n' > "$temporary/run/armed-full-v1"
 MEMORY_BOOT_TEST_CPUS=64
 MEMORY_BOOT_TEST_INFO='100 bytes free (used/max 10 / 136289968128)'
 MEMORY_BOOT_TEST_RESULT=0
 case "$scenario" in
  missing) rm "$temporary/run/armed-full-v1";;
  invalid) printf 'other\n' > "$temporary/run/armed-full-v1";;
  symlink) mv "$temporary/run/armed-full-v1" "$temporary/token"; ln -s "$temporary/token" "$temporary/run/armed-full-v1";;
  consumed) touch "$temporary/run/consumed-full-v1";;
  lowcpu) MEMORY_BOOT_TEST_CPUS=2;;
  lowram) MEMORY_BOOT_TEST_INFO='100 bytes free (used/max 10 / 8500535296)' ;;
  threshold) MEMORY_BOOT_TEST_INFO='100 bytes free (used/max 10 / 128849018880)' ;;
  badinfo) MEMORY_BOOT_TEST_INFO='not a memory report';;
  badhash) printf '#changed\n' >> "$temporary/run/pioneer_memory_smoke-v2";;
  failure) MEMORY_BOOT_TEST_RESULT=7;;
 esac
 export MEMORY_BOOT_TEST_CPUS MEMORY_BOOT_TEST_INFO MEMORY_BOOT_TEST_RESULT
 result=0
 sh "$temporary/start.sh" || result=$?
 case "$scenario" in
  success|failure)
   test "$result" -eq "$MEMORY_BOOT_TEST_RESULT"
   grep -q 'BEGIN reported=136289968128 expected-cpus=64' "$temporary/serial"
   grep -q "END status=$MEMORY_BOOT_TEST_RESULT" "$temporary/serial"
   test ! -e "$temporary/run/armed-full-v1";;
  *) test "$result" -ne 0; ! grep -q 'BEGIN' "$temporary/serial";;
 esac
 cp "$directory/pioneer_memory_smoke_stub.sh" "$temporary/run/pioneer_memory_smoke-v2"
 chmod 700 "$temporary/run/pioneer_memory_smoke-v2"
done
grep -q 'if file_exists /boot/home/pioneer-memory-20261014/armed-full-v1' "$directory/../pioneer-full-memory-smoke.launch"
printf 'PASS: full-memory serial one-shot CPU/RAM gates, arm checks, consume-before-work and failure reporting\n'
