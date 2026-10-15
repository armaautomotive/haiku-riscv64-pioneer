#!/bin/sh
# Private opt-in SMP/P100A profile. The known-good single-core helper is unchanged.
set -eu
fail() { printf 'BLOCKED: %s\n' "$1" >&2; exit 1; }
[ "$#" -eq 3 ] || [ "$#" -eq 4 ] || fail 'Usage: pioneer_linux_boot_haiku_smp.sh prepare|boot|cancel SESSION CPU_COUNT [current|validated-control|capped-control|active-map-control|memory8-control|memory128-control|memory8-map-order-control|memory128-map-order-control|memory8-journal-recovery-control|memory128-kdl-control|memory128-kdl-journal-control|memory128-kdl-nosd-control]'
action=$1
session=$2
cpu_count=$3
profile=${4:-active-map-control}
case "$profile" in current|validated-control|capped-control|active-map-control|memory8-control|memory128-control|memory8-map-order-control|memory128-map-order-control|memory8-journal-recovery-control|memory128-kdl-control|memory128-kdl-journal-control|memory128-kdl-nosd-control) ;; *) fail 'Unknown pinned kernel profile';; esac
case "$cpu_count" in 2|4|8|16|32|64) ;; *) fail 'CPU count must be 2, 4, 8, 16, 32 or 64';; esac
if [ "$profile" = memory8-journal-recovery-control ]; then
 [ "$cpu_count" = 2 ] || fail 'Journal recovery requires the validated two-CPU/8 GiB control'
fi
case "$profile" in memory128-kdl-control|memory128-kdl-journal-control|memory128-kdl-nosd-control)
 [ "$cpu_count" = 64 ] || fail 'Private KDL diagnostic requires 64 CPUs'
 ;;
esac
case "$action" in prepare|boot|cancel) ;; *) fail 'Unknown action';; esac
case "$session" in ''|*[!A-Za-z0-9_-]*) fail 'Use a simple unique session name';; esac
[ "${#session}" -le 64 ] || fail 'Session name too long'
[ "$(id -u)" = 0 ] || fail 'Run as root on the recorded native Linux'
[ "$(uname -s):$(uname -m):$(uname -r)" = Linux:riscv64:6.1.31 ] || fail 'Wrong Linux platform'
base=/mnt/ssd/pioneer-haiku-kexec-20261013
run=$base/boot-$session
image=$base/smp-4/HaikuPioneerKexecProbe.Image
image_hash=7f9788287e1851789ff41b098c910d613df51c4044256278f104b9b806082fcb
memory_gib=3
bundle=$base/smp-kernel-bundle-1/samsung-ram.bfs
# Default has recorded all-core and VM workload passes, not full P100A/runtime
# certification. "current" is the frozen historical uncapped experiment.
case "$profile" in
 current) bundle_hash=a3e1015fb56a440d5ea75a538b590c59cebd1ce99183b176eaef2ac703010c28;;
 validated-control)
  bundle=$base/smp-kernel-bundle-validated-1/samsung-ram.bfs
  bundle_hash=5f2f13fd45946aaae8038f20facad6638574468801c49497085443558a13151a
  ;;
 capped-control)
  bundle=$base/smp-kernel-bundle-capped-1/samsung-ram.bfs
  bundle_hash=1b247841bdf4a5d5ecd7239103dee0c359f24c7fa3224f8bae7eb8f72115a626
  ;;
 active-map-control)
  bundle=$base/smp-kernel-bundle-active-map-1/samsung-ram.bfs
  bundle_hash=8a0110715013a322ace77f896a4ce00857c3b18ca4d7106edeac46a6fdfe5ced
  ;;
 memory8-control|memory128-control|memory8-journal-recovery-control)
  # Explicit experimental memory profile; requires its matching kernel's
  # permanent virtual-map reservation. Never pair it with an older bundle.
  image=$base/memory-2/HaikuPioneerKexecProbe.Image
  image_hash=b5473e6d00a9bcf469fce648b4415f28eda2dbc2510bfc9ed8d61af9034e6e98
  bundle=$base/smp-kernel-bundle-memory-1/samsung-ram.bfs
  bundle_hash=98eebc001c7c869566a81ec445ee71798ec98be05d7585e18d416f8029c759cd
  case "$profile" in memory8-control|memory8-journal-recovery-control) memory_gib=8;; memory128-control) memory_gib=128;; esac
  ;;
 memory8-map-order-control|memory128-map-order-control)
  image=$base/memory-2/HaikuPioneerKexecProbe.Image
  image_hash=b5473e6d00a9bcf469fce648b4415f28eda2dbc2510bfc9ed8d61af9034e6e98
  bundle=$base/smp-kernel-bundle-memory-map-order-1/samsung-ram.bfs
  bundle_hash=98783b7aa58b283c9ce33623deecd828eaf5a8f20394cfa92a2fe1065c098ebf
  case "$profile" in memory8-map-order-control) memory_gib=8;; memory128-map-order-control) memory_gib=128;; esac
  ;;
 memory128-kdl-control|memory128-kdl-journal-control|memory128-kdl-nosd-control)
  # Deliberately enters the debugger from AP1 after 120 seconds, even if
  # boot succeeds. This is a separate diagnostic, never the default kernel.
  image=$base/memory-2/HaikuPioneerKexecProbe.Image
  image_hash=b5473e6d00a9bcf469fce648b4415f28eda2dbc2510bfc9ed8d61af9034e6e98
  bundle=$base/smp-kernel-bundle-memory-kdl-timer-1/samsung-ram.bfs
  bundle_hash=abbd25643a1787f3a857535f4becee8f8b7fe7578404fb1dae52b425afae1e68
  memory_gib=128
  ;;
 *) fail 'Unknown pinned kernel profile';;
