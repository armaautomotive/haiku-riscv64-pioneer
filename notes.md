# Milk-V Pioneer (SG2042) Haiku RISC-V port notes

Last updated: 2026-08-16

## Goal and current status

Target: Milk-V Pioneer / Sophgo SG2042, with 64 Xuantie C920 RISC-V cores.

Haiku does **not** boot fully yet, but the port now passes the firmware, loader,
page-table handoff, and early kernel-entry stages. The active SD firmware boots
the vendor EDK2 environment, starts Haiku's embedded EFI loader, loads the BFS
kernel, calls `ExitBootServices`, enables Sv39, and reaches deep into kernel VM
initialization on the primary CPU.

The latest boot stopped after:

```text
riscv: reserve loader ranges
riscv: reserve loader deferred
```

Disassembly showed that the next operation used an early PLT entry for
`arch_vm_translation_map_init_post_area()`. The function's own first diagnostic
never appeared, indicating that control did not reach its body. A kernel-only
fix now invokes that function and the following three post-area initialization
functions through direct linked addresses. It has built and passed disassembly
verification, but **has not yet been copied to the SD card or boot-tested**.

## Exact next step

1. Power off the Pioneer and remove the SD card.
2. Boot Fedora Linux, then insert the SD card.
3. Mount/inspect the BFS partition (`/dev/mmcblk1p2`, label `Haiku`), back up its
   existing `kernel_riscv64`, and replace it with the newly built kernel below.
4. Verify the copied file's SHA-256 hash, unmount/sync the card, power off, and
   cold-boot with the card inserted.
5. Capture serial output and look for the new `tmap post start/done` sequence.

New kernel to install:

```text
/Volumes/HaikuBuild/generated.riscv64/objects/haiku/riscv64/release/system/kernel/kernel_riscv64
size:   2192121 bytes
sha256: 2350b9024dea3c1b777d0447a5cc2b2328a42cf700241e84b28c8663d0385a6b
```

This is a kernel-only update. Leave the currently working firmware on the FAT
partition unchanged.

## Safety and boot rules

- Do not write the Pioneer SPI flash / MTD devices.
- Do not write the Fedora NVMe or the user's SATA drive.
- Use only the removable SD card's dedicated test partitions:
  - `/dev/mmcblk1p1`: 1 GiB FAT32, label `HAIKUPNR`, SD-only firmware files.
  - `/dev/mmcblk1p2`: 1 GiB BFS, label `Haiku`, Haiku test volume.
- A normal Fedora reboot uses kexec and bypasses SD firmware. Haiku tests need a
  full power-off, a wait of about 10 seconds, then power-on with the SD inserted.
- To recover from a hung Haiku test: power off, remove the SD, then boot Fedora.
- The machine has intermittent cold-power behavior. Avoid unnecessary power
  cycles and always allow the PSU to discharge before another attempt.
- The Pioneer currently depends on the SATA drive being connected for its
  normal Linux boot configuration.

## Access and serial

Fedora SSH:

```text
host: 10.0.0.247
user: haikuport
key:  /Users/arma/.ssh/haiku_pioneer_ed25519
```

Serial console:

```text
/dev/cu.usbserial-0001, 115200 8N1
capture: /private/tmp/screenlog.0
```

## Build setup

Persistent source tree:

```text
/Users/arma/Documents/ChatGPT/HaikuOS
```

Build mirror and output:

```text
/Volumes/HaikuBuild/haiku
/Volumes/HaikuBuild/generated.riscv64
```

Build environment and useful targets:

```sh
cd /Volumes/HaikuBuild/generated.riscv64
HAIKU_REVISION=hrev99999 \
PATH=/Volumes/HaikuBuild/generated.riscv64/cross-tools-riscv64/bin:/usr/bin:$PATH \
/private/tmp/haiku-buildtools/jam/bin.macosxarm/jam -j4 kernel_riscv64

# EFI loader only:
HAIKU_REVISION=hrev99999 \
PATH=/Volumes/HaikuBuild/generated.riscv64/cross-tools-riscv64/bin:/usr/bin:$PATH \
/private/tmp/haiku-buildtools/jam/bin.macosxarm/jam -j4 haiku_loader.efi

# Full image when required:
HAIKU_REVISION=hrev99999 \
PATH=/Volumes/HaikuBuild/generated.riscv64/cross-tools-riscv64/bin:/usr/bin:$PATH \
/private/tmp/haiku-buildtools/jam/bin.macosxarm/jam -j4 haiku-mmc.image
```

RISC-V package-unavailable warnings are expected for this experimental target.

## Working firmware construction method

A fully source-rebuilt EDK2 image stopped immediately after OpenSBI, even when
the original SEC component was retained. The successful method is to preserve
the complete vendor firmware/DXE payload and replace only its embedded Haiku
loader FFS payload.

Important firmware identifiers and offsets:

```text
outer DXE FFS GUID:       9E21FD93-9C72-4C15-8C4B-E77F1DB2D792
LZMA guided GUID:         EE4E5898-3914-4259-9D6E-DC7BD79403CF
embedded loader FFS GUID: 4DB844A1-1B77-42B4-A90D-E72331D0A142
DXE volume size:          0x360000
loader FFS offset:        0x2c5aa0
PE payload offset:        16 + 0x2c5aa0 + 28
embedded loader size:     402409 bytes
```

