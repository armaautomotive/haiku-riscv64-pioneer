#!/bin/sh
# Files/processes ONLY. NEVER open probe/admin while a lease may be active.
set -eu
stage=/boot/home/pioneer-native-dram-model-band-20261104-a
test -d "$stage" && test ! -L "$stage"
for name in state gddr.stdout gddr.stderr;do
 test -f "$stage/$name" && test ! -L "$stage/$name"
 stat -c '%n %s bytes' "$stage/$name"
 tail -n 3 "$stage/$name"
done
listing=$(ps -o Id -o Team)
printf '%s\n' "$listing" | awk '$0 ~ /pioneer-native-dram-model-band/ {print}'
echo PROGRESS_FILES_PS_ONLY_NO_PROBE_FD_OR_HARDWARE_ACCESS
