#!/usr/bin/env bash
# Verify the already completed, pinned backup through Linux O_DIRECT.
# This never writes the source, backup, manifest, or device. Run as root on
# the same boot as the snapshot service; systemd journal is the result log.
set -euo pipefail
export PATH=/usr/sbin:/usr/bin:/sbin:/bin

unit=pioneer-haiku-snapshot-20261107-a.service
directory=/mnt/ssd/pioneer-haiku-raw-backup-20261107-a
file=$directory/haiku-samsung-disk.raw
manifest=$file.sha256
disk_link=/dev/disk/by-id/ata-R3SL480G_E201610050040028
partition_link=${disk_link}-part2
expected_size=120034123776

[[ $# == 0 && $(id -u) == 0 && $(uname -s) == Linux ]] || exit 2

# A live job is not a successful backup. Never restart or replace it.
while :; do
    state=$(systemctl show "$unit" -p ActiveState -p SubState -p Result)
    case "$state" in
        *ActiveState=active*SubState=running*) sleep 30 ;;
        *Result=success*ActiveState=active*SubState=exited*) break ;;
        *) printf 'REFUSED: snapshot service not running or successfully exited: %s\n' "$state" >&2; exit 1 ;;
    esac
done

check_destination() {
    local disk partition mount
    disk=$(readlink -e "$disk_link")
    partition=$(readlink -e "$partition_link")
    [[ $disk =~ ^/dev/sd[a-z]$ && $partition == "${disk}2" ]] || return 1
    [[ $(lsblk -dn -o SERIAL "$disk") == E201610050040028 ]] || return 1
    [[ $(blkid -s UUID -o value "$partition") == d7acf635-9bfa-43b5-abf9-a35bf5abfd82 ]] || return 1
    mount=$(findmnt -rn -T /mnt/ssd -o SOURCE,FSTYPE)
    [[ $mount == "$partition ext4" && ! -L /mnt/ssd && ! -L $directory ]] || return 1
    [[ $(stat -c '%u:%a' "$directory") == 0:700 ]] || return 1
    [[ -f $file && ! -L $file && -f $manifest && ! -L $manifest ]] || return 1
    [[ $(stat -c '%u:%h:%a:%s' "$file") == "0:1:600:$expected_size" ]] || return 1
    [[ $(stat -c '%u:%h:%a' "$manifest") == 0:1:600 ]] || return 1
}
check_destination || { echo 'REFUSED: backup destination identity' >&2; exit 1; }
line=$(<"$manifest")
[[ $line =~ ^([0-9a-f]{64})\ \ haiku-samsung-disk\.raw$ ]] || { echo 'REFUSED: invalid manifest' >&2; exit 1; }
expected=${BASH_REMATCH[1]}
record="SNAPSHOT_VERIFIED $directory bytes=$expected_size sha256=$expected"
journalctl -u "$unit" --no-pager -o cat | grep -Fx "$record" >/dev/null || {
    echo 'REFUSED: no matching successful snapshot record' >&2; exit 1;
}
before=$(stat -c '%d:%i:%s:%y:%z' "$file")
echo 'DIRECT_VERIFY_BEGIN: uncached backup read; no backup or source writes'
# pipefail makes a dd error fatal; file size is a 4096-byte multiple, so
# all O_DIRECT requests, including the last one, are sector aligned.
actual=$(dd if="$file" iflag=direct,noatime bs=8M status=none | sha256sum)
actual=${actual%% *}
check_destination || { echo 'REFUSED: destination changed during read' >&2; exit 1; }
[[ $(stat -c '%d:%i:%s:%y:%z' "$file") == "$before" && $(<"$manifest") == "$line" && $actual == "$expected" ]] || {
    echo 'DIRECT_VERIFY_FAILED: backup identity, manifest or SHA-256 changed' >&2; exit 1;
}
printf 'DIRECT_VERIFY_PASS bytes=%s sha256=%s\n' "$expected_size" "$actual"