Vendor firmware source image:

```text
/private/tmp/sophgo-edk2/Build/MilkV-Pioneer/RELEASE_GCC5/FV/MILKV-PIONEER.fd
size:   8585216 bytes
sha256: e7fb4785665e1f9fc433c5aeed75476957297f7b7e75cb51ca5539af444df1f6
```

BaseTools used to rebuild the nested firmware sections:

```text
/private/tmp/sophgo-edk2-2023/edk2/BaseTools/Source/C/bin/LzmaCompress
/private/tmp/sophgo-edk2-2023/edk2/BaseTools/Source/C/bin/GenSec
/private/tmp/sophgo-edk2-2023/edk2/BaseTools/Source/C/bin/GenFfs
```

The recompression pipeline was validated against the original: LZMA and FFS
contents were byte-identical apart from the expected erase-polarity state byte
(`0x07` standalone versus `0xf8` inside the FV). Every candidate should be
verified by decompressing its readback, comparing the embedded loader exactly,
and confirming that the bytes before and after the replaced outer FFS remain
identical to the vendor image.

The firmware must embed Haiku's **PE-format** loader:

```text
/Volumes/HaikuBuild/generated.riscv64/objects/haiku/riscv64/release/system/boot/efi/haiku_loader.efi
size:   402409 bytes
sha256: 516300120cc78007939732afd11e764c5012304681b3101d69df767f46b13da0
```

Do not embed the intermediate `boot_loader_efi` ELF.

## Active SD firmware and recovery files

The currently installed firmware is known to reach the Haiku kernel:

```text
/run/media/milkv/HAIKUPNR/riscv64/MilkV-Pioneer.fd
sha256: 8ec5c79ad0eaade5d7871b89a59c84e465fb870d5664391d2e5d2b82f979ef62
```

Recovery copies on the FAT partition:

```text
MilkV-Pioneer.fd.pre-embedded-loader-20260815  # original/vendor base
  e7fb4785665e1f9fc433c5aeed75476957297f7b7e75cb51ca5539af444df1f6
MilkV-Pioneer.fd.pre-page-table-fix-64fbb297
MilkV-Pioneer.fd.pre-trampoline-fix-6b13f4b6
MilkV-Pioneer.fd.failed-98113077
MilkV-Pioneer.fd.failed-572e3441
```

Do not overwrite or remove these recovery copies.

## Confirmed boot progression

The SD-only ZSBL -> OpenSBI -> vendor EDK2 -> embedded Haiku loader path works.
The loader finds BFS, loads `kernel_riscv64`, reads the Pioneer FDT, finds the
8250 UART at physical `0x7040000000`, exits UEFI boot services, installs Sv39
page tables, and transfers to the kernel on one CPU.

No graphics output is expected yet; firmware reports `GOP protocol not found`.
Serial is the authoritative console. Secondary CPUs remain deliberately
disabled until the primary kernel path is stable.

### Handoff trampoline fix

Candidate `6b13f4b6...` reached the loader but panicked with:

```text
Failed to allocate handoff trampoline.
```

In `arch_start.cpp`, the fixed-address request at `0x40000000` was changed from
`AllocateAddress` to `AllocateMaxAddress` with a maximum of `0xffffffffULL`.

### Page-table allocation fix

Candidate `64fbb297...` passed the trampoline but failed after printing the EFI
memory map:

```text
Unabled to allocate memory: -9223372036854775799
```

`mmu_allocate_page()` had forced every page-table-walk page below 4 GiB. Mapping
128 GiB with 4 KiB pages requires roughly 65,000 page-table pages, exhausting
that range. Page-table allocation was restored to `AllocateAnyPages`; only the
handoff trampoline remains constrained below 4 GiB.

### Kernel entry and VM initialization

The active `8ec5c79a...` firmware then printed:

```text
SATP: 0x8000000001fff2de
Calling ExitBootServices. So long, EFI!
Ariscv: kernel _start
riscv: cpu count set
riscv: rendezvous 1
riscv: rendezvous 2
riscv: cpu preboot
riscv: thread preboot
riscv: cpu ready
riscv: platform init
machine_platform: SBI
riscv: debug init
riscv: debug ready
riscv: dprintf enable
riscv: dprintf enabled
riscv: cpu init
riscv: cpu initialized
riscv: cpu percpu
riscv: cpu percpu ready
riscv: interrupts init
riscv: interrupts ready
riscv: vm init
riscv: vm tmap
riscv: vm arch
riscv: vm pages counted
riscv: vm slab
riscv: heap entry
riscv: heap allocate
riscv: heap initialized
riscv: vm heap
riscv: vm pages
riscv: vm cache refs
riscv: vm cache anon
riscv: vm cache anon no-swap
riscv: vm cache vnode
riscv: vm cache device
riscv: vm cache null
riscv: aspace lock
riscv: aspace table
riscv: aspace object
riscv: kernel areas cache
riscv: kernel ranges cache
riscv: kernel free lists
riscv: kernel first range
riscv: aspace object init
riscv: aspace map
riscv: aspace insert
riscv: aspace kernel
riscv: reserve loader ranges
riscv: reserve loader deferred
```

