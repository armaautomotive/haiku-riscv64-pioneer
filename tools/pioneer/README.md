# Pioneer relay control

The DSD TECH SH-UR04A relay appears on this Mac as
`/dev/cu.usbserial-1`. The Pioneer serial console is the separate
`/dev/cu.usbserial-0001` device; the scripts deliberately exclude that port
from automatic relay detection.

Channel assignments:

- Relay 1: Pioneer power-button contacts, wired through `COM` and `NO`.
- Relay 2: SD attachment control. Closed means attached; open means detached.
- Relays 3 and 4: currently unused.

```sh
tools/pioneer/pioneer_power.sh test
tools/pioneer/pioneer_power.sh on
tools/pioneer/pioneer_power.sh off
tools/pioneer/pioneer_power.sh restart
tools/pioneer/pioneer_power.sh sd-attach
tools/pioneer/pioneer_power.sh sd-detach
tools/pioneer/pioneer_power.sh sd-status
tools/pioneer/pioneer_power.sh status
```

`on` holds the physical power button for 0.75 seconds. `off` and `restart` hold
it for seven seconds and release it, providing the long press used for forced
shutdown or recovery.
Override the hold time with `--duration`, or select another serial port with
`--device`:

```sh
tools/pioneer/pioneer_power.sh restart --duration 12
tools/pioneer/pioneer_power.sh test --device /dev/cu.usbserial-1
```

The relay uses 9600 baud, 8 data bits, no parity, and one stop bit. The script
uses only the Python standard library; no `pyserial` installation is needed.

Never detach the SD card while either OS has it mounted or while it may be
reading or writing. Detach it only while the Pioneer is fully powered off.
Likewise, attach it before powering on. A single relay contact is suitable only
if it drives a purpose-built SD isolation/multiplexer control input; it must not
be used to interrupt only the card's power line while its signal lines remain
connected.

## Haiku build and SD deployment

### Source-built UEFI for SSD boot experiments

`pioneer_uefi_build.sh` builds the pinned SOPHGO `devel-sg2042` sources on
native RISC-V Linux, with `ACPI_ENABLE=FALSE` so Haiku can discover CPUs,
interrupt controllers, and the UART through the EFI device-tree table.
Pass an initialized recursive checkout of `sophgo/sophgo-edk2` at
`cd2d36fde3e0247a91fdb192f17882a4dc317159`. The script checks the three main
submodule revisions, and keeps its compiler cache and temporary files beside
the source checkout. It does not write an SD card or flash firmware.

Pass `DEBUG` as the optional second argument to build a separate DEBUG image
with `DEBUG_ON_SERIAL_PORT=TRUE`. The platform defaults to a null debug
library without that switch, even for a DEBUG target. RELEASE remains the
default. Use the verbose image for bounded PCI/storage discovery tests;
serial logging and assertions may change timing compared with RELEASE.

The upstream v1.4.3 ACPI build reached Haiku's loader in the September 5 test,
but Haiku reported zero CPUs and no UART. The older deployed DTB also lacks
the 16-bit `socket-id` property required by the firmware PCI host constructor.
That constructor exits before enabling any PCI roots when the property is
missing. The matching vendor DTB additionally changes PCI address windows;
firmware and DTB must be tested as a pair. SSD boot remains unverified.

The native source build with `ACPI_ENABLE=FALSE` completed successfully. The
generated PCD database defaults `PcdForceNoAcpi` to true. This is a firmware
enumeration experiment, not yet a desktop-ready replacement: the vendor DTB
uses per-domain PCI bus numbers starting at zero, while the current Haiku
Pioneer PCI code identifies the USB/SATA root using bus `0xc0`. That handoff
must be reconciled before calling SSD boot supported.

