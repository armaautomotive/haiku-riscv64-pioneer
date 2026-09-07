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

Samsung update preparation (no Samsung writes yet): Linux identifies the
Samsung SSD 840 as `/dev/sda`, with its existing 111.8 GiB BFS installation on
`/dev/sda1`. It is mounted read-only at
`/mnt/ssd/haiku-deploy/samsung-inspect.wxNpcj`. The old system package is backed
up under `/mnt/ssd/haiku-deploy/samsung-system-backup.908Twq/`, SHA-256
`6e0b8977344bf316f85d220c20ee190a83d644eb3f82e81b0dac75ce34cab848`.
The working checkpoint system package is staged on SD FAT at
`pioneer-update-8ecc61384c/haiku-r1~beta6_hrev99999-1-riscv64.hpkg`, with
readback SHA-256 `16a5fd3df8af53c2c8128eb469246b0d42f579bb36237df3b40b1ca985e62c9a`.
This is staging only, not an installed update or successful Samsung boot.
Next use the working SD Haiku system to inspect/mount the Samsung and apply
a backed-up system-package update without formatting or replacing home/settings.

The ensuing SD boot (serial offset 14093225) reached graphics/input, but AHCI
mapped its BAR through the wrong root (`0x40e0100000`, AHCI version zero).
The device-specific PCI `ram_address` callback still ignored the device and
called the legacy all-domain lookup. It now calls `PCI::RamAddress` using the
owning domain, reusing the already-corrected BAR translation helper. The
compiled callback relocation was verified to call this method. No SATA
transfer code changed. New, not-yet-deployed payload:
`haiku-pioneer-bfs-pci-device-domain.img`, SHA-256
`780f197609a52b64759baabefcab7ee6271431a1366f93210e53ff22024315e9`.
Do not apply the older staged Samsung package until SATA is validated; stage
the subsequently validated system package instead. Samsung remains unchanged.

Device-domain fix deployed and fully readback verified with rollback stamp
`20260906T220241Z`. Clean Linux shutdown, 20-second wait, relay on; serial
offset 14400511. AHCI now maps PCI `0xe0100000` to CPU `0x4ce0100000`, size
`0x2000`, reports version 1.3.1 and starts port 3 at 6 Gb/s. Disk discovery
reports `Samsung SS`. The SD boot partition mounts normally. This verifies
controller access and disk discovery, not Samsung filesystem mounting, writes,
or SSD boot. Existing JMB585 port-4 masking remains. The FAT-staged
`pioneer-update-8ecc61384c` package predates this fix and must not be used for
the Samsung update; stage the new validated package first.

Later on that same boot the user reported frozen input after the desktop
appeared. Serial confirms a kernel storePageFault on CPU 17, thread 249
`screensaver controller`, in `read_port_etc + 0x142`, runtime PC
`0xffffffc0021109da`, fault address `0xffffffc00a9fa7f8`. ELF offset `0x909da`
is `sd zero,8(s2)`, clearing a link while removing the head port message;
preceding instructions read the message links. This does not identify the
cause as screensaver or USB. Read-only KDL `port 167` reports the expected
port object `0xffffffc00a1675f8`, owner 211, capacity 200, read_count 1,
write_count 199, total_count 13. Further memory mapping/lifetime evidence is
needed before choosing a fix. Samsung package replacement has not occurred.
KDL `aspace 1` places the faulting address within slab area 10576, base
`0xffffffc00a800000`, size `0x800000`, protection `0x30` (kernel read/write).
Area-level permissions are not proof of the faulting page's PTE state or
message-object lifetime; inspect those before attributing a root cause.