It then remained silent. The next call was through the PLT to
`arch_vm_translation_map_init_post_area()`, and that function's entry marker was
never reached.

The untested kernel-only fix in `src/system/kernel/vm/vm.cpp` obtains direct
linked addresses with `lla` and calls these four functions via pointers:

- `arch_vm_translation_map_init_post_area`
- `arch_vm_init_post_area`
- `vm_page_init_post_area`
- `slab_init_post_area`

New markers are `tmap post start/done`, `arch vm post done`, `page post done`,
`slab post done`, and `kernel args post start/done`. Disassembly confirms direct
`auipc`/`addi`/`jalr` sequences to the real symbols rather than PLT entries.

## SD update procedures

For a full generated image, copy only its BFS payload to SD partition 2:

```sh
sudo dd if=/home/haikuport/haiku-pioneer/haiku-mmc.image \
  of=/dev/mmcblk1p2 bs=512 skip=65540 count=614400 conv=fsync status=none
```

For the next test, do **not** rewrite the whole image. Mount/inspect the BFS
partition, locate its existing `kernel_riscv64`, make a timestamped backup, and
replace only that file with the new kernel. Confirm the destination path and
hash before unmounting; do not assume a mount path if the desktop automounter
has chosen a different one.

When a firmware update is genuinely required:

```sh
sudo mount /dev/mmcblk1p1 /mnt/haiku-sd
sudo install -m 0644 /home/haikuport/haiku-pioneer/MILKV-PIONEER.fd \
  /mnt/haiku-sd/riscv64/MilkV-Pioneer.fd
sudo sync
sudo umount /mnt/haiku-sd
```

The desktop automounter sometimes races combined commands. Update p2 and p1 as
separate operations and verify SHA-256 hashes before every boot.

## Source changes made

Port work currently touches these principal files:

- `headers/private/kernel/boot/uart.h`
- `headers/private/kernel/arch/generic/debug_uart_8250.h`
- `src/system/kernel/arch/generic/debug_uart_8250.cpp`
- `src/system/boot/platform/riscv/fdt.cpp`
- `src/system/kernel/arch/riscv64/arch_debug_console.cpp`
- `src/system/boot/platform/efi/dtb.cpp`
- `src/system/boot/platform/efi/mmu.cpp`
- `src/system/boot/platform/efi/arch/riscv64/arch_dtb.cpp`
- `src/system/boot/platform/riscv/start.cpp`
- `src/system/ldscripts/riscv64/boot_loader_riscv.ld`
- `src/system/boot/platform/efi/arch/riscv64/arch_smp.cpp`
- `src/system/boot/platform/efi/arch/riscv64/arch_mmu.cpp`
- `src/system/boot/platform/efi/arch/riscv64/arch_start.cpp`
- `src/system/boot/platform/efi/arch/riscv64/entry.S`
- `src/system/boot/platform/efi/arch/riscv64/arch_traps_asm.S`
- `src/system/kernel/arch/riscv64/arch_vm.cpp`
- `src/system/kernel/main.cpp`
- `src/system/kernel/slab/HashedObjectCache.cpp`
- `src/system/kernel/slab/Slab.cpp`
- `src/system/kernel/slab/allocator.cpp`
- `src/system/kernel/vm/VMAddressSpace.cpp`
- `src/system/kernel/vm/VMCache.cpp`
- `src/system/kernel/vm/VMKernelAddressSpace.cpp`
- `src/system/kernel/vm/vm.cpp`
- `src/system/kernel/vm/vm_init.cpp`
- `src/system/kernel/vm/vm_page.cpp`

Major changes include Pioneer-specific UART register width/shift handling, FDT
parsing and SBI platform selection, single-core handoff, correct EFI descriptor
walking, SATP-transition mappings, low trampoline allocation, unrestricted
page-table-page allocation, early kernel serial instrumentation, and direct
linked-address calls during the earliest post-area VM initialization.

## Work after the next boot

1. If `tmap post start` appears, use the following marker to localize whether
   translation-map post-area initialization returns.
2. Continue the same direct-address or relocation audit for any next early call
   that fails before reaching its function-entry marker.
3. Once VM post-area initialization completes, advance through kernel-args,
   scheduler, driver, and userland initialization.
4. Remove temporary high-volume diagnostics after primary-CPU boot is stable.
5. Only then revisit SMP, PLIC interrupts, storage, networking, graphics, and
   broader device support.

## Hardware history and caution

Two earlier power supplies appeared to fail during repeated cold-boot testing.
A replacement PSU restored operation, and reconnecting the SATA drive restored
the expected Linux boot path. This is no longer the immediate software blocker,
but the board still sometimes needs to remain off briefly before it will start.
Treat power anomalies as a separate hardware issue: avoid rapid power cycling,
keep the known-good PSU and required SATA connection, and do not assume a blank
display means the board is off—check fans, LEDs, SSH, and serial.