September 5 DT-mode probe: firmware `9986fd1658d32bb61d4a1580115e5c28fd593291a1aaae12ebb8432ba0ce17fe`
with vendor DTB `d57ec939b5d71513380eb4e4c8d5cb6efc15417c53f027bea7ae2b18500a138e`
started UEFI, but did not reach the Haiku loader. OpenSBI 1.8 reported
`Platform IPI Device: ---` and `Platform Timer Device: --- @ 0Hz` instead
of the previous `aclint-mswi` and `aclint-mtimer @ 50000000Hz`. The vendor
DTB's T-Head-specific split timer bindings are not compatible with the
currently deployed OpenSBI. Do not treat this firmware/DTB combination as
validated. Preserve the working timer/CPU bindings in the next experiment;
all of ZSBL, OpenSBI, UEFI, and Haiku consume this hardware description.
Rollback files for this probe have stamp `20260905T205157Z` in
`/mnt/ssd/haiku-deploy`; the backed-up firmware is the prior ACPI experiment,
not the known-good desktop firmware. The previous DTB is the working older
device tree. Partition 2 and both SSDs were not written by this deployment.

`pioneer_uefi_dtb.sh vendor.dtb working.dtb output.dtb` prepares the next
enumeration-test DT. It checks the vendor hash and working timer bindings,
restores the ACLINT compatible strings and `mtimecmp` register ranges, and
preserves vendor CPU interrupt phandles and PCI descriptions. The candidate
SHA-256 is `cc3739456a25cb2d86aa810de12314ab29d719c7fe85f2e3667de042bcd56ed8`.
That candidate was deployed with readback verification (backup stamp
`20260905T210531Z`). It failed in OpenSBI with
`clint-mtimer@70ac0b0000 ... init failed: -3`, before UEFI. The vendor root
compatible list omitted `sophgo,sg2042`, disabling OpenSBI's platform override
that pre-registers one combined region for all 16 timers; individual ranges
can otherwise exhaust the root-domain region table. The preparation script
now also restores that identifier. The corrected candidate is
`6efa4dc64bd6b3f3a6a597c40a16563bce520c6b2daac8df30172132dca5ee96`
and was deployed with readback verification (backup stamp
`20260905T211613Z`). OpenSBI now detects `aclint-mswi` and
`aclint-mtimer @ 50000000Hz`; UEFI passes a valid FDT to Haiku, and the loader
enables 64 CPUs and starts the kernel. The kernel then panics with
`did not find any boot partitions!`, after initial device discovery returns
`No such file or directory` and zero partitions. The loader selected the SD
partition at offset `0x40400000`. SSD enumeration remains unverified; scanning
only the selected boot disk does not establish whether other EFI Block I/O
handles exist. The next diagnostic should report all EFI disk handles and
keep firmware discovery separate from the kernel's vendor-DTB compatibility.
The SD contains this corrected candidate. Kernel, payload, and SSDs were
unchanged. Serial capture starts at byte offset 10752283 in the live log.

The following loader-only diagnostic prints every EFI Block I/O handle:
loader `477ccfeff4d309e8549d68fdd232f836d29e100e4858513005f077a64586bf05`,
firmware `a52c475c710ca6cb9db4cc4f0df66d1a3e71d290f518bcdae69626fd3f2df066`,
same corrected DTB. Deployment backup stamp: `20260906T045343Z` (Linux clock).
It reports exactly three handles: one removable whole disk, 512-byte blocks,
last block 61132799 (the 31,299,993,600-byte SD), and two logical partitions,
each with last block 2097151. Only one boot-device candidate is registered.
No SSD is exposed via EFI Block I/O. Linux's known capacities are Samsung
120034123776, R3SL480G 480103981056, and FORESEE NVMe 1024209543168 bytes.
The kernel repeats the no-boot-partitions panic. Next: instrument UEFI PCI
and storage-driver discovery; changing kernel disk selection cannot expose
a disk absent from the firmware's Block I/O list. Live-log byte offset:
10889256. The diagnostic firmware is currently on the SD.

