#!/bin/sh
set -eu
s=/boot/home/pioneer-q38-full-fixed-config-q4-20261105-a
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
[ "$(cat "$s/parent.exit")" = 0 ] && [ "$(cat "$s/rate.exit")" = 0 ]
[ "$(cat "$s/state")" = FULL_CARD_RATE32_CLOSED_STABLE_RESTORED ]
listing=$(ps -o Id -o Team)
printf '%s\n' "$listing" | awk '
 $1=="Id" && $2=="Team" && NF==2{header++;next}
 $1 ~ /^[0-9]+$/ && NF>=2{
  rows++;if($1==28724 || $1==28725 || $1==28809 || $1==28810)bad++
  next
 }
 NF{bad++}END{if(header!=1 || !rows || bad)exit 1}'
cd "$s"
sha256sum launcher.pid launcher.stderr parent.exit parent.pid physical-parent.sh physical-parent.stderr physical-parent.stdout rate.exit rate.stderr rate.stdout state
