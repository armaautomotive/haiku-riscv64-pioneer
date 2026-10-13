# Pioneer SD boot recovery and xHCI cleanup, 2026-10-12

## Verified baseline

The persistent SD is healthy and boots Haiku with the P100A removed. SSH
verified `/boot` as BFS on `/dev/disk/mmc/0/1`, 4 GiB total with about
2.8 GiB free. Tracker, Deskbar, app_server, input_server, net_server and
sshd were running; the user confirmed a visible desktop and responsive
mouse/keyboard. The login account is `baron` (UID 0).

The SD remains CID `ad4c534d53534430610000e752019300`, Linux device
`/dev/mmcblk1`, 31,299,993,600 bytes. Its 1 GiB FAT firmware partition and
4 GiB BFS partition were not rewritten. Firmware, DTB, EFI loader, early
boot code and system package matched the September 16 recovery records.
A read-only FAT check found a dirty flag but no allocation errors. A
read-only BFS check of a complete 4 GiB snapshot found 612 nodes and zero
missing, duplicate or freeable blocks.

Snapshot (ignored local artifact):
`generated.pioneer/sd-recovery/haiku-sd-current-4g-20261012.img`, SHA-256
`987165a790a12170c75554a9a2833e4c8c14b9d8a698559c9e3932fd94940d0f`.
The full-card rollback remains on Linux at
`/mnt/ssd/haiku-deploy/sd-expand-4g-20260916/sd-full-before.img`.
Do not deploy the generic 300 MiB diagnostic filesystem over this card.

## Failure isolated to P100A configuration

With P100A `1e52:b140` installed, UEFI's PCI allocation for `PcieRoot(0x2)`
failed with a 64-bit memory request of length `0x820100000` and alignment
`0x7ffffffff`; its retry failed too. The subsequent Haiku boot found the
USB controller's register BAR at address zero, size `0x8000`.

The xHCI constructor correctly rejected this BAR, but its destructor
unconditionally called `WriteOpReg(XHCI_CMD, 0)`. Registers were unmapped
and the operational register offset was uninitialized. The resulting
store to `0xcccccccc` panicked at `XHCI::~XHCI() + 0x2a`.

After physical removal of the P100A, unchanged firmware reported
`AllocateResources - Success`, started its XHCI driver, and booted the
same SD to a working desktop. This A/B test identifies a PCI-allocation
trigger, not an SD bootloader/filesystem failure.

Ignored serial evidence:

- `generated.pioneer/sd-recovery/haiku-sd-boot-20261012.log`: with-P100A panic.
- `generated.pioneer/sd-recovery/haiku-without-p100a-boot-20261012.log`:
  successful baseline, starting at combined serial byte offset 144822.

## Narrow cleanup fix and checks

`src/add-ons/kernel/busses/usb/xhci.cpp` initializes all four register
offsets to zero and stops the controller during destruction only when
`fRegisterArea >= B_OK`. Later failures after successful mapping must
still stop the controller; do not gate this write on `fInitOK`.
Existing thread, mutex, area and MSI teardown remains unchanged. This
does not fix PCI resource allocation or make P100A usable on the Pioneer.

Run the host regression with:

```sh
sh tools/pioneer/tests/run_xhci_cleanup_test.sh
```

It extracts the production destructor into a minimal kernel-API fixture
and runs with address/undefined-behavior sanitizers and warnings as errors.
It checks rejected BARs, mapping errors, valid area ID zero, mapped partial
initialization and DMA/MSI cleanup. It passes with the patch and fails
against the previous destructor with an unmapped-register access. It is
not a full USB simulation or proof of runtime behavior on hardware.

Restored official Haiku buildtools at revision
`33d46bf5c300bd34cd07c596431e4545c754ac03` in
`/private/tmp/pioneer-buildtools-20261012`; its Jam bootstrap passed.
Only the xHCI source was synchronized to `/Volumes/HaikuBuildLocal/haiku`.
The existing configured RISC-V build successfully rebuilt `<usb>xhci`
(three targets). Expected architecture/package notices and existing kernel
linker orphan-section warnings remain; no compiler errors occurred.
RISC-V disassembly confirms a signed-negative area check branches around
the register store.

Fixed driver: 111,187 bytes, SHA-256
`dfaa3f1f61e2c14c1ee26902d90ea5fcc5bb425fad65961bdf9127db8ec8e806`.
Packaged original SHA-256:
`9da29bfa55c05ba2c4b22974371d88e6dd10472effeb62d65408a36f1cfe929e`.

## Reversible SD test

An experimental **user** non-packaged override was staged with a boot
symlink so the driver could be preloaded before the root filesystem mounts.
**This boot method failed validation; do not repeat it as a deployment recipe.**

- `/boot/home/config/non-packaged/add-ons/kernel/busses/usb/xhci`
- `/boot/home/config/non-packaged/add-ons/kernel/boot/xhci`
  pointing to `../busses/usb/xhci`.

The bootloader reads these user paths from the BFS root and prepends their
images after reading packaged modules. The test log reports duplicate xHCI
module names. It does not establish that this arrangement is safe for the
Pioneer bootstrap. The original packaged driver and system package remain
intact. Do not attempt to reload the USB controller live.

