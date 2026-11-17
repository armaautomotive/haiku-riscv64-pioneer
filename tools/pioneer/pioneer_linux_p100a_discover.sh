#!/bin/sh
# Read-only Linux PCI inventory. This does not enable, rescan, reset or bind.
set -eu
[ "${1-}" = --sysfs-test-root ] && [ "$#" = 2 ] && root=$2 || {
 [ "$#" = 0 ] || { echo 'usage: pioneer_linux_p100a_discover.sh' >&2; exit 2; }
 root=/sys/bus/pci/devices
}
[ -d "$root" ] || { echo 'P100A_DISCOVERY_ERROR sysfs_missing' >&2; exit 1; }
count=0
found=
for d in "$root"/*; do
 [ -r "$d/vendor" ] && [ -r "$d/device" ] || continue
 [ "$(cat "$d/vendor")" = 0x1e52 ] && [ "$(cat "$d/device")" = 0xb140 ] || continue
 count=$((count+1)); found=$d
done
if [ "$count" -ne 1 ]; then
 echo "P100A_DISCOVERY_REFUSED matches=$count;NO_PCI_WRITES"
 exit 1
fi
bdf=${found##*/}
printf '%s\n' "$bdf" | grep -Eq '^[0-9a-fA-F]{4}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}\.[0-7]$' || {
 echo 'P100A_DISCOVERY_ERROR invalid_bdf' >&2; exit 1;
}
for f in subsystem_vendor subsystem_device class enable; do
 [ -r "$found/$f" ] || { echo "P100A_DISCOVERY_ERROR missing_$f" >&2; exit 1; }
done
[ "$(cat "$found/subsystem_vendor")" = 0x1e52 ] && [ "$(cat "$found/subsystem_device")" = 0x0043 ] || {
 echo 'P100A_DISCOVERY_REFUSED unexpected_subsystem;NO_PCI_WRITES' >&2; exit 1;
}
printf 'P100A_DISCOVERY bdf=%s subsystem=1e52:0043 enable=%s;READ_ONLY\n' "$bdf" "$(cat "$found/enable")"
printf 'SYSFS_PATH %s\n' "$(readlink -f "$found")"
if [ -L "$found/driver" ]; then printf 'DRIVER %s\n' "$(readlink -f "$found/driver")"; else echo 'DRIVER unbound'; fi
for f in current_link_speed current_link_width max_link_speed max_link_width; do
 if [ -r "$found/$f" ]; then printf '%s %s\n' "$f" "$(cat "$found/$f")"; fi
done
[ ! -r "$found/resource" ] || { echo 'RESOURCE_WINDOWS'; cat "$found/resource"; }
echo 'LSPCI_VERBOSE_BEGIN'
lspci -D -s "$bdf" -vv
echo 'LSPCI_VERBOSE_END'
echo 'P100A_DISCOVERY_COMPLETE;NO_PCI_WRITES;NOT_PERMISSION_TO_ENABLE_OR_MAP'