Next diagnostic image: `haiku-pioneer-bfs-fatal-map.img`, SHA-256
`3e832234dcc852f5c50c61523c0a0d16f7a7dda0ccc2f4b4db20b3d779fc1acb`.
Kernel SHA-256:
`9be4abd7eb5e630d527c0022a0793120a4af0d2fa51517e9bebee19efb45a45a`.
P330 prints the translation-map query result (physical address and page
flags), area and per-page permissions, and cache type immediately before an
unhandled kernel page-fault panic. It does not dereference the faulting
message or change fault recovery. Both the built and package-staged kernel
contain the diagnostic strings. This is instrumentation, not a crash fix;
firmware, DT, CPU configuration and device drivers remain unchanged from the
SATA device-domain test. Samsung has not been updated.
SD deployment completed with a full 300 MiB readback matching the payload;
rollback backup stamp `20260906T223936Z`. Linux was shut down cleanly and
the next boot capture starts at serial byte offset 14783717.
That boot reached the kernel, mounted `/dev/disk/mmc/0/1` at about 94.2 s,
and mounted packagefs at about 98.5 s. Samsung disk discovery and its MBR
partition are visible, but the partition scan reports status 2, content size
0 and no recognized filesystem; it is not yet a validated Samsung BFS mount.
Desktop stability remains to be tested.

Follow-up: user confirmed responsive desktop/input. SSH required creating the
missing `sshd` service account; the bundled host-key fingerprint was verified
against the build before connection. From running Haiku, Samsung was already
mounted as BFS at `/Haiku1` (`/dev/disk/scsi/0/3/0/0`). The initial partition
scan above was therefore not its final recognition state. Three 4 KiB header
reads agreed, and the installed 36 MB system package matched the Linux backup
SHA-256 `6e0b8977344bf316f85d220c20ee190a83d644eb3f82e81b0dac75ce34cab848`.

With user approval, replaced only Samsung's
`system/packages/haiku-r1~beta6_hrev99999-1-riscv64.hpkg` with the package from
the running fatal-map SD image. Source, staged copy and installed file matched
SHA-256 `64f17196a418a9cc4efbcf6c9159e0d3528bf8427aa38e127c01208aa01f059f`;
`sync` completed before and after the replacement. Verified previous package
is retained at `/Haiku1/pioneer-update-fatal-map-20260906/previous-haiku.hpkg`,
outside the active package directory. Apps, settings, other packages, SD
firmware and Linux were not changed by this update. Samsung boot remains
untested; the machine is still running from SD. Do not confuse a successful
package update with successful SSD boot, or with a fix for the earlier panic.

Samsung boot-selection test (serial offset 15182077): after clean Haiku
shutdown and user confirmation of safe power-off, relay off/on with SD in.
A single serial Space after `[Bds]Stop Hotkey Service!` opened the loader
menu. Enter opened volume selection; two Down keys highlighted
`Haiku (111.79 GiB)`, Enter selected it, Escape returned to the main menu,
which confirmed that volume. Enter continued. No firmware or boot-order
configuration was written by these menu actions.

The Samsung kernel started but panicked in `vfs_mount_boot_file_system()`:
`did not find any boot partitions!`. At about 185.8 s, the final disk dump
DOES recognize Samsung's BFS partition: offset 32768, size 120033640448,
status 0, content name Haiku. The boot message has `user selected=true`,
partition offset 32768, hard-disk boot method 0 and a 79-byte disk identifier.
Nevertheless the boot candidate count is zero. This narrows the next check
to loader/kernel disk identity matching (size and sector checksums), not a
missing SATA driver or unrecognized BFS filesystem. Actual identifier values
and mismatched checksum/read status still need diagnostics; do not bypass
identity checks or assume a root cause. The board is halted in KDL.

