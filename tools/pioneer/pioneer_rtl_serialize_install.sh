#!/bin/sh
# ONE pinned offline NIC comparison. Run on Linux, NEVER with Haiku active.
# Rehearse on a private COW view before applying. Exact rollback is supported.
set -eu
fail() { echo "STOP: $*" >&2; exit 1; }
case "${1-}" in --rehearse) mode=rehearsal;; --apply) mode=installed;; --rollback) mode=rollback;; *) exit 2;; esac
[ "$#" = 1 ] || exit 2
base=/mnt/ssd/pioneer-rtl-serialize-20261017-a
run=$base/$mode
tool=/mnt/ssd/pioneer-xhci-rollback-20261012/host/objects/linux/riscv64/release/tools/bfs_shell/bfs_shell
relative=home/config/non-packaged/add-ons/kernel/drivers/dev/net/rtl8125
saved=home/pioneer-rtl-serialize-20261017-a
package=system/packages/haiku-r1~beta6_hrev99999-1-riscv64.hpkg
old=fe6273c985de229e0c6d8fba9dcc27d7978cb9ab2b419670058ceb0198a13c00
new=0b83ceed0071f3dab3f9b175355bd83fe7364176676264ae66d751ddba2b7c9a
os=6a40c785862ab19913a7cf0b9ef64c90fd5998cda2c22c6636edc8f3463b348a
exact() { [ -f "$1" ] && [ ! -L "$1" ] && [ "$(sha256sum "$1" | cut -d ' ' -f1)" = "$2" ]; }
[ "$(id -u)" = 0 ] && [ "$(uname -s):$(uname -m):$(uname -r)" = Linux:riscv64:6.1.31 ] || fail platform
[ "$(findmnt -n -o SOURCE /)" = /dev/nvme0n1p3 ] || fail root
[ "$(findmnt -n -o SOURCE /mnt/ssd)" = /dev/sdb2 ] || fail build_disk
[ "$(cat /sys/kernel/kexec_loaded)" = 0 ] || fail loaded_kexec
[ "$(blockdev --getsize64 /dev/sda)" = 120034123776 ] || fail disk_size
[ "$(blockdev --getsize64 /dev/sda1)" = 120033640448 ] || fail partition_size
[ "$(lsblk -dn -o SERIAL /dev/sda | tr -d ' ')" = S14CNSAD300075H ] || fail serial
[ "$(blkid -s UUID -o value /dev/sda1)" = 020147de08752551 ] || fail bfs_uuid
[ "$(cat /sys/class/block/sda1/start)" = 64 ] || fail partition_offset
[ -z "$(findmnt -rn -S /dev/sda1)" ] || fail mounted_samsung
[ "$(blockdev --getro /dev/sda):$(blockdev --getro /dev/sda1)" = 0:0 ] || fail existing_readonly_policy
[ "$(setpci -s 0002:81:00.0 COMMAND.w)" = 0000 ] || fail active_accelerator
exact /boot/extlinux/extlinux.conf 53c0da6d26b93ac6e6a152c8146296325a9c25f54409ef7e6dcfc83e3b1fad9c || fail default_boot
exact "$tool" ed09122a70c324e83b38b708289ad058ae764004b0f343ebf27d4797a28f6eec || fail bfs_tool
[ -d "$base" ] && [ ! -L "$base" ] && [ "$(stat -c '%u:%g:%a' "$base")" = 0:0:700 ] || fail stage_owner
exact "$base/rtl8125-serialized" "$new" || fail candidate_hash
if [ "$mode" = installed ]; then
 [ -f "$base/rehearsal/VERIFIED" ] && [ ! -L "$base/rehearsal/VERIFIED" ] || fail rehearse_first
 [ "$(stat -c '%u:%g:%a:%h:%s' "$base/rehearsal/VERIFIED")" = 0:0:600:1:65 ] || fail rehearsal_marker
 [ "$(cat "$base/rehearsal/VERIFIED")" = "$new" ] || fail rehearsal_candidate
fi
[ ! -e "$run" ] && [ ! -L "$run" ] || fail fresh_run_required
# Do not silently replay/repair a dirty source in this installation helper.
[ "$(dd if=/dev/sda1 bs=1 skip=596 count=4 status=none | od -An -tx4 --endian=little | tr -d ' \n')" = 434c454e ] || fail source_journal_not_clean
source_superblock=$(dd if=/dev/sda1 bs=1024 count=1 status=none | sha256sum | cut -d ' ' -f1)
umask 077
mkdir -m 700 "$run" "$run/view"
mounted=0; connected=0; protected=0; loaded=0
cleanup()
{
 result=$?; trap - EXIT
 if [ "$mounted" = 1 ]; then umount "$run/view" || exit 1; fi
 if [ "$connected" = 1 ]; then
  qemu-nbd --disconnect /dev/nbd0 || exit 1
  for attempt in 1 2 3 4 5; do [ -e /sys/block/nbd0/pid ] || break; sleep 1; done
  [ ! -e /sys/block/nbd0/pid ] || exit 1
 fi
 if [ "$loaded" = 1 ]; then modprobe -r nbd || exit 1; fi
 if [ "$protected" = 1 ]; then
  blockdev --setrw /dev/sda; blockdev --setrw /dev/sda1
 fi
 exit "$result"
}
trap cleanup EXIT
trap 'exit 1' HUP INT TERM
mount -t befs -o ro,nosuid,nodev,noexec /dev/sda1 "$run/view"; mounted=1
for path in home home/config home/config/non-packaged home/config/non-packaged/add-ons home/config/non-packaged/add-ons/kernel home/config/non-packaged/add-ons/kernel/drivers home/config/non-packaged/add-ons/kernel/drivers/dev home/config/non-packaged/add-ons/kernel/drivers/dev/net; do
 [ -d "$run/view/$path" ] && [ ! -L "$run/view/$path" ] || fail parent_path
