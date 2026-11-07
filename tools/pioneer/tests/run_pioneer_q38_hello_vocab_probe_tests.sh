#!/bin/sh
# CPU-only negative-path and role checks; does not read a model or device.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-hello-vocab.XXXXXXXX")
common='-std=c++17 -O2 -ffp-contract=off -Wall -Wextra -Werror'
${CXX:-c++} $common "$directory/pioneer_q38_hello_vocab_probe.cpp" "$directory/../PioneerModelWeightCache.cpp" -o "$temporary/probe"
if "$temporary/probe" relative.gguf;then exit 1;fi
if "$temporary/probe" /definitely-not-a-pioneer-model.gguf;then exit 1;fi
for role in _KERNEL_MODE PIONEER_Q38_WORK_INFERENCE TT_PIONEER_Q38_WORK_MODE;do
 if ${CXX:-c++} $common -D"$role" -fsyntax-only "$directory/pioneer_q38_hello_vocab_probe.cpp" 2> "$temporary/$role.stderr";then exit 1;fi
 rg -q 'standalone vocabulary inspection' "$temporary/$role.stderr"
done
printf 'PASS Hello metadata probe: missing/relative paths and 3 role refusals; NO MODEL OR DEVICE\n'
