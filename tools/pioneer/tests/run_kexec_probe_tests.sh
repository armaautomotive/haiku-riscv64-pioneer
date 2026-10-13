#!/bin/sh
set -eu
test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-kexec-probe-tests.XXXXXX")
trap 'rm -f "$temporary/format-test" "$temporary/flow-test" "$temporary/format.o"; rmdir "$temporary"' EXIT HUP INT TERM
${CC:-cc} -std=c11 -O2 -Wall -Wextra -Werror \
	-fsanitize=address,undefined \
	"$test_dir/kexec_probe_format_test.c" "$test_dir/../kexec_probe/format.c" \
	-o "$temporary/format-test"
"$temporary/format-test"
${CC:-cc} -std=c11 -O2 -Wall -Wextra -Werror -fsanitize=address,undefined \
	-Dprobe_pioneer_ram_contains=probe_real_ram_contains \
	-c "$test_dir/../kexec_probe/format.c" -o "$temporary/format.o"
${CC:-cc} -std=c11 -O2 -Wall -Wextra -Werror -fsanitize=address,undefined \
	"$test_dir/kexec_probe_flow_test.c" "$test_dir/../kexec_probe/main.c" \
	"$temporary/format.o" -o "$temporary/flow-test"
"$temporary/flow-test"
python3 "$test_dir/kexec_probe_image_test.py"