Next build: `haiku-pioneer-bfs-boot-identity.img`, SHA-256
`e1e4958500e7dd7bdcb634ab9643a80d2e4d0652fd02a7b7b18573a0133d3824`.
Kernel SHA-256 `b383cd88e99dfdb932ddf346cf736e5196ebb687edf3abb48fa2774e1ea486ab`;
system package SHA-256
`2896076e4c97a1d58bdfbd79f30822e7b00308c03333e77de53107dcf520fc50`.
P331 traces at most 64 disk-identity comparisons, reporting actual/expected
size, checksum offsets, read return values and actual/expected sums. Rejects
malformed identifier lengths before accessing the structure. Existing valid
disk-matching rules are not relaxed. Both built and package-staged kernels
contain P331. Firmware/loader unchanged. After SD removal the board booted
Linux (capture offset 15404530); this diagnostic package still needs SD
deployment and then a verified Samsung package update from running Haiku
before another Samsung-selected boot can exercise it.
SD deployment was subsequently verified by complete 300 MiB readback;
rollback backup stamp `20260906T231730Z`, SD serial `0x0000e752`.
Clean Linux shutdown followed by relay on; normal SD boot capture starts at
15470203. Samsung still contains the fatal-map package, not P331. The fresh
SD image also still needs the `sshd` service account created before its
bundled SSH launcher can run; this account setup is not yet automated.

The P331 SD test passed all five disk-identity checks and mounted the SD boot
volume at about 96.1 s. After all 64 CPUs were enabled (about 110.7 s), it
panicked with interrupts disabled in `SVec + 0x08`, CPU 1, thread 124
`_power_daemon_event_loop_`. Fault address `0xffffffc0007a6000`, PC
`0xffffffc0021d4818`, SP `0xffffffc0007a5fc0`; disassembly of this kernel
identifies `sd tp,64(sp)` while saving a trap frame. KDL reports the thread's
kernel stack as `0xffffffc0007a2000` to `0xffffffc0007a7000`. This is not the
Samsung identity mismatch; mapping state and original nested-trap cause are
not yet known. No Samsung update occurred on that boot. Following user SD
removal, relay off/on began a Linux boot at serial offset 15665688.

Next SD image `haiku-pioneer-bfs-stack-pte.img`:
SHA-256 `9523e9602061de861d32f0d3983d42348388ad991c5f78eb0a140f1fa7a58c09`;
kernel `0f2475bb0a3cf5c2e1d8ce488c3ba8ccd01a4fa94c0c87598f100a9fc64c7daf`;
system package `deb5b0a46fdd837c4b9aa12506cb71f993186f0fcd0cef262ca96af2712afe61`.
P332 invokes the existing raw active-page-table dump immediately before an
interrupt-disabled page-fault panic; normal fault/mapping behavior is unchanged.
P331 disk-identity diagnostics remain. Compiled and package-staged kernels
contain P332. This is not a proven fix for either failure.

`pioneer_start_sshd.sh` is the tracked source for the image's
`generated.pioneer/ssh/start-sshd.sh` asset (copy it there before building).
It creates the missing non-login sshd account when absent, refuses UID 0 for
that account, and retains the existing key-based SSH configuration. Shell
syntax checked; automatic startup still requires runtime verification.
SD serial `0x0000e752` deployment completed with full readback verification;
rollback backup stamp `20260906T235019Z`. Clean Linux shutdown, 20-second
wait, relay on; next normal SD boot capture starts at 15731039.
That SD boot reached the desktop (user confirmed), passed all five P331 SD
identity checks, and allowed automatic key-authenticated SSH. `sshd` account
was created as UID 1000/GID 101. No panic was present in the inspected log.
Samsung was not auto-mounted; its device size and BFS header were verified,
then `/dev/disk/scsi/0/3/0/0` was mounted explicitly at `/Haiku1`.

Samsung's installed fatal-map system package matched expected SHA-256
`64f17196a418a9cc4efbcf6c9159e0d3528bf8427aa38e127c01208aa01f059f`.
Backed it up to
`/Haiku1/pioneer-update-stack-pte-20260906/previous-haiku.hpkg`, verified the
backup, and replaced only the system package with the running SD package
`deb5b0a46fdd837c4b9aa12506cb71f993186f0fcd0cef262ca96af2712afe61`.
Staging and final hashes matched, with sync before/after replacement. Samsung
now has both P331 and P332; its next selected boot is still untested. No
other installed packages or settings were replaced. The SSH image launcher
change is outside the system package and was not copied to Samsung.