The hash-checked backup/staging directory is
`/boot/home/pioneer-xhci-cleanup-20261012`, outside active add-on paths.
Rollback is to move both override entries into that directory, then reboot.
Do not remove or replace the packaged driver. If Haiku cannot boot, use
the established Linux recovery path and the saved SD snapshot.

Staging passed: the backup and packaged original matched the original hash,
and both user override paths matched the fixed driver hash. The package hash
was checked before staging and no package was replaced. A normal
`shutdown -r` closed the desktop and SSH but did not restart the board. The
user requested a hardware restart; relay off (7 seconds), ten-second wait,
then relay on (0.75 seconds) produced a fresh firmware/Haiku boot.

That boot passed firmware PCI allocation but panicked at about 94.4 seconds
with `did not find any boot partitions!`. Device discovery published the
FDT blob but did not register the SD controller, PCI devices or native USB
controller. The xHCI constructor/cleanup failure path was not exercised.
This is a new early device-discovery failure, not proof of a bad cleanup
guard or of SD corruption. Its relation to duplicate preloading remains
unproven until an unchanged-package baseline is retested without the override.
Evidence: ignored `generated.pioneer/xhci-cleanup/haiku-override-boot-20261012.log`,
combined serial start offset 431481.

## File-level rollback under Linux

After the user removed the SD, Linux booted from `/dev/nvme0n1p3`. The
user reinserted the SD; its CID and both partition sizes matched the records
above. Both partitions were unmounted before backup and maintenance.

A fresh complete 4 GiB partition backup is on the separate R3SL staging SSD:
`/mnt/ssd/pioneer-xhci-rollback-20261012/sd-before-rollback.img`, SHA-256
`fce7aa1c3203b1326c43c787eecdc69aa468519f4fcd230824940e47ac3a5e1e`.
The directory is private; raw filesystem backups must not be pushed to GitHub.

Linux's in-kernel BFS driver is read-only. Built this checkout's native
`<build>bfs_shell` on Linux/riscv64 (91 targets), using the restored Jam source.
The host-only configure path unnecessarily required NASM, then replaced it
with a dummy target-assembler command. Changed `configure` to skip only the
unused NASM check for `--host-only`; target-build validation is unchanged.
Configuration passed without NASM, with `--no-xattr --no-downloads`. No
Linux packages were installed. Build workspace and tool:

```text
/mnt/ssd/pioneer-xhci-rollback-20261012/host
/mnt/ssd/pioneer-xhci-rollback-20261012/host/objects/linux/riscv64/release/tools/bfs_shell/bfs_shell
```

Rehearsed the following two renames on a copy of the fresh backup, then
performed the same operations on the verified, unmounted `/dev/mmcblk1p2`:

- Move the user `boot/xhci` symlink to
  `/home/pioneer-xhci-cleanup-20261012/disabled-boot-link`.
- Move the user `busses/usb/xhci` file to
  `/home/pioneer-xhci-cleanup-20261012/disabled-driver`.

No driver was deleted. Only these test entries were moved out of active
add-on paths; no full-filesystem payload, partition table, firmware or package
replacement was performed. The copy and actual card passed `checkfs -c`
before and after: 614 nodes, zero missing/duplicate/freeable blocks. All 11
command replies in each run were zero. Extracted system package, original
driver backup and disabled candidate matched their recorded hashes.

An independent read-only Linux mount confirmed both active override entries
were absent and rechecked the system package, firmware, DTB and EFI loader
hashes. Both partitions were unmounted again. Logs and audit files are saved
in the rollback workspace (`rehearsal.log`, `live.log`, `*.reply`, and audit
directories). The original full-card and pre-override backups remain intact.

Linux was shut down cleanly, followed by a twenty-second wait and relay on
with the SD still inserted. The baseline re-boot succeeded. SSH verified
Haiku on `/dev/disk/mmc/0/1`, 4 GiB total with 2.8 GiB free (1,473,257
blocks of 2048 bytes). Tracker, Deskbar, app_server, input_server, net_server
and sshd are running. The active override entries remain absent; the
packaged xHCI driver still has its original hash. `listimage 1` shows the
original single xHCI image, ID 47. No panic or duplicate-module warning
appeared in the inspected rollback boot log.

Evidence: ignored `generated.pioneer/xhci-cleanup/haiku-after-rollback-20261012.log`,
combined serial start offset 650820. Restored device discovery after removing
the extra preloaded driver implicates the override arrangement; the precise
bootstrap failure mechanism remains undiagnosed. Do not claim the cleanup
guard caused the disk failure or that the new driver passed a hardware test.

Haiku is left running with the original driver, the SD inserted, and the
P100A removed. Post-rollback physical desktop/input confirmation is pending.
The cleanup fix is saved/built but disabled on this SD. Before another runtime
test, prepare a backed-up package-level driver update that does not preload
both copies; do not overwrite the persistent filesystem or firmware.

Host-only configure checks also passed shell syntax validation. An isolated
NASM-check test succeeds for `HAIKU_HOST_BUILD_ONLY=1` with a nonexistent
assembler and still rejects it for `HAIKU_HOST_BUILD_ONLY=0`.
