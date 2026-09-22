# Tenstorrent P100A: RISC-V Haiku bring-up

## Status

Source/feasibility investigation started 2026-09-15. No functioning Haiku
accelerator driver yet. No kernel, firmware, PCI configuration or disk changes
were deployed for this investigation. The running Haiku `listdev` inventory
does not show vendor 0x1e52 / device 0xb140. Physical installation is not yet
confirmed; absence from this inventory alone does not explain why.

Download pinned official sources with:

```sh
sh tools/pioneer/tenstorrent_fetch.sh
```

Default location is ignored `generated.pioneer/tenstorrent/`. Existing trees
are never overwritten. No upstream build/install scripts are executed.

| Component | Official source | Investigated revision | License notice |
|---|---|---|---|
| Kernel driver | https://github.com/tenstorrent/tt-kmd | 22cca4cb33455d962cf8340a8f5b896dae696fd7 | GPL-2.0-only; ioctl header has Linux-syscall-note |
| User-mode driver | https://github.com/tenstorrent/tt-umd | 5c9ea40f8e29f84c32dde2646f93e19e1d13dd9a | Apache-2.0 |

Keep original notices and provenance. Do not relabel copied GPL driver code
as MIT. Packaging/upstream acceptance needs an explicit licensing review.
These pins are investigation snapshots, not a validated mutually compatible
release pair or a firmware recommendation.

## Findings from source

- P100A belongs to Blackhole. KMD `enumerate.h` identifies Blackhole as
  1e52:b140; this ID alone does not identify the precise board SKU.
- KMD `blackhole.c` maps portions of BAR0 and BAR2. BAR4 length determines
  the exposed count of 4-GiB TLB windows. Do not assume a particular BAR size
  or that a 32-bit `pci_info` field can describe the complete aperture.
- KMD `memory.c` uses coherent DMA allocation, pinned user pages, scatter/
  gather mappings and IOMMU handling. Blackhole's current DMA limit is 58 bits;
  that is not proof the Pioneer host can DMA to every physical RAM address.
- KMD `interrupt.c` requests one PCI interrupt with Linux IRQ allocation.
  Haiku has MSI/MSI-X interfaces, but the P100A path must be verified.
- UMD `tt-kmd-lib/src/tt_kmd_lib.c` uses Linux ioctls plus file-backed mmap
  offsets for device/TLB/DMA memory. Implement a Haiku transport; do not assume
  a Linux ioctl number or mmap offset works with Haiku's driver ABI.
- UMD `device/api/umd/device/driver_atomics.hpp` already includes RISC-V
  fences. This is a head start, not evidence that the whole runtime supports
  RISC-V Haiku. Audit discovery, mapping, dependencies and cache behavior.
- UMD's README describes IOMMU/hugepage requirements for host-device memory.
  An initial bounded DMA allocation is more realistic than promising general
  Linux hugepage/pinning compatibility on Haiku.

## Existing Haiku foundations and risks

`src/add-ons/kernel/busses/pci/ecam/ECAMPCIControllerFDT.cpp` already has
SG2042 PCIe translation and MSI support. Preserve this working infrastructure;
change it only when a captured P100A failure demonstrates a missing capability.

The current NVMe driver explicitly uses non-cacheable DMA bounce buffers on
SG2042 because ordinary cached payload memory is not coherent in this port.
This is a central accelerator risk: CPU fences alone do not flush/invalidate
non-coherent caches. Initially use small, explicitly managed DMA buffers with
validated bus addresses, ownership transitions and lifetime rules.

BAR addresses are PCI bus addresses, not automatically CPU physical addresses.
Validate translated ranges, 64-bit BAR pairing, full widths, mapping attributes
and bridge apertures before touching registers. Avoid destructive BAR sizing
probes against a live device. PCI enumeration is not accelerator readiness.

## Incremental implementation plan

1. **Inventory:** install with the machine fully powered down; capture Linux
   PCI identity/BDF, assigned BAR resources, link state and firmware/driver
   versions. Confirm Haiku enumerates the same endpoint. Do not flash firmware.
2. **Native diagnostic driver:** bind only the Blackhole device, expose a
   versioned read-only identity/resource ioctl, and fail closed on unsupported
   operations. No DMA, resets, arbitrary physical mapping or user register
   writes. Build this as an optional add-on, not a mandatory boot dependency.
3. **Controlled device access:** validate BAR mappings, port the minimum ARC
   firmware/message and TLB access required for a bounded identity/telemetry
   query. Register reads are not all side-effect-free; audit each access.
4. **DMA/interrupt smoke tests:** a small buffer round-trip with known patterns,
   guards, timeout, cleanup and repeated tests; then high RAM, multiple CPUs,
   cancellation and process-exit cases. Enable bus mastering only after DMA
   mappings and teardown are correct. No arbitrary user-page DMA initially.