Samsung P331/P332 boot test, serial offset 16133110: clean shutdown confirmed
safe by user, relay off/on, single Space at firmware handoff, then selected
`Haiku (111.79 GiB)` and confirmed selection on main menu before continuing.
Kernel again panicked `did not find any boot partitions!` (about 176.8 s).
P331 identifies the reason: Samsung device 2 size 120034123776 matches the
loader's expected size, but checksum 0 at offset 0 (512 bytes read) is
`0xb2ec35bc`, versus loader identifier `0x83a217d7`. This repeats in strict
and non-strict passes. The SD's distinct checksum is `0x50334973`. Next
compare the actual 512-byte Samsung sector under Linux against both read
paths; do not weaken identity matching. No Samsung filesystem was mounted
as boot, and the board is halted in KDL. This test does not establish which
read path is incorrect or resolve the separate intermittent stack fault.

Linux comparison after user SD removal (boot serial offset 16368183):
identified `/dev/sda` as Samsung SSD 840 Series, serial `S14CNSAD300075H`,
120034123776 bytes. Read sector zero without mounting or writing Samsung.
Summing its 128 little-endian uint32 words modulo 2^32 gives `0x83a217d7`,
matching the loader, not Haiku kernel `0xb2ec35bc`. A second read using
`dd iflag=direct` produced identical bytes and checksum. Thus the evidence
points to the Haiku kernel read path; the particular AHCI/DMA/copy layer is
not established yet. Next instrument sector-zero data at the AHCI bounce
buffer and destination to distinguish transfer data from copy/mapping errors.
Do not bypass disk identity checking. No disk writes occurred in this check.

Next SD test `haiku-pioneer-bfs-ahci-sector.img`, SHA-256
`206c9a80d1458cce570eb8d3bab3f8fa57f4efe79d697ac402542f563c4fb02e`;
system package `a3e97847d7245ca747c6dbb595480499d876a64fbf568c3043582936a7cc1464`.
Kernel and EFI loader unchanged from P332. AHCI P333 logs at most 16
completed read-DMA requests starting at LBA 0, comparing the first 512 bytes
of the bounce buffer with the destination via the physical scatter/gather
copy helper. It reports both checksums, destination-read status and first
mismatch offset (512 means identical). No transfer or identity-matching rule
is changed. The packaged AHCI binary contains P333. An ordinary SD boot can
exercise this probe during Samsung discovery; Samsung is not yet updated
with this package.
Deployment to SD serial `0x0000e752` passed complete readback; rollback backup
stamp `20260907T001055Z`. Linux clean shutdown and 20-second wait preceded
relay on. Normal SD boot capture starts at 16436303.
User confirmed desktop on that boot; SSH works and no panic appeared in the
inspected log. During early discovery P333 repeatedly reports LBA0 bounce
and destination checksum `0xd541b20c`, with all 512 bytes identical between
them. After desktop startup a read-only SSH `dd` of Samsung raw sector zero
returned exactly the 128 uint32 words previously read under Linux, checksum
`0x83a217d7`; the corresponding P333 entry likewise reports that correct
checksum for both bounce and destination. Thus early wrong data is already
in the bounce buffer, and later reads can succeed without a disk change.
The probe does not yet distinguish DMA coherency, command completion, or
another early initialization issue. Samsung has not received P333 and has
not been modified during this diagnostic session.

Live follow-up: built `pioneer_sector_probe` (SimpleTest target) and copied
only that test executable to Haiku `/tmp`. It opens the specified disk
O_RDONLY and requests affinity for its own thread on CPUs 0,1,2,16,32,63,0,
reading one sector per CPU. No sector result returned: while the test was
running, kernel panicked at scheduler.cpp:503,
`nextThreadData->Core() == core`, CPU 31, thread 277 `w>Desktop` in reschedule
while waiting for a port message. This does not establish CPU-dependent disk
data, nor prove affinity caused the assertion. Do not repeat the probe until
the scheduler issue is understood. No CPUs were explicitly disabled, and no
Samsung writes occurred. The running image/kernel was not replaced. Board
is halted in KDL; preserve both the scheduler and earlier P333 evidence.