Serial-debug firmware probe: `0fbced1a89a8921e2367a1408ac55c503bbc9ec45097789fc2444e00af05a775`,
same loader/DTB. Backup stamp `20260906T170247Z`; live-log offset 11025291.
DEBUG build completed in 5m29s, and PciHostBridgeDxe's library list confirmed
BaseDebugLibSerialPort. Firmware finds roots 0/2/3, starts xHCI, and enables
NVMe successfully, but xHCI Enable Slot commands time out and NVMe commands
time out (`NvmExpressDriverBindingStart: end with Not Found`). SATA controller
start succeeds and ATA pass-through begins; no SSD Block I/O handles appear.
The loader again reports only the SD and its two partitions. Kernel SDHCI
initialization reports `Bad data` and the kernel repeats its no-boot-partitions
panic. Next investigate firmware DMA/cache maintenance and completion polling;
the shared timeout pattern is evidence for that hypothesis, not confirmation.
This debug firmware is currently deployed. No kernel or payload was changed.

NVMe cache-coherency probe (`uefi-nvme-cache-probe.patch`, applied to the pinned
edk2 checkout) confirmed the hypothesis for NVMe. The firmware runs with
`PcdCpuRiscVMmuMaxSatpMode=0` and `PcdRiscVFeatureOverride=0`; generic PCI DMA
allocation/map paths do not perform cache maintenance. The probe cleans and
invalidates the Identify SQ/CQ/data buffers before submission and invalidates
the CQ during polling, using T-Head physical CMO encodings verified against
Linux `arch/riscv/errata/thead/errata.c`. It requires bare SATP, accepts only
blocking 4096-byte admin Identify, and halts after the result (or unexpected
command) before exposing an incompletely coherent disk.

Firmware `0dc98f820a370eef827742a616467e7269fa113ab315d6fde27bf1b329f7ba63`,
same diagnostic loader and corrected DTB, was deployed and readback-verified
with backup stamp `20260906T171855Z`. Serial at byte offset 11241634 reports:
`PIONEER DMA PROBE: Identify result=Success phase=1 expected=0 status=0/0; stopped`.
SQ=BEF7F000, CQ=BEF80000, PRP/buffer=BEF7D018. This confirms the NVMe command
path needs cache coherency handling; it does not yet validate USB, SATA,
Samsung SSD boot, or a complete DMA implementation. Next implement correct
DMA buffer ownership/cache handling and remove the diagnostic halt. The
remote edk2 checkout currently has this probe patch applied and the SD has
this intentionally halting firmware. Do not mistake it for a bootable release.

Next integration test: `uefi-nvme-dma.patch` replaces the Identify-only probe
in the pinned edk2 tree; `uefi-nvme-dma-platform.patch` is applied separately
to pinned edk2-platforms. Reverse the exact old probe with `git apply -R`
(check first), then apply these patches with `--ignore-space-change` because
the upstream files use CRLF. Do not apply both NVMe patches together.

The new implementation wraps all NVMe DMA mappings. Streaming mappings use
private page-aligned bounce buffers mapped through PCI common-buffer services;
device-to-host transfers invalidate that private storage before copying back.
The queue allocation is published after zeroing while the controller is
disabled, SQ entries and completed PRP lists are published before submission,
and both blocking and asynchronous completion paths invalidate CQ cache lines
before testing their phase. CQ lines are never cleaned while device-owned.
Physical T-Head cache instructions require bare SATP. This is opt-in through
the Pioneer NVMe component's `PIONEER_NVME_NON_COHERENT_DMA` define, not a
global change to PCI DMA or other RISC-V platforms.

For this first integration test, `PIONEER_NVME_READ_ONLY_TEST` also advertises
read-only media and restricts PassThru to Identify, internal queue creation,
and NVM Read. This prevents firmware writes to the Linux NVMe installation.
It does not validate write support, timeout recovery, or sustained async I/O.
SATA and USB DMA paths are unchanged and still require separate work.