5. **UMD transport:** substitute Haiku discovery/ioctls/areas and implement only
   verified operations. Unsupported Linux-only features return errors, not
   dummy success. Validate runtime/firmware compatibility before compute.
6. **Compute:** minimal supported runtime operation, then matrix multiply,
   then an LLM integration and performance tests. A PCI/kernel driver by itself
   does not make the existing CPU llama.cpp build use the card; it needs a
   compatible accelerator backend/runtime path too.

Hardware-dependent steps cannot be marked complete from compilation alone.
Preserve the working SD/Samsung CPU-only configuration as a recovery path.

## Fedora hardware inventory, 2026-09-21

With the P100A in the bottom PCIe slot and a native 12+4-pin PSU cable, the
first boot did not enumerate the card. After a clean shutdown and physical
reseat, Fedora 38 (Linux 6.1.31, riscv64) enumerated `0002:81:00.0` as
`1e52:b140`, class `1200` (processing accelerator). The root port is
`0002:80:00.0`. The link trained at PCIe 2.5 GT/s x8; this is below the
endpoint's advertised 32 GT/s x16 capability and is not a performance result.

No Tenstorrent module, device node, or `tt-smi` installation was present.
Linux failed to assign all three memory BARs: BAR0 512 MiB, BAR2 1 MiB,
and BAR4 32 GiB. It also failed to allocate the root-port's requested
48 GiB prefetchable bridge window. The root bus advertises a high
prefetchable window at `0x4900000000..0x4affffffff` (8 GiB), plus a
768 MiB lower prefetchable window. Its non-prefetchable windows cannot
accommodate the 32 GiB prefetchable BAR. `lspci` shows the bridge memory
windows disabled and the endpoint resources unassigned. The other Pioneer
root buses likewise advertise only 8 GiB high prefetchable windows in this
Fedora boot; moving slots alone is not an established fix.

The card's usable memory capacity is distinct from the size of
its BAR4 PCI address aperture. Do not treat the user's 28 GB memory figure
as a contradiction of the measured 32 GiB BAR. The captured allocation
failure is sufficient to stop driver/LLM installation attempts for now.
The SG2042 manual and Tenstorrent BAR4 discussion below narrow the possible
workaround; a larger device-tree window is not the solution.
Do not write live PCI configuration, flash firmware, or replace the working
boot setup as a diagnostic shortcut. A future aperture change needs a
recovery plan and separate validation. No Linux/Haiku PCI configuration,
driver, firmware, or storage was changed during this inventory.

### Read-only aperture feasibility check

The active device tree's `pcie@7062000000` ranges match the kernel source in
`arch/riscv/boot/dts/sophgo/mango-pcie-4rc.dtsi`: 768 MiB low prefetchable,
256 MiB non-prefetchable, 8 GiB high prefetchable at `0x4900000000`, and
4 GiB high non-prefetchable at `0x4b00000000`. `/proc/iomem` confirms the
adjacent `0x4c00000000..0x4fffffffff` address space belongs to the next
PCIe controller, while `0x4800000000..0x48ffffffff` contains this root's
configuration and smaller windows. Thus the 32-GiB-aligned interval
`0x4800000000..0x4fffffffff` cannot simply be assigned to BAR4.

The Sophgo Cadence host driver programs an outbound translation region for
each device-tree range. Its CPU-address fixup masks addresses with
`0xCFFFFFFFFF`; for example, the next 32-GiB-aligned candidate at
`0x5000000000` would become `0x4000000000` in the programmed translation
register. The controller's wider address decode and a non-overlapping host
aperture have **not** been established, so changing only the device-tree
window would be an unsafe experiment. This read-only check made no changes
to the Fedora boot or PCI configuration.

The local Tenstorrent KMD source derives its number of BAR4 TLB windows from
the exposed BAR length, but the UMD and compute stack still need validation
without BAR4. Even after BAR0/BAR2 allocation works, host-side TT-Metal
support on riscv64 remains a separate porting question.

### SG2042 physical limit and BAR4-free path

