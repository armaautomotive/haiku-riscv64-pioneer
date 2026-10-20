#!/bin/sh
# OFFLINE prototype ROOT;no installed selector or native launch permission.
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
main=$(shasum -a 256 "$1/text.bin"); main=${main%% *}
helper=$(shasum -a 256 "$1/output.bin"); helper=${helper%% *}
[ "$main" = 8163fb311edbc4d86983a55bd3961b9e6932ee2b0c48ed4b421bcb21862596b5 ]
[ "$helper" = 50c328edcdd289e56652e463bf414185a8f7d69be71e98374e0cd83a09587dce ]
output=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-output-root-test.XXXXXXXX")
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -ffp-contract=off \
 "$directory/pioneer_q38_output_root_test.cpp" -o "$output/test"
"$output/test" "$1" "$2"
printf 'Offline unselected output-root evidence retained at %s\n' "$output"
