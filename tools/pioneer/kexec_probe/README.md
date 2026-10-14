# Pioneer kexec entry probe

This is the first SD-free bootstrap experiment, not a Haiku kernel or a
working Haiku-from-disk loader. It implements the RISC-V Linux Image header
and receives `a0` (hart ID) and `a1` (physical device-tree address) in S-mode.
Version 2 uses bounded, 32-bit polled UART output and read-only OpenSBI
Base/HSM-status queries. It does not touch PCI, disk controllers, firmware
flash, boot variables, secondary harts or persistent boot configuration. Every success/failure path deliberately
halts; it never returns to the Linux kernel that performed kexec.

## Build and local checks

```sh
sh tools/pioneer/tests/run_kexec_probe_tests.sh
sh tools/pioneer/pioneer_kexec_probe_build.sh
```

Default output is ignored `generated.pioneer/kexec-probe-20261013/build/`.
Set `PIONEER_CROSS_PREFIX` to a compatible RV64 compiler/binutils prefix to
use another toolchain. The script uses no Haiku libraries or startup objects.
Compilation is freestanding RV64 IMAC with CSR/fence support and soft-float
ABI, warnings as errors, no stack protector, jump tables or runtime imports.
The Haiku GCC specs have no usable lp64 linker emulation; explicit binutils
`ld -m elf64lriscv` links the objects without those specs.

`PioneerKexecProbe.Image` is the raw loadable artifact. **Do not kexec the
debug ELF**, which is fixed-linked at `0x2200000` for byte/layout inspection,
not validated as a load target.
The raw image is relocatable: a second link at `0x80200000` must produce
identical bytes. Its 64-byte header jumps directly to entry, declares a
2 MiB text offset, little-endian format and header version 0.2. The image
includes a 16 KiB main stack and independent 4 KiB supervisor-trap stack.
There is no BSS that a loader would have to initialize.

Host tests run the production FDT header/bounds checks and diagnostic control
flow under ASan/UBSan. The control-flow fixture substitutes physical-address
and SBI/console calls; it is not a CPU emulator or an actual SBI/kexec test.
Python tests reject truncated/malformed Image headers. Relink comparison,
no unresolved symbols and no retained ELF relocations are local checks,
not evidence that the Pioneer has executed the payload.

## Hardware gates

First confirm saved work and shut down the current OS cleanly. Remove the SD
only after the board is powered off. Leave P100A removed. With SD absent,
the unchanged SPI/LinuxBoot path must boot the existing NVMe Fedora system,
and pinned SSH plus the serial console must both work. Confirm the relay is
available for recovery before loading any probe. No SPI update is needed.
The live FDT must confirm UART `0x7040000000`, range `0x1000`, register shift
2 and I/O width 4. Output reads LCR/LSR and writes only a TX byte; it preserves
baud, clock, FIFO and interrupt configuration. A divisor-bank selection or
262,144 unsuccessful polls halts rather than writing an unsafe register or
continuing to a PASS with failed output. Host UART tests mock MMIO, not hardware.
Firmware boot-banner capture alone does not establish the UART's later line
state. Native Fedora's `/dev/ttyS0` reports a 9600-baud default, while capture
is 115200. Before a reviewed test, explicitly prepare the verified UART in
Linux for 115200/raw/no-flow-control and observe a fresh serial marker. This is
a temporary runtime console change, separate from the read-only preflight.
The Image itself does not initialize baud; visibility can still be lost during
Linux's shutdown. Do not infer payload execution from a successful Linux marker.

Run the read-only inventory/gates on native Linux:

```sh
sh pioneer_kexec_probe_preflight.sh
```

It verifies the known board family, non-EFI boot, NVMe root, no MMC device,
live FDT/UART layout, enabled/unoccupied kexec interface and secondary-CPU
shutdown configuration. It does not load an image. Review the actual installed kexec
version/help and its RISC-V Image support; do not assume current upstream
features are present in the older Fedora installation. An unknown or already
loaded request is a blocker, not permission to unload someone else's image.
The observed Fedora kexec-tools 2.0.26 supports only `elf-riscv`. A separate,
uninstalled upstream build at commit `0fd0d8e9ea975696fdc05ad267a65f3efd415612`
provides `image-riscv`. With that build, omit `-d`: it sends `KEXEC_DEBUG`
(0x10), which the older Linux 6.1 syscall rejects. This is not permission to
change a kernel security policy; retain hash, configuration and memory gates.