P334 follow-up (2026-09-06): after returning to Linux and confirming SD
serial `0x0000e752`, add a bounded AHCI LBA0 cache-visibility diagnostic.
For the first 16 sector-zero reads, on detected T-Head MAEE hardware only,
compare the bounce-buffer checksum before and after physical cache-line
invalidation of its first 512 bytes. The dedicated allocation is page aligned;
the completed request still owns it. Interrupts are disabled during comparison
to avoid migration. No dirty cache data is cleaned back over device output.
The physical IPA and SYNC.S encodings match Linux's T-Head cache operations
(`arch/riscv/errata/thead/errata.c`). The kernel already exports the detection
flag; verified the built driver loads physical addresses into a0 for IPA.
This runs **after** the normal destination copy, so it does not repair the
current read: a changed bounce checksum and P333 destination mismatch would
be diagnostic evidence, not a new copy failure. No command, identity policy,
CPU count, firmware, or Samsung package changes. Do not repeat the affinity
probe. Test via the normal SD boot before attempting another Samsung boot.

P334 built and SD full-readback verified: payload
`haiku-pioneer-bfs-ahci-cache-probe.img`, SHA-256
`d29b6642a0419932763dee88716ac529145ccff4c40e737b64553d118c5f6eb3`;
package `b0e3fbdeebc3433b35624603ae8c2c3f9535f4a8e6bb378a9f5e57bc3c426308`.
Kernel remains `0f2475bb0a3cf5c2e1d8ce488c3ba8ccd01a4fa94c0c87598f100a9fc64c7daf`.
Rollback stamp `20260907T003612Z`. Linux shut down via `shutdown -h now`,
waited 20 seconds, then relay ON only. Serial boot offset 16780414.

P334 result: SD mounted at 96.58s; Haiku SSH works. Samsung raw device
published, but initial device scan returned `Bad data` without issuing a
traced LBA0 read. No early P334 measurement was obtained. A later SSH read
returned the correct sector; P334 reports physical `0x27b8000`, checksum
`0x83a217d7 -> 0x83a217d7`, P333 destination identical. That read used command
0x25 (48-bit), whereas P333's earlier normal Samsung reads used 0xc8 (28-bit).
Inspect the IDENTIFY/capacity initialization path next; this test does not
establish that cache invalidation fixes the early failure. No disk writes or
affinity changes were performed in the running Haiku. Board left in Haiku.

After user confirmed the desktop is up, read-only `driveinfo` on Samsung raw
reports **size 0 bytes**, bytes_per_sector 512, sectors_per_track 0,
cylinder_count/head_count 1, media status `No error`. Only SD is mounted.
This directly confirms incorrect exposed capacity, despite successful later
sector-zero reads. Instrument IDENTIFY and synthesized READ CAPACITY results
next; do not treat this as a missing partition or reformat the disk.

P335 preparation: add AHCI IDENTIFY diagnostics at DMA completion (status,
task-file status, PRDBC, requested length, checksum and parsed sector data),
and at the inquiry destination after completion. Log synthesized READ CAPACITY
10/16 sector count, block size and last LBA before the scatter/gather copy.
IDENTIFY data traces are capped at 24 and capacity traces at 16 per variant.
This is logging only; sector parsing, command selection, DMA handling and
the prior P334 diagnostic are unchanged. No Samsung package update planned
for this test: the normal SD boot exercises Samsung discovery.

P335 build passed (972 targets); verified P335 strings in both compiled and
packaged AHCI. Payload `haiku-pioneer-bfs-ahci-identify-probe.img`, SHA-256
`bb0552346d7db79109b174e42b9ef0f5c8adc779daea11fa5767a12bc9bd1cd3`.
Kernel and EFI loader hashes unchanged from P334. Not deployed yet; the
P334 Haiku instance remains running pending the next clean shutdown cycle.