DEBUG build completed successfully in 31 seconds with both defines confirmed
in the compiler invocation. Repacked firmware:
`033bad71bc9d53be0356f4b5e122d64af54e9fbaa4ea2843f72f54e52688faeb`
(`MilkV-Pioneer.v143-nvme-dma-readonly-haiku.fd`), with the same diagnostic
loader, corrected DTB, and unchanged BFS payload. Firmware-only deployment
was readback-verified with backup stamp `20260906T174215Z`; SD serial
`0x0000e752`, size 31,299,993,600 bytes. Linux was shut down cleanly before
the relay `on` command. Serial-log starting offset: 11387191.
Hardware result: NVMe controller and namespace Identify succeeded, model
`FORESEE XP1000F001T`, namespace size `0x773BD2B0` sectors. The NVMe driver
reported `end successfully`; firmware discovered all four Linux NVMe
partitions and installed the FAT filesystem on its EFI partition. This
demonstrates successful data reads, not only command completion polling.
Haiku's loader reported eight Block I/O handles: SD whole disk plus its two
partitions, and NVMe whole disk (lastblock 2000409263, 512-byte blocks,
1,024,209,543,168 bytes) plus four partitions. Two boot-device candidates,
up from one. No Samsung or R3SL480G SATA Block I/O handles appeared.

The unchanged Haiku kernel again failed SDHCI initialization with `Bad data`
under the vendor-derived DTB and panicked with `did not find any boot
partitions!`. This test validates initial read-only NVMe firmware access,
not Samsung boot, kernel DTB compatibility, or a usable desktop. Next apply
the ownership/cache approach to SATA, then reconcile the DT handed to Haiku.

SATA follow-up is deliberately an Identify-only diagnostic, not a complete
AHCI DMA implementation. `uefi-sata-cache-probe.patch` applies to edk2 on top
of the NVMe integration patch; `uefi-sata-cache-probe-platform.patch` enables
it only for Pioneer's AtaAtapiPassThru component in edk2-platforms. It publishes
initial transfer descriptors before enabling FIS reception. The Identify
probe stops command/FIS engines before rebuilding descriptors, uses a private
page-aligned buffer mapped as a PCI common buffer, publishes the command
table/list/FIS/data, and invalidates the byte-count and data buffers after
stopping DMA. It prints status, transferred bytes, and ATA model then halts.
Other PIO commands halt; DMA and non-data commands return Unsupported. No
sector I/O, SMART enable, or device-configuration command is permitted by
this probe. Existing read-only NVMe support is retained. Linux maps Samsung
to ata4 and the development R3SL480G to ata5 on the same SATA controller.

SATA probe DEBUG build completed in 30 seconds. Repacked firmware
`MilkV-Pioneer.v143-sata-cache-probe-haiku.fd` has SHA-256
`ea5da5db395f10e9a757c902e89e86447fba25b9478fed810fcd534ad0fc21d3`;
same loader/DTB/BFS as the read-only NVMe integration test. Firmware-only
deployment was readback-verified; backup stamp `20260906T175810Z`. Linux was
shut down cleanly, followed by a 20-second wait and relay `on`. Serial-log
starting offset: 11606272.

Hardware result: `PIONEER SATA PROBE: Identify port=3 command=EC`, data
`BE9B8000`, command list `BEABA000`, command table `BE9B9000`, then
`result=Success bytes=512 config=0040 model=[Samsung SSD 840 Series                  ]; stopped`.
The read-only NVMe driver also initialized and read its partitions again.
This validates Samsung Identify through the ownership/cache-synchronized
AHCI probe. It does not yet validate normal SATA sector reads, Samsung
partition discovery, or SSD boot. Next integrate safe streaming DMA and
descriptor/FIS maintenance into the normal AHCI paths. This build is on
the SD, intentionally halts in firmware, and is not a desktop boot image.

Next SATA integration: reverse the exact SATA probe and platform probe
patches, then apply `uefi-sata-dma-readonly.patch` to edk2 and
`uefi-sata-dma-readonly-platform.patch` to edk2-platforms. The NVMe integration
patches remain applied. `PIONEER_SATA_NON_COHERENT_DMA` opts only Pioneer's
ATA pass-through component into this experimental, blocking/read-only path.
Identify, ordinary sector reads, and Read Log Ext are allowed; data writes,
ATAPI packets, and asynchronous requests are rejected. Non-data commands are
restricted to Set Features / transfer mode (feature 03), a volatile setting
required by normal driver initialization. SMART enable, security/erase,
PUIS, and other configuration commands are rejected.

