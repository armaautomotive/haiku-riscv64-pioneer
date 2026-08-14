# Milk-V Pioneer (SG2042) Haiku RISC-V port notes

## Goal and current status

Target: Milk-V Pioneer / Sophgo SG2042, 64 Xuantie C920 RISC-V cores.

Haiku does **not** boot fully yet. The custom boot path successfully loads the
Haiku RISC-V kernel and transfers control to it on one CPU. The latest test
proved that the kernel begins executing, then immediately takes a supervisor
trap. The trap handler currently recursively faults before it can print the
exception cause.

The immediate blocker is hardware: the Pioneer no longer powers on after a
power-off, and the user reports two power supplies have failed. Do not resume
boot tests until power delivery has been inspected and is stable.

## Safety rules used for this work

- Do not write the Pioneer SPI flash / MTD devices.
- Do not write the Fedora NVMe or the user SATA drive.
- Use only the removable SD card's dedicated test partitions:
  - `/dev/mmcblk1p1`: 1 GiB FAT32, label `HAIKUPNR`, SD-only firmware files.
  - `/dev/mmcblk1p2`: 1 GiB BFS, label `Haiku`, Haiku test volume.
- A normal Fedora reboot uses kexec and bypasses SD firmware. Haiku tests need
  a full power-off, wait about 10 seconds, then power-on with the SD card in.
- To recover from a hung Haiku test: power off, remove SD, boot Fedora.

## Access and serial

Fedora was reachable as `haikuport@10.0.0.247` by SSH using the temporary
key `/private/tmp/codex_pioneer_ed25519` on the development Mac.

Serial console: `/dev/cu.usbserial-0001`, 115200 8N1. A `screen` capture was
written to `/private/tmp/screenlog.0`.

## SD test update procedure

The full generated image contains a partitioned layout. Only its BFS payload
is copied to SD partition 2:

```sh
sudo dd if=/home/haikuport/haiku-pioneer/haiku-mmc.image \
  of=/dev/mmcblk1p2 bs=512 skip=65540 count=614400 conv=fsync status=none
```

Matching custom firmware is copied to the FAT partition:

```sh
sudo mount /dev/mmcblk1p1 /mnt/haiku-sd
sudo install -m 0644 /home/haikuport/haiku-pioneer/MILKV-PIONEER.fd \
  /mnt/haiku-sd/riscv64/MilkV-Pioneer.fd
sudo sync
sudo umount /mnt/haiku-sd
```

The desktop automounter sometimes raced the combined update command. The
reliable pattern is to write p2 first, then mount p1 separately, copy firmware,
and compare SHA-256 hashes before booting.

## Build setup

Persistent source tree:

```text
/Users/arma/Documents/ChatGPT/HaikuOS
```

Separate build source/output volume:

```text
/Volumes/HaikuBuild/haiku
/Volumes/HaikuBuild/generated.riscv64
```

Build command:

```sh
cd /Volumes/HaikuBuild/generated.riscv64
HAIKU_REVISION=hrev99999 \
PATH=/Volumes/HaikuBuild/generated.riscv64/cross-tools-riscv64/bin:/usr/bin:$PATH \
/private/tmp/haiku-buildtools/jam/bin.macosxarm/jam -j4 haiku-mmc.image
```

The RISC-V package-unavailable warnings are expected for this experimental
target. The generated SD image is `haiku-mmc.image`.

Temporary EDK2 source/output:

```text
/private/tmp/sophgo-edk2
/private/tmp/sophgo-edk2/Build/MilkV-Pioneer/RELEASE_GCC5/FV/MILKV-PIONEER.fd
```

The firmware embeds Haiku's **PE-format** loader:

```text
objects/haiku/riscv64/release/system/boot/efi/haiku_loader.efi
```

Do not embed the intermediate `boot_loader_efi` ELF; doing so produced a BDS
error and boot loop. Rebuild EDK2 after every loader change.

## Confirmed boot progress