Attempted P335 deployment through running Haiku at user's request. /boot had
36.1 MiB free; new hpkg is 34,788,779 bytes. Saved and hash-verified old P334
package on Mac at `/private/tmp/pioneer-haiku-p335-rollback/previous-haiku.hpkg`
(SHA b0e3fbdeebc3433b35624603ae8c2c3f9535f4a8e6bb378a9f5e57bc3c426308).
Created `/boot/pioneer-update-p335`; hard-link rollback attempt was rejected
with Operation not allowed. Transfer to `new-haiku.hpkg` subsequently timed
out. **No publication/active-package replacement command was issued.**
An incomplete staging file may remain and must be checked/removed on recovery.
Serial shows old P334 kernel panic in thread 196 `/dev/net/rtl8125/1 consumer`,
CPU19, list_remove_item+0x0e, PC ffffffc0021b040e, store fault
ffffffc01c0740a0. P330 query: VA ffffffc01c074000 PA 7ca1f000 flags7030,
area13310 slab baseffffffc01c000000 size800000 protection30, cachetype4.
Do not attribute this to unactivated P335. Board halted in KDL; Samsung
contents and active SD system package unchanged by this deployment attempt.

P335 subsequently deployed through Linux after user removed/reinserted SD.
Verified serial 0x0000e752, staged on /mnt/ssd (sdb2), rollback image stamp
`20260907T013952Z`. Complete 300 MiB readback matches
`bb0552346d7db79109b174e42b9ef0f5c8adc779daea11fa5767a12bc9bd1cd3`.
This replaces the SD BFS filesystem, including any incomplete SSH staging
file; the backup preserves its previous contents. Firmware/Samsung unchanged.
Linux clean shutdown, 20-second wait, relay ON. Serial boot offset 17102187.

P335 result: IDENTIFY completion status0 TFD50 PRDBC512, both bounce and
destination sum2162b018, 234441648 sectors, logical/physical512, use48=true.
CAPACITY16 correctly emits lastLBA0xdf94baf, block512, copy32. First LBA0
read provides direct cache-visibility evidence: P334 physical0x27b8000
checksum **0x84bc582c -> 0x83a217d7** after IPA/SYNC.S. Already-copied
destination retains bad84bc582c (first mismatch0); subsequent reads match
83a217d7, the Linux/loader checksum. This diagnostic is still after-copy,
not a complete DMA ownership fix. SD boot mounted97.66s. Haiku SSH works;
read-only driveinfo now reports correct120034123776-byte Samsung capacity.
The zero-capacity failure did not recur on this boot; don't claim its exact
cause independently established. Next implement proper cache ownership
handling, including allocation memory-type transition and command metadata,
then retest rather than bypassing disk identity or changing partitions.

P336 candidate: AHCI-local T-Head-gated physical cache synchronization helper
uses clean+invalidate before DMA ownership transfer, invalidate-only after
completion, and SYNC.S/fence ordering. Retire cached create_area initialization
before switching DMA allocations to non-cacheable memory. Publish initialized
metadata before port enable; before each doorbell publish bounce data, slot0
command header and command table/active PRDs. Do not clean controller-owned
received FIS memory while the port is running. After successful completion,
invalidate command header before PRDBC and read payload before any copy or
IDENTIFY interpretation. Check RISC-V PRD construction success before using
its count. Existing P281 length workaround and P333/P334/P335 diagnostics
remain for comparison; P334 should no longer change the first-sector checksum.
No kernel/firmware/CPU-count/Samsung modifications. Runtime validation pending.

