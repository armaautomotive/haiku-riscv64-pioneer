#!/bin/sh
set -eu
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
helper=/boot/home/pioneer-p100a-batched-output-closure-20261102-a/pioneer_q38_batched_output_closure_exchange.sh
test -f "$helper" && test ! -L "$helper"
case "$(stat -c '%u:%h:%a' "$helper")" in 0:1:644|0:1:600|0:1:755|0:1:700) ;; *) exit 1;; esac
row=$(sha256sum "$helper");printf '%s\n' "$row"
[ "${row%% *}" = 0d8dc2eb62439daefd9acadd9a2addbc21cec36e4ccc713902025001b9a246db ]
metadata=$(/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload --status)
printf '%s\n' "$metadata"
[ "$metadata" = 'P100A_RELOAD driver55 tag2026103100010075 generation3 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY' ]
/bin/sh "$helper" --restore
