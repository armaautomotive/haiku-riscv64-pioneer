#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-rtl-test.XXXXXXXX")
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
awk '
$0 == "rge_tick(void *arg)" { print "void"; copy = 1 }
copy { print }
copy && $0 == "}" { exit }
' "$directory/../../../src/add-ons/kernel/drivers/network/ether/rtl8125/dev/pci/if_rge.c" > "$temporary/production_tick.inc"
test -s "$temporary/production_tick.inc"
awk '
/Explicit private warm-boot experiment/ { copy = 1 }
/intrstr = pci_intr_string/ { exit }
copy { print }
' "$directory/../../../src/add-ons/kernel/drivers/network/ether/rtl8125/dev/pci/if_rge.c" > "$temporary/production_select.inc"
test -s "$temporary/production_select.inc"
for profile in enabled disabled; do
	if [ "$profile" = enabled ]; then flags='-D__riscv -DHAIKU_PIONEER_RTL_DIAGNOSTICS=1'; else flags='-D__riscv'; fi
	${CC:-cc} -std=c11 -Wall -Wextra -Werror -Wno-unused-function \
		-fsanitize=address,undefined $flags -I "$temporary" \
		"$directory/pioneer_rtl_diagnostics_test.c" -o "$temporary/test"
	"$temporary/test"
	if [ "$profile" = enabled ]; then flags='-D__riscv -DHAIKU_PIONEER_RTL_MSI_ONLY=1'; else flags='-D__riscv'; fi
	${CC:-cc} -std=c11 -Wall -Wextra -Werror -Wno-unused-function \
		-fsanitize=address,undefined -D__HAIKU__ $flags -I "$temporary" \
		"$directory/pioneer_rtl_msi_selection_test.c" -o "$temporary/test"
	"$temporary/test" > /dev/null
done
printf 'PASS: production RTL diagnostics bounded/read-only; MSI selection/failure and normal fallback\n'