PIO and DMA data mappings use private page-aligned bounce storage. AHCI
common descriptors retain their original PCI mappings and teardown paths;
they are published after initialization and each command build. The active
port is quiesced before rewriting shared descriptors and before releasing
streaming mappings. Failure to stop DMA deliberately halts rather than
reclaiming possibly device-owned memory. PRDBC and received-FIS buffers are
invalidated before CPU reads. This is not yet write/async support or a
production error-recovery implementation. Cache/bounce helpers currently
duplicate the proven NVMe experiment locally; consolidate them behind a
platform DMA abstraction after the SATA behavior is validated.

SATA read-only DEBUG build completed in 30 seconds. Repacked firmware
`MilkV-Pioneer.v143-sata-dma-readonly-haiku.fd` SHA-256:
`bbff6ac04e448df90afe1f6cd8706c45a8119a4b8d3c2fa6c108b9d6c893f88c`.
Embedded diagnostic loader, corrected DTB and BFS payload remain unchanged.
Firmware-only deployment readback passed, backup stamp `20260906T181147Z`.
After clean Linux shutdown and a 20-second wait, relay `on` started the test.
Serial-log starting offset: 11755756.

Hardware result: AHCI Identify and normal initialization succeeded on ports
3 and 4. Firmware read the partition tables of both SATA disks. Haiku's
loader reports 15 Block I/O handles and four physical candidates: SD,
FORESEE NVMe, Samsung (512-byte blocks, lastblock 234441647 =
120,034,123,776 bytes), and R3SL480G (lastblock 937703087 =
480,103,981,056 bytes). Samsung's partition is exposed with lastblock
234440703; the development disk has four logical partition handles. This
validates blocking SATA reads and EFI disk/partition discovery, not just
Identify. No SATA/NVMe data writes were enabled by the firmware test.

The loader still selected the SD payload: its scan shows partition starts
4194304 and 1077936128, each 1 GiB. Samsung BFS mounting/boot selection was
not exercised. The unchanged kernel then reported SDHCI `Bad data`, PCI ECAM
`Operation not supported`, and `did not find any boot partitions!` under
the vendor-derived DTB. Next reconcile the kernel's DT/PCI expectations and
exercise explicit Samsung boot selection. Do not call SSD boot completed.

Kernel handoff diagnosis: `sdhci_fdt.cpp` requires an interrupt description
even though the current SG2042 controller runs polled. The v1.4.3 vendor
SD node has no `interrupts` or `interrupt-parent`. The working reference
node `/soc/bm-sd@704002B000` has `interrupts = <0x88 4>` and references its
PLIC. The vendor PLIC uses the same two-cell interrupt format but a different
phandle. `pioneer_uefi_dtb.sh` now accepts optional `--sd-interrupt`, validates
both physical SD register tuples and PLIC formats/parent identity, and adds
the reference IRQ with the vendor PLIC's phandle. No phandles are copied
blindly between trees.

The resulting `/private/tmp/pioneer-v143-aclint-sd-irq.dtb` SHA-256 is
`2adfe4509b1491a5f8be8bce21ebd68699c6f7743e96190bd5af77095acd4116`.
Decompiled comparison against the preceding DT shows exactly two added
properties under `/soc/bm-sd@704002b000`: `interrupts = <0x88 4>` and
`interrupt-parent = <0xa4>`. Firmware, embedded loader, kernel and BFS
payload are unchanged. This isolated test targets kernel SD initialization;
it does not fix PCI bus numbering or select the Samsung boot volume.
Deployment readback passed with backup stamp `20260906T182615Z`. After
clean Linux shutdown and a 20-second wait, relay `on` started the test;
serial-log starting offset 11983008.

Hardware result: kernel SD initialization now succeeds, reporting registers
`0x704002b000`, size `0x1000`, IRQ 136, and `P202:SD7 controller ready`.
VFS finds one boot partition, mounts the SD boot volume, brings up system/home
packagefs, and starts post-boot services. All 64 CPUs were enabled and
launch_daemon/app_server started. The old no-boot-partitions panic is resolved
for this SD test by the two DT properties alone.

