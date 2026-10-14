#!/bin/sh
# Recorded Pioneer only: stage/boot known artifacts, never edit default boot.
set -eu
fail() { printf 'BLOCKED: %s\n' "$1" >&2; exit 1; }
[ "$#" -eq 2 ] || fail 'Usage: pioneer_linux_boot_haiku.sh prepare|boot|cancel SESSION'
action=$1
session=$2
case "$action" in prepare|boot|cancel) ;; *) fail 'Unknown action';; esac
case "$session" in ''|*[!A-Za-z0-9_-]*) fail 'Use a simple unique session name';; esac
[ "${#session}" -le 64 ] || fail 'Session name too long'
[ "$(id -u)" = 0 ] || fail 'Run as root on the recorded native Linux'
[ "$(uname -s):$(uname -m):$(uname -r)" = Linux:riscv64:6.1.31 ] || fail 'Wrong Linux platform'
base=/mnt/ssd/pioneer-haiku-kexec-20261013
run=$base/boot-$session
image=$base/enter-2/HaikuPioneerKexecProbe.Image
bundle=$base/samsung-bundle-2/samsung-ram.bfs
loader=/mnt/ssd/pioneer-kexec-probe-20261013/kexec-tools-0fd0d8e9ea975696fdc05ad267a65f3efd415612/build/sbin/kexec
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
uart_helper=$script_dir/pioneer_linux_uart_keepalive.sh
uart=/sys/devices/platform/soc/7040000000.serial
hash_is()
{
 [ -f "$1" ] && [ ! -L "$1" ] || fail 'Expected regular pinned input'
 [ "$(sha256sum "$1" | cut -d ' ' -f1)" = "$2" ] || fail "Hash mismatch: $1"
}
[ "$(findmnt -n -o SOURCE /)" = /dev/nvme0n1p3 ] || fail 'Unexpected root'
[ "$(findmnt -n -o SOURCE /boot)" = /dev/nvme0n1p2 ] || fail 'Unexpected boot volume'
[ "$(findmnt -n -o SOURCE /mnt/ssd)" = /dev/sdb2 ] || fail 'Unexpected build disk'
[ "$(cat /sys/devices/system/cpu/online)" = 0-63 ] || fail 'Expected all 64 Linux CPUs'
[ "$(cat /proc/sys/kernel/tainted)" = 0 ] || fail 'Unexpected tainted kernel'
for device in /sys/block/mmcblk*; do [ ! -e "$device" ] || fail 'Remove SD only while powered off'; done
[ "$(blockdev --getsize64 /dev/sda)" = 120034123776 ] || fail 'Wrong Samsung capacity'
[ "$(lsblk -dn -o SERIAL /dev/sda | tr -d ' ')" = S14CNSAD300075H ] || fail 'Wrong Samsung serial'
[ "$(blkid -s UUID -o value /dev/sda1)" = 020147de08752551 ] || fail 'Wrong BFS identity'
[ "$(cat /sys/class/block/sda1/start)" = 64 ] || fail 'Wrong partition offset'
[ -z "$(findmnt -rn -S /dev/sda1)" ] || fail 'Samsung must be unmounted'
[ ! -e /usr/lib/systemd/system-shutdown/pioneer-kexec-trace ] && [ ! -L /usr/lib/systemd/system-shutdown/pioneer-kexec-trace ] || fail 'Clean up diagnostic hook first'
[ ! -e /run/pioneer-kexec-trace ] && [ ! -L /run/pioneer-kexec-trace ] || fail 'Clean up diagnostic arm state first'
hash_is /boot/extlinux/extlinux.conf 53c0da6d26b93ac6e6a152c8146296325a9c25f54409ef7e6dcfc83e3b1fad9c
[ "$(stat -c '%u:%g:%a:%h:%s' /boot/extlinux/extlinux.conf)" = 0:0:664:1:878 ] || fail 'Unexpected default boot metadata'
hash_is /usr/sbin/kexec 64b403d2c1c0f0d63581b902477a9945ba187c8b1ab18ae733c69a48dc3b0950
hash_is "$loader" 06aa5c6fa6e78bdf92abefb4e7f4b0accadfe6212a02c700905392f8d8056748
root_manifest()
{
 disk=120034123776
 set -- 1 "$(printf '%x' "$((disk >> 32))")" "$(printf '%x' "$((disk & 4294967295))")" 0 8000
 for offset in 0 512 32768 33280 120034123264; do
  sum=$(dd if=/dev/sda bs=512 skip="$((offset / 512))" count=1 status=none |
   od -An -tu4 --endian=little -v | awk '{for(i=1;i<=NF;i++){sum+=$i;words++}} END {if(words!=128)exit 1;printf "%.0f",sum%4294967296}') || return 1
  set -- "$@" "$(printf '%x' "$((offset >> 32))")" "$(printf '%x' "$((offset & 4294967295))")" "$(printf '%x' "$sum")"
 done
 printf '%s\n' "$*"
}
validate_run()
{
 [ -d "$run" ] && [ ! -L "$run" ] || fail 'Unknown session'
 [ "$(stat -c '%u:%g:%a' "$run")" = 0:0:700 ] || fail 'Unexpected session owner/mode'
 [ -f "$run/prepared" ] && [ ! -L "$run/prepared" ] || fail 'Session not prepared'
 [ "$(stat -c '%u:%g:%a:%h:%s' "$run/prepared")" = 0:0:600:1:33 ] || fail 'Unexpected prepared marker'
 cmp "$run/token" "$run/prepared"
 sha256sum -c "$run/inputs.sha256"
 cmp /sys/firmware/fdt "$run/live-fdt.dtb"
 [ "$(fdtget -t x "$run/probe-fdt.dtb" /chosen haiku,pioneer-kexec-probe)" = 4 ] || fail 'Wrong loader mode'
 [ "$(fdtget -t x "$run/probe-fdt.dtb" /chosen haiku,pioneer-root-v1)" = "$(root_manifest)" ] || fail 'Root identity changed since prepare'
}
case "$action" in
 prepare)
  [ "$(cat /sys/kernel/kexec_loaded)" = 0 ] || fail 'Kexec slot occupied'
  [ ! -e "$run" ] && [ ! -L "$run" ] || fail 'Use a fresh session name'
  [ "$(cat "$uart/power/control")" = auto ] || fail 'UART policy already owned elsewhere'
  [ ! -e /run/pioneer-uart-keepalive ] && [ ! -L /run/pioneer-uart-keepalive ] || fail 'UART state already exists'
  hash_is "$image" 6188d850ba4fda0851031dd9213d2cd1e760276eddfa801a577d73ddc6850961
  hash_is "$bundle" 3d909f2d0de60528496d1ee040a918a7bf6f1f65d6ddc16d9904c3a022e335c1
  hash_is "$uart_helper" a9056dbc4317cc52bea715d9687ba223dee6957ca5e625d91cba8f6fb7071a22
  [ "$(grep 'System RAM' /proc/iomem | head -n1 | cut -d- -f1)" = 02200000 ] || fail 'Unexpected low RAM start'
  [ "$(awk '/MemAvailable:/ {print $2}' /proc/meminfo)" -gt 20971520 ] || fail 'Insufficient staging RAM'
  umask 077; mkdir -m 700 "$run" "$run/source-check"
  mounted=0; uart_owned=0; attempted_load=0
  cleanup()
  {
   result=$?; trap - EXIT
   if [ "$mounted" = 1 ]; then umount "$run/source-check" || exit 1; fi
   if [ "$result" != 0 ]; then
    if [ "$attempted_load" = 1 ] && [ "$(cat /sys/kernel/kexec_loaded)" = 1 ]; then "$loader" -c -u || exit 1; fi
    if [ "$uart_owned" = 1 ]; then sh "$uart_helper" restore || exit 1; fi
   fi
   exit "$result"
  }
  trap cleanup EXIT
  mount -t befs -o ro,nosuid,nodev,noexec /dev/sda1 "$run/source-check"; mounted=1
  hash_is "$run/source-check/home/config/non-packaged/add-ons/kernel/drivers/dev/net/rtl8125" fe6273c985de229e0c6d8fba9dcc27d7978cb9ab2b419670058ceb0198a13c00
  hash_is "$run/source-check/system/packages/haiku-r1~beta6_hrev99999-1-riscv64.hpkg" 6a40c785862ab19913a7cf0b9ef64c90fd5998cda2c22c6636edc8f3463b348a
  umount "$run/source-check"; mounted=0
  cp /sys/firmware/fdt "$run/live-fdt.dtb"
  cp "$run/live-fdt.dtb" "$run/probe-fdt.dtb"
  fdtput -t x "$run/probe-fdt.dtb" /chosen haiku,pioneer-kexec-probe 4
  # Only internally generated hexadecimal words undergo word splitting.
  manifest=$(root_manifest)
  set -- $manifest
  [ "$#" = 20 ] || fail 'Malformed root manifest'
  fdtput -t x "$run/probe-fdt.dtb" /chosen haiku,pioneer-root-v1 "$@"
  token=$(tr -d '-' < /proc/sys/kernel/random/uuid)
  printf '%s\n' "$token" > "$run/token"
  grep -Eq '^[0-9a-f]{32}$' "$run/token" || fail 'Invalid token'
  sha256sum "$image" "$bundle" "$loader" "$uart_helper" "$run/live-fdt.dtb" "$run/probe-fdt.dtb" > "$run/inputs.sha256"
  uart_owned=1
  sh "$uart_helper" enable
  attempted_load=1
  "$loader" -c -l -t image-riscv --mem-min=0x2200000 --mem-max=0x1fffffffff \
   --dtb="$run/probe-fdt.dtb" --initrd="$bundle" "$image"
  [ "$(cat /sys/kernel/kexec_loaded)" = 1 ] || fail 'Image not loaded'
  cp "$run/token" "$run/prepared"
  printf 'PREPARED %s; Linux is still running. Save work, then run: %s boot %s\n' "$session" "$0" "$session"
  ;;
 boot)
  validate_run
  [ "$(cat /sys/kernel/kexec_loaded)" = 1 ] || fail 'Kexec slot empty'
  [ "$(cat "$uart/power/control")" = on ] && [ "$(cat "$uart/power/runtime_status")" = active ] || fail 'UART not kept active'
  printf 'Switching to Haiku via normal Linux shutdown; default NVMe boot unchanged\n'
  systemctl kexec
  ;;
 cancel)
  validate_run
  if [ "$(cat /sys/kernel/kexec_loaded)" = 1 ]; then "$loader" -c -u; fi
  [ "$(cat /sys/kernel/kexec_loaded)" = 0 ] || fail 'Image did not unload'
  sh "$uart_helper" restore
  mv "$run/prepared" "$run/cancelled"
  printf 'CANCELLED %s; image unloaded and UART policy restored\n' "$session"
  ;;
esac
