#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-smp-boot.XXXXXXXX")
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
mkdir "$temporary/run"
cp "$directory/pioneer_smp_smoke_stub.sh" "$temporary/run/pioneer_smp_smoke"
chmod 700 "$temporary/run/pioneer_smp_smoke"
hash=$(shasum -a 256 "$temporary/run/pioneer_smp_smoke" | cut -d ' ' -f1)
# Substitute only platform boundaries, paths and the cross-built test artifact.
sed -e "s|/boot/home/pioneer-smp-20261014|$temporary/run|g" \
 -e "s|/dev/dprintf|$temporary/serial|g" \
 -e "s|test \"\$(uname -s):\$(uname -m)\" = Haiku:riscv64|:|" \
 -e 's|test "$(id -u)" = 0|:|' \
 -e 's|sysinfo -cpu|printf "%s" "${SMP_SMOKE_TEST_CPUS:-64}"|' \
 -e "s|cbabe84a18808d4610e56c829f48e01b798a9da9f51e9b86d932fc414df3e139|$hash|" \
 "$directory/../pioneer_smp_boot_smoke.sh" > "$temporary/start.sh"
export SMP_SMOKE_TEST_BASE="$temporary/run"
for scenario in missing invalid symlink badhash singlecore success failure consumed; do
 rm -f "$temporary/run/armed" "$temporary/run/consumed-v1"
 printf 'smp64-serial-v1\n' > "$temporary/run/armed"
 SMP_SMOKE_TEST_RESULT=0
 SMP_SMOKE_TEST_CPUS=64
 case "$scenario" in
  missing) rm "$temporary/run/armed";;
  invalid) printf 'other\n' > "$temporary/run/armed";;
  symlink) mv "$temporary/run/armed" "$temporary/token"; ln -s "$temporary/token" "$temporary/run/armed";;
  badhash) chmod 600 "$temporary/run/pioneer_smp_smoke"; printf '#changed\n' >> "$temporary/run/pioneer_smp_smoke";;
  singlecore) SMP_SMOKE_TEST_CPUS=1;;
  failure) SMP_SMOKE_TEST_RESULT=7;;
  consumed) touch "$temporary/run/consumed-v1";;
 esac
 export SMP_SMOKE_TEST_RESULT SMP_SMOKE_TEST_CPUS
 result=0
 sh "$temporary/start.sh" || result=$?
 case "$scenario" in
  success|failure)
   test "$result" -eq "$SMP_SMOKE_TEST_RESULT"
   grep -q 'BEGIN expected=64' "$temporary/serial"
   grep -q "END status=$SMP_SMOKE_TEST_RESULT" "$temporary/serial"
   test ! -e "$temporary/run/armed";;
  *) test "$result" -ne 0; ! grep -q 'BEGIN' "$temporary/serial";;
 esac
 cp "$directory/pioneer_smp_smoke_stub.sh" "$temporary/run/pioneer_smp_smoke"
 chmod 700 "$temporary/run/pioneer_smp_smoke"
done
grep -q 'if file_exists /boot/home/pioneer-smp-20261014/armed' "$directory/../pioneer-smp-smoke.launch"
printf 'PASS: one-shot serial CPU test guards, hash refusal, consume-before-work and failure reporting\n'