The next failure is a kernel `loadAccessFault` during
`radeon_hd_init(radeon_info&) + 0x5fe`, called when app_server opens
`graphics/radeon_hd_010000` (Caicos 1002:6779). Thread 170 on CPU 12,
EPC `0xffffffc00237bfea`, tval 0. This identifies the fault site, not its
root cause; examine the GPU MMIO/resource assumptions under the new firmware
PCI layout. Samsung boot selection remains untested. No kernel, loader,
or firmware binary was changed in this DT-only boot cycle.

The following PCI test addresses an observed resource-layout conflict: the
kernel's legacy Caicos fixup assigned MMIO PCI address `0x50000000`, outside
the new firmware root's MMIO windows. The faulting instruction reads
`CONFIG_MEMSIZE` at register offset `0x5428`. `ECAMPCIControllerFDT::Finalize`
now uses controller-local configuration accesses, validates the firmware BARs
against root resources and bridge windows, and preserves valid assignments
(`P326`). Legacy fixed assignments are allowed only when their ranges fit.
This also avoids applying a global bus-1 fixup through multiple PCI domains.
The built test payload is `haiku-pioneer-bfs-pci-firmware-bars.img`, SHA-256
`bfd81b9532b914677690019478492058f880696b775f12a564a8b69595cdc5e6`.
Firmware, loader and SD-interrupt DTB are unchanged. Deployment and complete
payload readback passed, with rollback stamp `20260906T192610Z`. After clean
Linux shutdown, a 20-second wait and relay `on`, serial capture starting at
offset 12232551 confirms `P326` preserved FB `0x4200000000` and MMIO
`0xe0000000`. Radeon initialization passed the former fault site and proceeded
through framebuffer configuration and display power-up. Visual output and
input still need user confirmation; Samsung boot selection remains untested.

The user subsequently confirmed the experimental instance lacks keyboard and
mouse. Its peripheral PCI root was skipped with `P233` because the kernel
recognized the single-config-aperture link only by bus base `0xc0`; the vendor
tree uses zero. The next test identifies this Pioneer link by physical config
aperture `0x4c00000000`, size `0x1000`, matching both trees. It retains the
legacy topology setup only for the old bus numbering and initializes existing
SG2042 MSI support for either layout. `P327` marks firmware-layout handling.
No xHCI, HID, or Radeon driver code changed in this test.
Payload `haiku-pioneer-bfs-uefi-usb-root.img` SHA-256:
`3940402ceea3d3cc0e23f98f0b10da15f543154276a26bfccc513c395fe7f164`.
Build succeeded; the EFI loader is unchanged. Payload-only SD deployment and
full readback passed (backup stamp `20260906T194240Z`). After clean Linux
shutdown and a 20-second wait, relay `on` started the test, serial offset
12500671. This attempt paused at `P205:SP0 direct PHY config write 0xee0003`
during SD initialization, before PCI-root initialization or `P327`. No panic
was captured; USB validation remains pending. Do not attribute this stall to
execution of the changed PCI code: that code has not yet been reached.

An unchanged-image power-cycle retry (serial offset 12642625) passed SD PHY
initialization and reached `P251`/`P327`. The peripheral root is now enumerated
as Haiku PCI domain 2, with ASMedia switch, NVMe, xHCI, both RTL8125 endpoints,
and JMB585 visible. SG2042 MSI allocated vectors 128-159. xHCI initialization
then failed with `unsupported interface version: 0x0000`; register mapping,
bridge decoding and firmware handoff still require investigation. This is
not a working input result. The SD boot volume subsequently mounted normally.

