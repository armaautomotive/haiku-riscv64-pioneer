# Pioneer SD expansion: 300 MiB BFS to 4 GiB

Approved scope: preserve the 1 GiB FAT firmware partition; enlarge partition 2
and its Haiku filesystem to 4 GiB. Do not write Samsung or Linux storage except
for backups/staging on the separate R3SL volume at `/mnt/ssd`.

## Identity and original geometry

- Linux device: `/dev/mmcblk1`, 31,299,993,600 bytes (29.2 GiB).
- CID: `ad4c534d53534430610000e752019300`.
- DOS partition table, disk ID `0x5be3c9a2`, 512-byte sectors.
- p1: start 8192, size 2097152 sectors, type c, FAT label HAIKUPNR.
- p2: start 2105344, size 2097152 sectors, type eb, BFS label Haiku.
- Existing BFS geometry: 314572800 bytes despite a 1 GiB partition.
- Proposed p2: same start/type, size **8388608 sectors** (4 GiB).
- No other partitions reported. Remaining space stays unallocated.

## Backups and candidate

Linux staging: `/mnt/ssd/haiku-deploy/sd-expand-4g-20260916`.
Full raw card backup: `sd-full-before.img`; partition text backup:
`partition-before.sfdisk`. Do not modify partitions until the full raw backup
has completed and its byte count/hash have been recorded.

Full backup completed: 31,299,993,600 bytes, SHA-256
`d17bf043e4486ea73a11fb94505585a4488251826e9b67adfa238346e3a6aee3`.
The full 1 GiB FAT partition matches its backup segment byte-for-byte, and
the 300 MiB BFS snapshot matches its segment in the full-card backup.
Both SD partitions were unmounted for these operations.

Fresh unmounted BFS snapshot: `bfs-current-300m.img`, SHA-256
`f0295af7e232b01fef1159fbacb5525184791fee6c4f961f21120a68df62ed9c`.

Mac candidate: `/private/tmp/verse-sd-expand-4g-20260916/haiku-4g.img`.
4 GiB image SHA-256:
`bcee44dbd5032b78134741f63c9da0286a98af5024da026675e445ddd4c96302`.
Compressed staging copy: `haiku-4g.img.gz` (gzip -1).

## Migration method and checks

This checkout's BFS ResizeVisitor returns B_NOT_SUPPORTED. Its existing cp
copies attributes but does not apply ownership/timestamps. Added an offline
`clonefs` command to the **host** bfs_shell, not the deployed kernel:

1. Require a newly initialized empty destination.
2. Mount source image read-only; reject unsupported nodes and non-directory
   hard links rather than silently losing their semantics.
3. Create source typed indices before copying attribute-bearing files.
4. Copy contents, symlinks and typed attributes directly between BFS mounts.
5. Recursively compare names, bytes, links, attribute names/types/bytes.
6. Apply and verify ownership, modes, modification and creation timestamps.

The existing interactive bfs_shell process can exit 0 even if a command fails;
require the explicit `clonefs: copy and verification passed (0)` message and
the subsequent consistency checks, not only the shell exit code.

Actual source migration passed. Source image checksum remained unchanged.
The fresh filesystem passed `checkfs -c`: 612 nodes, zero missing/double/freeable
blocks; 254 files, 121 directories, 112 attributes, 79 attribute directories,
46 indices. It reports 4 GiB total and about 3.7 GiB free.

Then replaced only the haiku system package with the already-prepared
user-space startup-timing candidate. Package readback matches:
`c7e0d87fcd3c15c284c320586b92455359e4341ab5514474a7ad0aee034f3d62`.
The second checkfs -c passed with the same node counts and no allocation errors.
The kernel remains the already-tested tracing kernel; bootloader/firmware
files are outside the rewritten filesystem.

## Deployment safeguards

Updated pioneer_sd_deploy.sh derives payload size from the BFS superblock,
requires exact matching image length and whole-MiB size, rejects malformed
geometry and rejects any payload smaller than the installed BFS filesystem.
Backups cover the entire target partition, and write/readback lengths use the
validated payload geometry. This prevents an old 300 MiB image from silently
shrinking the expanded installation. Host checks recognize both real 300 MiB
and 4 GiB images, reject a truncated header, and pass shell syntax checks.

The default image build still produces a 300 MiB *fresh-install* image. Do not
use it as a whole-filesystem update for this persistent card. For future updates,
back up the existing larger filesystem and replace the intended package in a
copy (or use a properly staged native package update after sufficient free
space is verified).

## Deployment results

Partition 2 was expanded in place to 8388608 sectors. Binary MBR comparison
confirmed all bytes outside its partition entry unchanged, including boot
code, disk ID and partition 1; p2 start/type remain unchanged.

Deployed with `--no-backup` because the verified full-card rollback above was
already present. The script's complete readback passed. A second independent
`dd iflag=direct` read bypassing Linux's page cache produced the same 4 GiB
SHA-256, `bcee44dbd5032b78134741f63c9da0286a98af5024da026675e445ddd4c96302`.
The script now uses direct I/O for future readback verification too.

A read-only Linux mount reported 4294967296 total bytes, 282798080 used and
4012169216 available. The installed system package matched the candidate hash
above. The filesystem was unmounted again. A dry-run using the old snapshot
failed as intended: "Refusing to shrink installed BFS from 4294967296 to
314572800 bytes". No Samsung or Linux-system partition was modified.

Expanded-volume boot validation is pending. Linux was shut down cleanly with
`shutdown -h now`; after the ten-second wait, `pioneer_power.sh on` completed.
No new firmware/Haiku serial output appeared during the following checks, and
SSH did not connect. This does not establish a filesystem or kernel failure;
board startup needs confirmation before diagnosing the candidate.

### Successful boot validation

User confirmed no fans on the first attempt. A second power-on command started
UEFI, and the user subsequently confirmed Haiku was up. SSH verified `/boot`
is BFS on `/dev/disk/mmc/0/1`, 2097152 blocks of 2048 bytes (4 GiB).
It reported 1473365 free blocks (about 2.81 GiB); boot created a 960 MiB
`/boot/system/var/swap`, accounting for the main increase in used space.
The running kernel SHA-256 remains
`693794818248a33e3df9d7a0486639a645925a89966b480811559c13426c71d0`.
User-space startup timing records are present for user launch_daemon,
Deskbar and Tracker. See `benchmarks/KERNEL_TRACING_PLAN.md` for measurements
and the remaining early system launch-daemon logging gap.