esac
loader=/mnt/ssd/pioneer-kexec-probe-20261013/kexec-tools-0fd0d8e9ea975696fdc05ad267a65f3efd415612/build/sbin/kexec
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
uart_helper=$script_dir/pioneer_linux_uart_keepalive.sh
journal_verifier=$base/pioneer_verify_samsung_journal_view-2.sh
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
device=/sys/bus/pci/devices/0002:81:00.0
[ "$(cat "$device/vendor")" = 0x1e52 ] && [ "$(cat "$device/device")" = 0xb140 ] || fail 'Wrong accelerator'
[ "$(cat "$device/subsystem_vendor")" = 0x1e52 ] && [ "$(cat "$device/subsystem_device")" = 0x0043 ] || fail 'Wrong board SKU'
[ ! -e "$device/driver" ] && [ "$(cat "$device/enable")" = 0 ] || fail 'Accelerator driver active'
[ "$(setpci -s 0002:81:00.0 COMMAND.w)" = 0000 ] || fail 'Endpoint decoding or DMA enabled'
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
remove_private_sd_node()
{
 # PIONEER_PRIVATE_NOSD_BEGIN
 # Only the explicit diagnostic calls this, after the global no-MMC guard.
 # Haiku's current FDT enumerator does not honor status="disabled", so remove
 # this one identity-checked node from the private copy, not the live FDT.
 target=$run/probe-fdt.dtb
 [ -f "$target" ] && [ ! -L "$target" ] || fail 'Wrong private FDT'
 [ ! "$target" -ef "$run/live-fdt.dtb" ] || fail 'Private FDT aliases source'
 sd=/soc/bm-sd@704002B000
 [ "$(fdtget -t s "$target" "$sd" compatible)" = bitmain,bm-sd ] || fail 'Wrong SD compatible'
 [ "$(fdtget -t x "$target" "$sd" reg)" = '70 4002b000 0 1000' ] || fail 'Wrong SD registers'
 [ "$(fdtget -t x "$target" "$sd" interrupts)" = '88 4' ] || fail 'Wrong SD interrupt'
 fdtput -r "$target" "$sd"
 if fdtget "$target" "$sd" compatible >/dev/null 2>&1; then fail 'Private SD node remains'; fi
 [ "$(fdtget -t x "$target" /soc/bm-emmc@704002A000 reg)" = '70 4002a000 0 1000' ] || fail 'eMMC node changed'
 # PIONEER_PRIVATE_NOSD_END
}
validate_run()
{
 [ -d "$run" ] && [ ! -L "$run" ] || fail 'Unknown session'
 [ "$(stat -c '%u:%g:%a' "$run")" = 0:0:700 ] || fail 'Unexpected session owner/mode'
 [ -f "$run/prepared" ] && [ ! -L "$run/prepared" ] || fail 'Session not prepared'
 [ "$(stat -c '%u:%g:%a:%h:%s' "$run/prepared")" = 0:0:600:1:33 ] || fail 'Unexpected prepared marker'
 cmp "$run/token" "$run/prepared"
 [ "$(cat "$run/cpus")" = "$cpu_count" ] || fail 'CPU count differs from prepared session'
 [ "$(cat "$run/profile")" = "$profile" ] || fail 'Kernel profile differs from prepared session'
 sha256sum -c "$run/inputs.sha256"
 cmp /sys/firmware/fdt "$run/live-fdt.dtb"
 [ "$(fdtget -t x "$run/probe-fdt.dtb" /chosen haiku,pioneer-kexec-probe)" = 4 ] || fail 'Wrong loader mode'
 [ "$(fdtget -t x "$run/probe-fdt.dtb" /chosen haiku,pioneer-cpu-count)" = "$(printf '%x' "$cpu_count")" ] || fail 'Wrong SMP opt-in'
 if [ "$memory_gib" = 3 ]; then
  if fdtget "$run/probe-fdt.dtb" /chosen haiku,pioneer-memory-gib > /dev/null 2>&1; then fail 'Unexpected memory opt-in'; fi
 else
  [ "$(fdtget -t x "$run/probe-fdt.dtb" /chosen haiku,pioneer-memory-gib)" = "$(printf '%x' "$memory_gib")" ] || fail 'Wrong memory opt-in'
 fi
 [ "$(fdtget -t x "$run/probe-fdt.dtb" /pcie@7062000000 haiku,pioneer-root-probe)" = 1e52b140 ] || fail 'Missing private root opt-in'
 [ "$(fdtget -t x "$run/probe-fdt.dtb" /chosen haiku,pioneer-root-v1)" = "$(root_manifest)" ] || fail 'Root identity changed since prepare'
 if [ "$profile" = memory128-kdl-nosd-control ]; then
  if fdtget "$run/probe-fdt.dtb" /soc/bm-sd@704002B000 compatible >/dev/null 2>&1; then fail 'Unexpected SD node in no-SD diagnostic'; fi
  [ "$(fdtget -t x "$run/probe-fdt.dtb" /soc/bm-emmc@704002A000 reg)" = '70 4002a000 0 1000' ] || fail 'eMMC node changed'
 fi
}
case "$action" in
 prepare)
  [ "$(cat /sys/kernel/kexec_loaded)" = 0 ] || fail 'Kexec slot occupied'
  [ ! -e "$run" ] && [ ! -L "$run" ] || fail 'Use a fresh session name'
  [ "$(cat "$uart/power/control")" = auto ] || fail 'UART policy already owned elsewhere'
  [ ! -e /run/pioneer-uart-keepalive ] && [ ! -L /run/pioneer-uart-keepalive ] || fail 'UART state already exists'
  hash_is "$image" "$image_hash"
  hash_is "$bundle" "$bundle_hash"
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
  if [ "$profile" = memory8-journal-recovery-control ] || [ "$profile" = memory128-kdl-journal-control ]; then
   # Linux befs refuses a pending journal. Explicit journal-view profiles
   # verifies the same two hashes through a fresh private replay overlay,
   # keeping the physical Samsung disk block-read-only throughout that check.
   # Only a subsequent ordinary Haiku root mount replays the real journal.
   hash_is "$journal_verifier" 8ff6ad3643e4baab6734cbb7799c1ba02bb8beb197cf43f3540a66c2b02c7ff9
   sh "$journal_verifier" "$session"
  else
   mount -t befs -o ro,nosuid,nodev,noexec /dev/sda1 "$run/source-check"; mounted=1
   hash_is "$run/source-check/home/config/non-packaged/add-ons/kernel/drivers/dev/net/rtl8125" fe6273c985de229e0c6d8fba9dcc27d7978cb9ab2b419670058ceb0198a13c00
   hash_is "$run/source-check/system/packages/haiku-r1~beta6_hrev99999-1-riscv64.hpkg" 6a40c785862ab19913a7cf0b9ef64c90fd5998cda2c22c6636edc8f3463b348a
   umount "$run/source-check"; mounted=0
  fi
  cp /sys/firmware/fdt "$run/live-fdt.dtb"
  cp "$run/live-fdt.dtb" "$run/probe-fdt.dtb"
  if [ "$profile" = memory128-kdl-nosd-control ]; then remove_private_sd_node; fi
  fdtput -t x "$run/probe-fdt.dtb" /chosen haiku,pioneer-kexec-probe 4
  fdtput -t x "$run/probe-fdt.dtb" /chosen haiku,pioneer-cpu-count "$(printf '%x' "$cpu_count")"
  if [ "$memory_gib" = 3 ]; then
   if fdtget "$run/probe-fdt.dtb" /chosen haiku,pioneer-memory-gib > /dev/null 2>&1; then fail 'Unexpected inherited memory opt-in'; fi
  else
   fdtput -t x "$run/probe-fdt.dtb" /chosen haiku,pioneer-memory-gib "$(printf '%x' "$memory_gib")"
  fi
  printf '%s\n' "$cpu_count" > "$run/cpus"
  printf '%s\n' "$profile" > "$run/profile"
  [ "$(fdtget -t x "$run/live-fdt.dtb" /pcie@7062000000 bus-range)" = '80 bf' ] || fail 'Wrong slot bus range'
  [ "$(fdtget -t x "$run/live-fdt.dtb" /pcie@7062000000 reg)" = '70 62000000 0 2000000 48 0 0 1000' ] || fail 'Wrong slot register layout'
  fdtput -t x "$run/probe-fdt.dtb" /pcie@7062000000 haiku,pioneer-root-probe 1e52b140
  # Only internally generated hexadecimal words undergo word splitting.
  manifest=$(root_manifest)
  set -- $manifest
  [ "$#" = 20 ] || fail 'Malformed root manifest'
  fdtput -t x "$run/probe-fdt.dtb" /chosen haiku,pioneer-root-v1 "$@"
  token=$(tr -d '-' < /proc/sys/kernel/random/uuid)
  printf '%s\n' "$token" > "$run/token"
  grep -Eq '^[0-9a-f]{32}$' "$run/token" || fail 'Invalid token'
  sha256sum "$image" "$bundle" "$loader" "$uart_helper" "$run/live-fdt.dtb" "$run/probe-fdt.dtb" "$run/cpus" "$run/profile" > "$run/inputs.sha256"
  if [ "$profile" = memory8-journal-recovery-control ] || [ "$profile" = memory128-kdl-journal-control ]; then
   sha256sum "$journal_verifier" >> "$run/inputs.sha256"
  fi
  uart_owned=1
  sh "$uart_helper" enable
  attempted_load=1
  "$loader" -c -l -t image-riscv --mem-min=0x2200000 --mem-max=0x1fffffffff \
   --dtb="$run/probe-fdt.dtb" --initrd="$bundle" "$image"
  [ "$(cat /sys/kernel/kexec_loaded)" = 1 ] || fail 'Image not loaded'
  cp "$run/token" "$run/prepared"
  printf 'PREPARED %s with %s CPUs (%s); Linux is still running. Run: %s boot %s %s %s\n' "$session" "$cpu_count" "$profile" "$0" "$session" "$cpu_count" "$profile"
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
