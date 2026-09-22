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
Next investigate SG2042 outbound address-map and device-tree/firmware limits
offline, and determine whether a supported smaller BAR4 configuration exists.
Do not write live PCI configuration, flash firmware, or replace the working
boot setup as a diagnostic shortcut. A future aperture change needs a
recovery plan and separate validation. No Linux/Haiku PCI configuration,
driver, firmware, or storage was changed during this inventory.

## Official references

- P100A hardware: https://docs.tenstorrent.com/aibs/blackhole/installation.html
- Kernel driver: https://github.com/tenstorrent/tt-kmd
- User driver and host-memory requirements: https://github.com/tenstorrent/tt-umd
- Compute runtime: https://github.com/tenstorrent/tt-metal