The next test corrects memory BAR translation in the PCI manager. The old
`pci_ram_address()` lookup searches all domains, so the vendor tree's
overlapping PCI memory windows can select a different root's CPU address.
Device memory BARs and ROM addresses now use the owning domain's ranges;
x86 identity translation and legacy I/O handling are unchanged. `P328` logs
the USB BAR PCI/CPU addresses. No USB driver change is involved.
Built payload `haiku-pioneer-bfs-pci-domain-map.img` SHA-256:
`02993ed073aa8d0c70bfd4777e4e484098505e31cb90f5f61a7c9619465ed50b`.
Loader/firmware/DT are unchanged. Payload deployment and full readback passed,
backup stamp `20260906T200721Z`. Linux was shut down cleanly, followed by a
20-second wait and relay `on`. No fresh boot text appeared in the next
40 seconds (serial offset 12920403, only a NUL appended). Hardware validation
is pending; absence of serial output alone does not establish power state.
After the user powered on, fresh UEFI and Haiku output appeared in that same
capture. The boot again stopped progressing at `P205:SP0 direct PHY config
write 0xee0003`, before `P328` or peripheral PCI initialization. The domain
translation fix is therefore still untested on hardware; no panic was logged.
The user-authorized unchanged-image power-cycle retry (serial offset 13062357)
again reached the same SD PHY line without advancing to PCI. Between `SP0`
and `SP1`, the source performs the PHY register store, a full memory barrier,
and `spin(1000)`; the existing log cannot distinguish which operation stalls.
Further retries alone do not validate the domain mapping fix.

SD checkpoint test: payload `haiku-pioneer-bfs-sd-phy-checkpoints.img`, SHA-256
`4becbf3328a5a7f923349ef981cae4433c7efc8707befd2ec1d49b2d9ea8cf32`,
deployed/readback verified with backup stamp `20260906T210830Z`. Serial offset
13269661 shows all three `P329` markers (store issued, barrier returned, spin
returned), followed by normal boot progress but xHCI version zero. Logging
changes timing, so this does not establish that the SD stall is fixed.

Correction to the PCI mapping test claims above: the source sync used uppercase
`PCI.cpp`/`PCI.h`, aliases of the tracked lowercase files on the Mac workspace
but distinct files on the case-sensitive build volume. Jam builds lowercase
`pci.cpp`, so those images did NOT contain the mapping change. The missing
`P328` in both boot output and the built PCI binary exposed this mistake.
The mirror has now been updated using the tracked lowercase names; require
source comparison and the `P328` marker in the built/packaged PCI binary before
deploying the replacement. Earlier SD and ECAM diagnostics were present.
Preserved source timestamps also left the stale object newer than the corrected
source. Explicitly touching the two lowercase mirror sources forced the needed
recompilation. `P328` is now verified in both the linked PCI binary and package
contents. Replacement payload `haiku-pioneer-bfs-domain-map-recompiled.img`,
SHA-256 `c577971a4e3c906d8321cd87d0f16ec7ba26832825e49c8da669223782ab718e`,
is built but NOT deployed. Do not deploy the earlier misleadingly named
`haiku-pioneer-bfs-domain-map-verified.img` (it still had the stale object).
The user confirmed the current SD-checkpoint boot reached the welcome screen
without mouse input; the board remains running that earlier payload.

Corrected-image deployment: `c577971a...` was backed up and readback verified
with stamp `20260906T212048Z`; clean Linux shutdown, 20-second wait, relay on.
Serial offset 13547952 confirms all `P329` SD stages passed and `P328` executed:
USB domain 2 bus 4 BAR PCI `0xe0300000`, host `0`, size `0x8000`. That PCI
address is outside this root's declared memory windows. xHCI then panicked on
loadAccessFault in its constructor (+0x144). The domain-scoped lookup exposed
an unmatched address, but returning zero without rejecting the mapping was
unsafe. Next work must handle unmatched resources safely and reconcile actual
firmware BAR assignments with the peripheral root's declared/outbound windows.
This is not a USB success; the board is halted at the kernel debugger.