The [SG2042 Technical Reference Manual, Chapter 2, Table 1](https://github.com/milkv-pioneer/pioneer-files/blob/main/hardware/SG2042-TRM.pdf)
assigns only **16 GiB** of CPU physical address space to each PCIe link:
`0x4000000000..0x43ffffffff`, `0x4400000000..0x47ffffffff`,
`0x4800000000..0x4bffffffff`, and `0x4c00000000..0x4fffffffff`.
The P100A is on PCIe1 link 0 (`0x4800000000..0x4bffffffff`). Its 32 GiB
BAR4 cannot fit inside that link's documented physical aperture, regardless
of how the current 8 GiB device-tree subwindow is enlarged. Do **not** make
a 32 GiB device-tree `ranges` change or borrow space from the neighboring
link. The live `lspci -vv` capability list also shows no PCI Resizable BAR
capability; a standard BAR resize is not available in this observation.

There is, however, a credible BAR4-free path. A [Tenstorrent engineer's
August 2026 Linux PCI patch](https://lists.openwall.net/linux-kernel/2026/08/24/1853)
targets Blackhole device `1e52:b140`, exactly this P100A's PCI ID. It
proposes omitting BAR4 from Linux resource assignment when the host aperture
is too small, allowing BAR0 and BAR2 to fit. The author reports that this
made `tt-smi` and `tt-bh-linux` work on a 4 GiB-aperture RISC-V host.
That is evidence of basic card access without BAR4, **not** evidence that
TT-Metal or LLM inference works without it.

The patch is not safe to copy uncritically: [Linux PCI review](https://lkml.iu.edu/2608.3/06163.html)
points out that zeroing Linux's BAR4 resource metadata does not disable
the BAR in the device. Enabling memory decoding could leave a live,
unassigned BAR that decodes an unintended PCI address. Do not use this
unmodified PCI quirk on the Pioneer. The firmware-side sizing mechanism
below is preferable once the card can safely be reached for flashing.

On the live Fedora boot, read-only `setpci` shows PCI COMMAND `0x0000`
(memory decoding and bus mastering both off), while BAR4's low/high config
dwords are `0x0000000c`/`0x00000000` (an unassigned 64-bit BAR). This is
safe in the current state, but it reinforces the reviewer's concern:
turning on memory decoding after merely hiding BAR4 from Linux would leave
the hardware BAR pointed at bus address zero. No PCI config register was
written during this check.

### Firmware-side BAR4 resize: offline dry run

Tenstorrent's public [system-firmware source](https://github.com/tenstorrent/tt-system-firmware)
at `04f0df6ebfbd016d98189e8131b16af17b48638b` exposes
`pci0_property_table.pcie_bar4_size` in MiB. The P100A table defaults to
`32768`; `lib/tenstorrent/bh_arc/pcie.c` accepts power-of-two values and
explicitly handles `0` as “BAR4 Disabled.” The project's own
`scripts/update_bar4_size.py` edits that field in a firmware bundle and
states that a **cold reboot** is required for the change to take effect.
The `P100A-1` board in the bundle matches this card's `0x0043` subsystem
device family, but its exact installed firmware version remains unknown.

An offline dry run used the official `v19.15.0` firmware bundle, whose
downloaded SHA-256 matched the published
`c1a317f9658435a7f2e8ab1e18a9fe942cd36334d8b88f01b777c8de975b2aef`.
The official script changed only `P100A-1` PCI bus 0 to BAR4 size `4096`
MiB, and its post-write verification passed. `tt_fwbundle.py diff` reported
only the P100A-1 `cmfwcfg` entry changed, from CRC `8cc1b6a1` to
`34a5fa6f`. The resulting **unflashed, experimental** bundle is at
`/private/tmp/fw_pack-19.15.0-p100a-bar4-4096-test.fwbundle` with SHA-256
`6dda5a74404672a194bf472a2c816e75b043dda015307d6b6774c6e4df61d602`.
Do not copy this firmware bundle to the public project repository or flash
it without determining the card's installed firmware version, taking a
recoverable backup, verifying board compatibility, and establishing a way
to access the card with BAR0/BAR2 despite today's failed allocation.

Why 4 GiB: one BAR4 4-GiB TLB window plus BAR0 (512 MiB) and BAR2 (1 MiB)
should, in principle, fit the root's 8 GiB prefetchable window. Linux bridge
allocation and Tenstorrent userspace behavior with only one large window
have **not** been tested. Setting BAR4 to zero is another documented firmware
option for basic access, but its effect on TT-Metal/LLM use is even less
certain. No card firmware, kernel, boot configuration, or PCI register was
changed during this work.

Next gate: find a safe one-time access path (for example a compatible host)
and a firmware backup/recovery procedure, then select a matching firmware
release and test a 4 GiB BAR4 configuration under supervision. Do not
reboot or flash this Pioneer unattended: a failed card flash or boot test
could need physical recovery. First verify PCI resource assignment and
`tt-smi`; model inference is a later milestone.

## Official references

- P100A hardware: https://docs.tenstorrent.com/aibs/blackhole/installation.html
- Kernel driver: https://github.com/tenstorrent/tt-kmd
- User driver and host-memory requirements: https://github.com/tenstorrent/tt-umd
- Compute runtime: https://github.com/tenstorrent/tt-metal