The SD-only ZSBL -> OpenSBI -> custom EDK2 path works. EDK2 loads the embedded
Haiku boot loader, which:

- finds the BFS test volume;
- loads `kernel_riscv64`;
- reads the Pioneer FDT;
- finds the 8250 UART at physical `0x7040000000`;
- exits UEFI boot services;
- builds Sv39 page tables;
- transfers to the kernel on one CPU.

No graphics output is expected: the firmware reports `GOP protocol not found`.
This is a serial-headless test.

The original SMP start-up hang was avoided by forcing one CPU. Do not re-enable
secondary CPUs until the primary kernel path is stable.

Latest meaningful serial result:

```text
arch_enter_kernel(... kernelArgs: phys 0x1ffe8b8000,
  virt 0xffffffc0028aa000, kernelEntry: 0xffffffc0021095a2, ...)
BC
K ...
TTTTTTTT...
```

Interpretation:

- `B` and `C`: the post-SATP handoff ran, the virtual kernel stack was set,
  and the jump to the kernel was issued.
- `K`: the kernel `_start` function prologue executed using the new virtual
  stack.
- repeated `T`: the supervisor trap vector was entered, then faulted while
  saving its frame or calling its C handler.

The next diagnostic source change adds `P` immediately after `PushTrapFrame`.
It has been built and staged for the next test; it has not yet been booted.

## Source changes made

All port edits were mirrored in both source trees above. Major affected files:

- `headers/private/kernel/boot/uart.h`
- `headers/private/kernel/arch/generic/debug_uart_8250.h`
- `src/system/kernel/arch/generic/debug_uart_8250.cpp`
- `src/system/boot/platform/riscv/fdt.cpp`
- `src/system/kernel/arch/riscv64/arch_debug_console.cpp`
- `src/system/boot/platform/efi/dtb.cpp`
- `src/system/boot/platform/efi/arch/riscv64/arch_dtb.cpp`
- `src/system/boot/platform/riscv/start.cpp`
- `src/system/ldscripts/riscv64/boot_loader_riscv.ld`
- `src/system/boot/platform/efi/arch/riscv64/arch_smp.cpp`
- `src/system/boot/platform/efi/arch/riscv64/arch_mmu.cpp`
- `src/system/boot/platform/efi/arch/riscv64/arch_start.cpp`
- `src/system/boot/platform/efi/arch/riscv64/entry.S`
- `src/system/boot/platform/efi/arch/riscv64/arch_traps_asm.S`
- `src/system/kernel/main.cpp`

Important current changes:

- Pioneer-aware 8250 UART support (register shift/32-bit access width).
- Pioneer FDT parsing and SBI platform selection.
- Single-core kernel handoff.
- Correct EFI descriptor walking using `descriptorSize`.
- Identity mappings for the loader handoff code, trap vector, and kernel-args
  physical page across the SATP transition.
- Kernel virtual UART mapping and early serial diagnostics.
- `A/B/C` assembly markers in `arch_enter_kernel`.
- `K` marker at kernel `_start`; `T` at trap-vector entry; next build also has
  `P` after the trap frame is pushed.

## Likely next software work (after hardware recovery)

1. Boot the staged `P`-marker test from SD.
2. If `T` appears without `P`, inspect the trap frame's stack address and its
   page mapping. If `TP` appears, instrument the C trap call and print
   `scause`, `sepc`, and `stval` using a raw UART routine.
3. Fix the first actual exception, then remove temporary markers and the large
   memory-map trace.
4. Only after primary CPU boot is stable, revisit SMP, PLIC interrupts, storage,
   networking, display, and userland.

## Hardware caution

The board failing to power on, especially after two PSU failures, is not caused
by Haiku or the SD image: neither is running before PSU power is established.
Leave it unplugged until the PSU, motherboard/CPU power connectors, SATA/PCIe
power leads, front-panel switch, and possible shorts are checked. Test a known
good PSU first with only motherboard + CPU power connected, no SD, SATA, GPU,
or USB peripherals.
