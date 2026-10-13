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

Use a **user** non-packaged override, with a boot symlink so the driver
can be preloaded before the root filesystem mounts:

- `/boot/home/config/non-packaged/add-ons/kernel/busses/usb/xhci`
- `/boot/home/config/non-packaged/add-ons/kernel/boot/xhci`
  pointing to `../busses/usb/xhci`.

The bootloader reads these user paths from the BFS root and prepends their
images after reading packaged modules. They therefore register first.
The original packaged driver and system package remain intact. Do not
attempt to reload the USB controller live.

The hash-checked backup/staging directory is
`/boot/home/pioneer-xhci-cleanup-20261012`, outside active add-on paths.
Rollback is to move both override entries into that directory, then reboot.
Do not remove or replace the packaged driver. If Haiku cannot boot, use
the established Linux recovery path and the saved SD snapshot.

Staging passed: the backup and packaged original matched the original hash,
and both user override paths matched the fixed driver hash. The package hash
was checked before staging and no package was replaced. A normal
`shutdown -r` closed the desktop and SSH, but no new firmware output had
appeared during the initial checks; physical board-state confirmation is
pending before relay recovery. Reboot verification remains pending.
Keep the P100A removed for
the normal-controller regression test; reconnecting it is a separate,
supervised hardware test and is not necessary to validate the healthy path.