done
exact "$run/view/$package" "$os" || fail os_package
expected=$old; [ "$mode" != rollback ] || expected=$new
exact "$run/view/$relative" "$expected" || fail current_driver
if [ "$mode" = rollback ]; then
 exact "$run/view/$saved/qualified-msi-fe627-backup" "$old" || fail saved_control
 [ ! -e "$run/view/$saved/rtl8125.rollback" ] && [ ! -L "$run/view/$saved/rtl8125.rollback" ] || fail rollback_temporary_exists
 cp "$run/view/$saved/qualified-msi-fe627-backup" "$run/control"
else
 [ ! -e "$run/view/$saved" ] && [ ! -L "$run/view/$saved" ] || fail backup_directory_exists
 cp "$run/view/$relative" "$run/control"
fi
exact "$run/control" "$old" || fail extracted_control
umount "$run/view"; mounted=0
target=/dev/sda1
if [ "$mode" = rehearsal ]; then
 if [ ! -d /sys/module/nbd ]; then modprobe nbd max_part=0; loaded=1; fi
 [ "$(cat /sys/block/nbd0/size)" = 0 ] && [ ! -e /sys/block/nbd0/pid ] || fail nbd_already_owned
 [ -z "$(findmnt -rn -S /dev/nbd0)" ] || fail mounted_nbd
 protected=1; blockdev --setro /dev/sda; blockdev --setro /dev/sda1
 [ "$(blockdev --getro /dev/sda):$(blockdev --getro /dev/sda1)" = 1:1 ] || fail readonly_protection
 qemu-img create -f qcow2 -F raw -b /dev/sda1 "$run/changes.qcow2"
 qemu-nbd --fork --connect=/dev/nbd0 --format=qcow2 --cache=none --socket="$run/nbd.sock" --pid-file="$run/nbd.pid" "$run/changes.qcow2"
 connected=1; target=/dev/nbd0
 for attempt in 1 2 3 4 5; do [ "$(blockdev --getsize64 /dev/nbd0)" = 120033640448 ] && break; sleep 1; done
 [ "$(blockdev --getsize64 /dev/nbd0)" = 120033640448 ] || fail overlay_size
fi
if [ "$mode" = rollback ]; then
 printf '%s\n' "cp :$run/control /myfs/$saved/rtl8125.rollback" "chmod 755 /myfs/$saved/rtl8125.rollback" sync "mv /myfs/$saved/rtl8125.rollback /myfs/$relative" sync 'checkfs -c' sync > "$run/commands"
 replies=7; result_hash=$old
else
 printf '%s\n' "mkdir /myfs/$saved" "cp :$run/control /myfs/$saved/qualified-msi-fe627-backup" "cp :$base/rtl8125-serialized /myfs/$saved/rtl8125.incoming" "chmod 755 /myfs/$saved/rtl8125.incoming" sync "mv /myfs/$saved/rtl8125.incoming /myfs/$relative" sync 'checkfs -c' sync > "$run/commands"
 replies=9; result_hash=$new
fi
"$tool" -n "$target" 3< "$run/commands" 4> "$run/replies" > "$run/write.log" 2>&1
[ "$(wc -l < "$run/replies")" -eq "$replies" ] && [ "$(sort -u "$run/replies")" = 0 ] || fail fs_commands
mount -t befs -o ro,nosuid,nodev,noexec "$target" "$run/view"; mounted=1
exact "$run/view/$relative" "$result_hash" || fail installed_readback
exact "$run/view/$saved/qualified-msi-fe627-backup" "$old" || fail backup_readback
exact "$run/view/$package" "$os" || fail package_changed
[ "$(stat -c '%a' "$run/view/$relative")" = 755 ] || fail driver_mode
umount "$run/view"; mounted=0
sync
if [ "$mode" = rehearsal ]; then
 [ "$(blockdev --getro /dev/sda):$(blockdev --getro /dev/sda1)" = 1:1 ] || fail readonly_protection_lost
 [ "$(dd if=/dev/sda1 bs=1024 count=1 status=none | sha256sum | cut -d ' ' -f1)" = "$source_superblock" ] || fail original_superblock_changed
fi
# The marker's CONTENT is the candidate SHA,not an empty success flag.
printf '%s\n' "$new" > "$run/VERIFIED"
tail -n 18 "$run/write.log"
echo "PASS_OFFLINE_RTL_$mode;P100A_OS_PACKAGE_DEFAULT_BOOT_UNCHANGED;RUNTIME_TEST_STILL_REQUIRED"