Stage files only in a private directory on the separate R3SL staging SSD.
Verify the image SHA-256 and a saved live FDT before a reviewed **load-only**
operation. Do not substitute a new vendor DTB or alter its memory layout for
this first test. A load failure must leave Linux running; do not execute it.
An accepted load is still not proof of the entry contract.

Execution must use the normal `systemctl kexec` shutdown path after approval,
not a direct `kexec -e` shortcut that skips orderly userspace shutdown and
filesystem unmounting. Observe serial from a recorded byte offset, with a
bounded host deadline. The probe stops intentionally, so SSH will not return.
Recover using relay off (seven-second hold), ten-second wait and relay on
(0.75-second pulse). With SD absent and SPI unchanged, verify NVMe Linux and
its pinned SSH return. Recovery itself must pass before unattended testing.
No automatic load/execution or persistent boot-selection service is included.

After the temporary boot-logging experiment was rolled back and normal NVMe
Linux cold boot verified, a separate [late-shutdown trace](../kexec_trace/README.md)
was prepared to observe the systemd userspace boundary without changing boot
arguments. It requires its own saved-work/test approval, temporary installation
and volatile arm token. A trace marker would not establish kernel handoff or
probe execution; lack of a marker remains ambiguous until its self-test and
late-hook invocation are checked on hardware.

The October 13 controlled trace test subsequently observed both the native
self-test and matching late-shutdown marker, but no v2 probe output. The
late userspace phase is now observed, not final kernel handoff or payload
entry. Relay recovery and complete baseline verification passed, and the
temporary installed hook was removed. The SD-free selector remains unimplemented.

`pioneer_linux_cpu_quiesce.sh status|quiesce|restore` is a separate native
runtime diagnostic, not a boot selector. Quiesce requires the original 0-63
mask and no loaded image; it retains CPU 0, bounds each hotplug operation and
attempts rollback on errors. Restore accepts only its single-CPU state or the
already-restored full mask. Neither mode changes saved boot settings. A hung
kernel operation may defeat rollback and still requires approved relay recovery.
Do not leave ordinary Linux work on one CPU. The observed SPI/LinuxBoot uses
`maxcpus=1`; testing this hypothesis is not evidence that Fedora's final kexec
handoff works or that secondary-CPU shutdown caused the silent attempts.

## Expected diagnostics and limitations

`PIONEER_KEXEC_PROBE v2 BEGIN` precedes the CSR/FDT checks. The probe prints
the actual hart, FDT address, initial SATP/SSTATUS/SIE/SIP/STVEC, runtime image
range, bounded FDT header fields and SBI Base/legacy-console/HSM/RFENCE
query results.
HSM must report the incoming hart started and the other 63 stopped; errors,
pending states or another started hart fail the check without starting/stopping
any hart.
SATP must be zero, supervisor interrupts disabled and hart ID below 64.
FDT must be eight-byte aligned, fit a known Pioneer RAM window, have a
compatible v17 header and remain within the 2 MiB probe cap. Resident low
firmware, the 3..4 GiB hole and MMIO are excluded from permitted FDT addresses.
Full FDT tokens, reservations, CPU topology and memory descriptions are not
parsed or validated in this phase. The header's boot CPU field is printed,
not assumed to equal the actual incoming hart ID.

`END status=PASS (entry/header/SBI state only)` means these entry checks and
SBI-reported hart states passed. It does not independently verify actual
secondary-CPU execution, DMA shutdown, cache coherency,
Haiku startup or a boot-volume choice. `TRAP` reports SCAUSE/SEPC/STVAL on a
private stack, then emits `END status=FAIL`. Missing output is ambiguous: it
could be a failed handoff, unavailable UART or a shutdown failure. Always retain
Linux's preceding kexec diagnostics and perform the planned recovery.
Version 1's legacy-SBI-console attempt was silent and inconclusive; the direct
UART path is diagnostic instrumentation, not an established fix for that attempt.

Sources: [RISC-V boot contract](https://docs.kernel.org/arch/riscv/boot.html),
[Image header](https://docs.kernel.org/arch/riscv/boot-image-header.html),
[upstream Image loader](https://github.com/horms/kexec-tools/blob/main/kexec/arch/riscv/kexec-image-riscv.c),
[Linux 6.1 handoff](https://github.com/torvalds/linux/blob/v6.1/arch/riscv/kernel/machine_kexec.c).