P336 final build passed including PRD guard. Payload
`haiku-pioneer-bfs-ahci-dma-sync.img`, SHA-256
`c6818774a87dc636a1859b289df0f2b7c9af6e785e3103421c53a13728030678`.
Verified packaged P336 marker and physical cache-op/a0 encodings in compiled
driver; checked cache-line coverage arithmetic for 704 offset/length cases.
Kernel/loader hashes remain unchanged. Not deployed; board left running P335.

P336 deployed through Linux following user-confirmed clean Haiku shutdown
and SD removal/insertion. Verified SD serial0x0000e752; rollback stamp
`20260907T023027Z`. Full 300MiB readback matches
`c6818774a87dc636a1859b289df0f2b7c9af6e785e3103421c53a13728030678`.
Linux shutdown -h now, waited20s, relay ON. Serial boot offset17433730.
Firmware and Samsung unchanged.

P336 boot result at offset17433730: sync enabled=1 on ports0-3. First LBA0
already has correct83a217d7 before P334's extra invalidation; afterward
unchanged, and P333 destination identical. All five early traced reads match.
IDENTIFY now reports word0=0x40, checksum47290ea7, 234441648 sectors,
512-byte logical/physical sectors; bounce/destination agree. SD mounted94.80s;
64 CPUs enabled109.51s. SSH driveinfo confirms Samsung120034123776 bytes
and partition discovery. No P281 warning, PANIC or ASSERT in inspected boot.
This validates the first-read regression on this SD boot, not Samsung boot
or sustained disk writes. Samsung installed package remains unchanged.

User authorized Samsung update for SSD boot test. Under running P336 Haiku,
mounted /dev/disk/scsi/0/3/0/0 at /Haiku1 (111.8GiB BFS), verified old package
deb5b0a46fdd837c4b9aa12506cb71f993186f0fcd0cef262ca96af2712afe61.
Copied that to /Haiku1/pioneer-update-dma-sync-20260907/previous-haiku.hpkg
and verified it. Staged P336 directly from /boot/system/packages (no large
network transfer), verified SHA1160580d83f0c535a803b3b7965327725a5c711ff7158caa2dc233dda151dc46,
then same-filesystem mv to Samsung's existing
system/packages/haiku-r1~beta6_hrev99999-1-riscv64.hpkg. Sync and final hash
checks of installed and rollback copies both passed. Apps/settings/partitions
and SD package unchanged. Board remains running SD Haiku; Samsung mounted.
Next: clean shutdown, user confirms safe, off/on with SD LEFT INSERTED,
select Samsung111.79GiB through the existing serial boot menu procedure.

P336 Samsung boot test: user confirmed clean shutdown safe; relay off/on,
SD left inserted. Serial offset17874825. One Space at firmware handoff,
selected Haiku111.79GiB, confirmed main menu, continued. Kernel P336 active;
all five P331 disk-identity checksums MATCH (83a217d7, b9226b3b, 211d8c3e,
2551e54e, 5e896c45). **Mounted boot partition /dev/disk/scsi/0/3/0/0** at
193394455us (includes time spent selecting the menu). This confirms Samsung
kernel boot-volume discovery/mount, not yet full desktop startup. System
package loading proceeds from Samsung; no SPI or Linux boot changes made.

SSD-boot checkpoint: user subsequently confirmed a running Haiku desktop.
The kernel mounted the Samsung boot partition and enabled 64 CPUs. Boot still
requires the SD firmware/loader and manual serial selection of Samsung;
default SD boot and Linux-with-SD-removed workflow are unchanged. Persistent
apps/settings reside on Samsung, but a desktop-file persistence reboot test
has not yet been performed. Samsung SSH was not responding during the initial
check. The earlier Ethernet-consumer page fault and affinity-probe scheduler
assertion remain unresolved; avoid treating this checkpoint as stress-tested
or upstream-ready. Keep diagnostics and rollback packages for further work.

Persistence retest: user saved a desktop file and custom screen resolution,
then confirmed clean shutdown safe. Relay off/on with SD inserted; serial
offset18657323, selected Samsung111.79GiB again. All five disk identity
checksums match; kernel mounted /dev/disk/scsi/0/3/0/0 at171053577us
(includes menu wait). No package/config changes in this reboot. Await user
confirmation that desktop file and resolution survived.

