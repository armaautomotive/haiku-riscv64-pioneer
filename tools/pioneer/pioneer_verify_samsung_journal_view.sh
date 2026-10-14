#!/bin/sh
# Verify two boot inputs through a private journal overlay. Never repair/write
# Samsung from Linux: its whole disk and partition stay block-read-only while
# the Haiku BFS implementation replays only into the fresh qcow2 file.
set -eu
fail() { printf 'BLOCKED: %s\n' "$1" >&2; exit 1; }
[ "$#" -eq 1 ] || fail 'Usage: pioneer_verify_samsung_journal_view.sh UNIQUE_SESSION'
session=$1
case "$session" in ''|*[!A-Za-z0-9_-]*) fail 'Invalid session';; esac
[ "${#session}" -le 64 ] || fail 'Session too long'
[ "$(id -u)" = 0 ] || fail 'Run as root'
[ "$(uname -s):$(uname -m):$(uname -r)" = Linux:riscv64:6.1.31 ] || fail 'Wrong platform'
[ "$(findmnt -n -o SOURCE /mnt/ssd)" = /dev/sdb2 ] || fail 'Wrong build disk'
[ "$(blockdev --getsize64 /dev/sda)" = 120034123776 ] || fail 'Wrong disk size'
[ "$(blockdev --getsize64 /dev/sda1)" = 120033640448 ] || fail 'Wrong partition size'
[ "$(lsblk -dn -o SERIAL /dev/sda | tr -d ' ')" = S14CNSAD300075H ] || fail 'Wrong serial'
[ "$(blkid -s UUID -o value /dev/sda1)" = 020147de08752551 ] || fail 'Wrong BFS UUID'
[ "$(cat /sys/class/block/sda1/start)" = 64 ] || fail 'Wrong partition start'
[ -z "$(findmnt -rn -S /dev/sda1)" ] || fail 'Samsung mounted'
[ "$(blockdev --getro /dev/sda):$(blockdev --getro /dev/sda1)" = 0:0 ] || fail 'Unexpected existing read-only policy'
[ "$(cat /sys/kernel/kexec_loaded)" = 0 ] || fail 'Kexec slot occupied'
read_word() { dd if=/dev/sda1 bs=1 skip="$1" count=4 status=none | od -An -tx4 --endian=little | tr -d ' \n'; }
[ "$(read_word 544)" = 42465331 ] || fail 'Wrong BFS magic'
[ "$(read_word 596)" = 44495254 ] || fail 'Expected an explicitly dirty BFS journal'
base=/mnt/ssd/pioneer-haiku-kexec-20261013
run=$base/source-verify-$session
bfs=/mnt/ssd/pioneer-xhci-rollback-20261012/host/objects/linux/riscv64/release/tools/bfs_shell/bfs_shell
[ -f "$bfs" ] && [ ! -L "$bfs" ] || fail 'Missing regular BFS tool'
[ "$(sha256sum "$bfs" | cut -d ' ' -f1)" = ed09122a70c324e83b38b708289ad058ae764004b0f343ebf27d4797a28f6eec ] || fail 'BFS tool hash differs'
[ ! -e "$run" ] && [ ! -L "$run" ] || fail 'Use a fresh session'
umask 077
mkdir -m 700 "$run"
loaded=0; connected=0; protected=0
cleanup()
{
 result=$?; trap - EXIT
 if [ "$connected" = 1 ]; then
  qemu-nbd --disconnect /dev/nbd0 || exit 1
  for attempt in 1 2 3 4 5; do
   [ -e /sys/block/nbd0/pid ] || break
   sleep 1
  done
  [ ! -e /sys/block/nbd0/pid ] || { printf 'STOP: NBD still attached; Samsung remains read-only\n' >&2; exit 1; }
 fi
 if [ "$loaded" = 1 ]; then modprobe -r nbd || exit 1; fi
 if [ "$protected" = 1 ]; then
  blockdev --setrw /dev/sda
  blockdev --setrw /dev/sda1
  [ "$(blockdev --getro /dev/sda):$(blockdev --getro /dev/sda1)" = 0:0 ] || exit 1
 fi
 exit "$result"
}
trap cleanup EXIT
trap 'exit 1' HUP INT TERM
before=$(dd if=/dev/sda1 bs=1024 count=1 status=none | sha256sum | cut -d ' ' -f1)
if [ ! -d /sys/module/nbd ]; then modprobe nbd max_part=0; loaded=1; fi
[ ! -e /sys/block/nbd0/pid ] || fail 'NBD0 already owned'
[ -z "$(findmnt -rn -S /dev/nbd0)" ] || fail 'NBD0 mounted'
protected=1
blockdev --setro /dev/sda
blockdev --setro /dev/sda1
[ "$(blockdev --getro /dev/sda):$(blockdev --getro /dev/sda1)" = 1:1 ] || fail 'Read-only protection failed'
qemu-img create -f qcow2 -F raw -b /dev/sda1 "$run/journal.qcow2"
# A fresh regular qcow2 file is the only writable backing object. No commit,
# rebase, physical BFS mount, or checkfs repair command is used.
[ -f "$run/journal.qcow2" ] && [ ! -L "$run/journal.qcow2" ] || fail 'Bad overlay'
connected=1
qemu-nbd --fork --pid-file="$run/nbd.pid" --socket="$run/nbd.sock" \
 -f qcow2 --connect=/dev/nbd0 "$run/journal.qcow2"
for attempt in 1 2 3 4 5 6 7 8 9 10; do
 [ "$(blockdev --getsize64 /dev/nbd0)" = 120033640448 ] && break
 sleep 1
done
[ "$(blockdev --getsize64 /dev/nbd0)" = 120033640448 ] || fail 'Bad overlay size'
printf '%s\n' \
 "cp /myfs/home/config/non-packaged/add-ons/kernel/drivers/dev/net/rtl8125 :$run/rtl8125" \
 "cp /myfs/system/packages/haiku-r1~beta6_hrev99999-1-riscv64.hpkg :$run/haiku.hpkg" \
 sync > "$run/commands"
"$bfs" -n /dev/nbd0 3< "$run/commands" 4> "$run/replies" > "$run/replay.log" 2>&1
[ "$(wc -l < "$run/replies")" -eq 3 ] && [ "$(sort -u "$run/replies")" = 0 ] || fail 'Overlay extraction failed'
[ "$(sha256sum "$run/rtl8125" | cut -d ' ' -f1)" = fe6273c985de229e0c6d8fba9dcc27d7978cb9ab2b419670058ceb0198a13c00 ] || fail 'RTL override differs'
[ "$(sha256sum "$run/haiku.hpkg" | cut -d ' ' -f1)" = 6a40c785862ab19913a7cf0b9ef64c90fd5998cda2c22c6636edc8f3463b348a ] || fail 'OS package differs'
[ "$(blockdev --getro /dev/sda):$(blockdev --getro /dev/sda1)" = 1:1 ] || fail 'Lost read-only protection'
[ "$(dd if=/dev/sda1 bs=1024 count=1 status=none | sha256sum | cut -d ' ' -f1)" = "$before" ] || fail 'Original superblock changed'
printf 'PASS: journal replayed only in private overlay; OS and RTL hashes match; original Samsung unchanged\n'