Firmware resource reconciliation: ShowPciResource and PciHostBridgeLib confirm
the v1.4.3 firmware allocates PCI memory `0xe0000000` size `0x20000000` on
each root, with CPU prefixes `0x40`, `0x48`, `0x4c`. Its PCD allocation tables
do not match the vendor DT ranges. `pioneer_uefi_dtb.sh --firmware-pci-ranges`
now optionally emits the three actual firmware windows per root; a decoded
DT comparison confirmed only those three `ranges` properties changed.
Test DT `pioneer-v143-firmware-pci-ranges.dtb` SHA-256:
`dac43f6860c07a6950a6ea09cdc8d3a9c9fe6789d7ccab9a8e7d374569777010`.
The xHCI constructor now rejects zero, undersized, or I/O BAR mappings before
mapping/dereferencing the register aperture. Packaged binary markers verified.
Payload `haiku-pioneer-bfs-pci-map-guard.img` SHA-256:
`8c9653467833bd123c99c109f20dc69ceedcaee4e8d667b4aa2c9268f6afc5bf`.
Both deployed/readback verified, rollback stamp `20260906T213200Z`; unchanged
firmware and loader. Clean Linux shutdown, 20-second wait, relay on; capture
offset 13774024. All SD checkpoints passed. P328 now maps USB PCI
`0xe0300000` to CPU `0x4ce0300000`, size `0x8000`. xHCI reads version `0x0110`,
uses MSI and successfully starts. Hub descriptor/device initialization errors
remain; input functionality has not been confirmed. Kernel launches userland.

Checkpoint confirmation: the user subsequently confirmed both mouse and
keyboard work on this exact `8c965346...` payload / `dac43f68...` DT boot.
Graphics and input are restored under the new UEFI firmware. This is the
working checkpoint for continuing Samsung SSD boot selection, which remains
untested. The intermittent SD PHY startup stall and firmware USB enumeration
errors are still unresolved; retain the diagnostic patches as experimental
history, not production-ready firmware support.

For firmware-only experiments, add `--firmware-only --dtb FILE
--dtb-sha256 HASH` to the deployment command below. The payload is still
validated, but partition 2 is not written or backed up. The previous DTB is
backed up alongside the firmware; both replacements are staged and verified
before publication. FAT cannot publish two files atomically, so do not reboot
after a partial or failed deployment. Restore both backed-up files first.

Keep the previous SD firmware and DTB together as a rollback pair. Do not
flash SPI as part of these SD-based tests.

The Pioneer firmware image used by this port contains an embedded Haiku EFI
loader. That embedded loader is the copy the board actually executes; updating
only `EFI/BOOT/BOOTRISCV64.EFI` is insufficient.

Build the image and BFS payload with `pioneer_build.sh`, then repack the newly
built loader into either a known-working Haiku firmware image or an upstream
SOPHGO Pioneer firmware image. The latter contains the UEFI shell application;
the repacker replaces that application in place while retaining the upstream
PCI, NVMe, SATA, and USB DXE drivers:

```sh
tools/pioneer/pioneer_firmware_embed.sh \
  --base /path/to/known-working/MilkV-Pioneer.fd \
  --loader /path/to/generated.riscv64/objects/haiku/riscv64/release/system/boot/efi/haiku_loader.efi \
  --output /path/to/MilkV-Pioneer.updated.fd
```

The repacker validates the Pioneer firmware GUIDs and layout, preserves the
vendor firmware outside the compressed DXE allocation, decompresses its own
output, and compares the embedded loader byte-for-byte with the input loader.
It discovers validated firmware-volume and application offsets instead of
assuming one particular SOPHGO release layout.

Deploying a loader now requires both the standalone loader and the verified
repacked firmware:

```sh
tools/pioneer/pioneer_sd_deploy.sh \
  --device /dev/mmcblk1 \
  --payload /path/to/haiku-pioneer-bfs.img \
  --sha256 BFS_SHA256 \
  --loader /path/to/haiku_loader.efi \
  --loader-sha256 LOADER_SHA256 \
  --firmware /path/to/MilkV-Pioneer.updated.fd \
  --firmware-sha256 FIRMWARE_SHA256 \
  --apply
```

Before writing, the deployment script independently decompresses the supplied
firmware and rejects it unless its embedded loader hash matches
`--loader-sha256`. It backs up and readback-verifies the BFS payload, standalone
loader, and booted firmware.