Persistence reboot halted before desktop: P332 interrupt-disabled store page
fault, CPU46 thread121 launch_daemon, VAffffffc0020048a8,
PCffffffc002134f96. Correct ELF load bias is ffffffc002080000 (verified using
arch_thread_entry); ELF offset b4f96 resolves to _mutex_unlock, instruction
sd a3,16(a4), updating a mutex waiter link. Caller MemoryManager::_MapChunk
during slab allocation / VMCache creation / elf_load_user_image. Not the
nearest-symbol event_queue name printed by KDL. PTE snapshot MAEE1,
SATP8000000000005606, L2=700000002f635801, L1=700000002b5f0821,
L0=70000000015b7ce7 (valid/read/write/accessed/dirty present in snapshot).
Underlying mapping/coherency or waiter-lifetime cause not established.
Samsung identity and boot mount succeeded first; persistence of desktop file
and resolution remains unverified. Board left halted in KDL, no recovery
power operation or new kernel changes made during diagnosis.

Unchanged P336 SSD retry, user authorized, serial offset19077309: Samsung
identity checks pass and boot mount succeeds165579707us; secondary CPUs
enabled186182303us. Panics again, CPU8 thread88 scsi scheduler2, PC
ffffffc002135042 (ELF b5042: _mutex_lock), store atffffffc002388330,
instruction sd a4,8(a3) appending a mutex waiter. Stack is dprintf_args ->
dprintf -> AHCI sg_memcpy -> ExecuteSataRequest -> SCSI scheduler. P332
L0=700000000391cce7, L1=700000002b5f0c21, L2=700000002f635821,
SATP80000000000bd8d7. Like prior fault, snapshot shows valid writable A/D
mapping. Both failures concern mutex waiter-list writes, but different locks,
threads and addresses; shared underlying cause not yet proven. No build or
disk changes on retry; board left halted in KDL. Next investigate waiter
lifetime and cross-CPU stack mapping/TLB visibility, not disk identity bypass.

P337 candidate after recovery to Linux (SD serial0x0000e752 verified):
Map() already marks T-Head pages accessed, but leaves new stack PTEs dirty=0
after bootstrap. Set dirty=1 specifically for kernel B_KERNEL_STACK_AREA
mappings on detected T-Head MAEE. Wired stacks contain mutex waiter records
that other CPUs write with interrupts disabled; avoid needing first-write
dirty-bit faults there. Preserve user/pageable dirty tracking and existing
synchronous stack TLB flush. This is a narrow hypothesis test: panic-time
PTE snapshots were already dirty, so it does NOT prove the original cause;
stale translation state and waiter lifetime still need consideration if it
recurs. No CPU-count/device mapping/AHCI/firmware changes. Build pending.

P337 build/deploy passed1124 targets. Built kernel SHA
c0c72b3dc61629a86ce38a88530cacb55c04378536fefd409ce9932c2ae57c9f;
packaged kernel61045b6d7313d4d57192f3274864ff4842a6a6a1211e664e72543685e1c3c7af
(stripped; .text extracted and byte-compared equal to build).
Package7dd3646661bceaec5992858b9cf8871e2a6b29b2f6899395c8d605535bc378e8.
SD payload haiku-pioneer-bfs-stack-dirty.img full readback SHA
1bb85fa481389e436bc83eb8dc5126ee8d98290ac941016b81fd58b9bd231014,
rollback stamp20260907T050429Z. Linux clean shutdown,20s wait,relayON;
normal SD boot offset19598832. Samsung remains on P336 until safely updated.

P337 startup not confirmed: power-on returned success, but serial log stayed
at19598832 bytes through two20s waits. Serial device and existing screen
session39215 remain present. No firmware/kernel output from this attempt;
do not classify this as a candidate-kernel failure. Await physical board state.

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
