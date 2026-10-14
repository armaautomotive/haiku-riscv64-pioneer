# Optional kernel handoff checkpoints

This GPL temporary module is a diagnostic, not a kexec fix or boot selector.
It uses the installed matching Linux 6.1.31 headers, normal kprobe APIs and
fixed function-entry checkpoints. It never initiates reboot, loads an image,
edits saved registers, or accesses disks/flash. It is not installed in the
module tree and must never be configured for autoload.

Build privately on the recorded native Linux with
`sh pioneer_kexec_kernel_trace_build.sh OUTPUT_DIRECTORY`.
Portable token/UART tests: `sh tools/pioneer/tests/run_kexec_kernel_trace_tests.sh`.
Native GCC 13.2.1 differs from the kernel's GCC 13.1.1; the matching headers
provide kernel flags and symbol versions. No signature/security policy may
be weakened to load a diagnostic. An unsigned out-of-tree module taints the
running kernel; unloading does not clear that taint until reboot.

`token=` requires exactly 32 lowercase hexadecimal digits. By default the
module only registers a breakpoint in its own no-op function, emits a fresh
`PIONEER_KERNEL_TRACE v1 stage=self_test ... armed=0` marker, unregisters that
breakpoint and remains unloadable. **Observe the matching serial token and
unload it before a handoff.** A successful self-test is not payload entry.

Only explicit `arm=1` also registers the nine fixed checkpoints. Handlers
other than `kernel_kexec` remain silent until that entry is observed. Each
stage emits at most once, without sleeping, allocating or changing saved
registers. Registration failure unregisters all earlier checkpoints. Unload
unregisters all remaining probes and unmaps the UART. Probes can perturb
timing; no handler can guarantee progress through an already wedged kernel.

Normal Linux 6.1 order is kernel_kexec, kernel_restart_prepare,
device_shutdown, migrate_to_reboot_cpu, cpu_hotplug_enable, machine_shutdown,
smp_shutdown_nonboot_cpus, kmsg_dump, machine_kexec. Symbol presence and
registration do not establish that a particular path runs or returns.
Messages mark function **entry**, not successful completion. No relocation
instruction is instrumented. Actual armed handoff testing is not yet done.

The module verifies the recorded release/board and exact FDT UART range,
register shift and access width before mapping it. It preserves UART clock,
baud, FIFO and interrupt settings. Reads are only LCR/LSR; writes only TX.
DLAB refuses output. A total 20,000 one-microsecond polling-delay budget covers
each <=191-byte message and final transmitter-empty drain. Hardware bus
failures can defeat such software bounds. Missing output is not proof of
missing execution.

## UART runtime-power prerequisite

Native self-test demonstrated that ttyS0 closing allows Linux to suspend
this UART: runtime_status=suspended, LCR=0x13, LSR=0, and the actual breakpoint
handler runs but cannot transmit. Holding ttyS0 open gives runtime_status=active,
LSR=0x60 and the same self-test passes. Setting the exact UART's power/control
to on also passes with no open tty. Therefore successful userspace UART
output followed by close does **not** establish post-close visibility.

`pioneer_linux_uart_keepalive.sh status|enable|restore` controls only that
runtime policy and uses root-owned volatile state to restore the original
auto value. It never changes boot arguments or initiates a handoff. Do not
make it persistent. Require a fresh UART marker after enabling and before
any reviewed load/normal systemctl kexec. Cold recovery must confirm the
original auto policy, no module/arm state, all CPUs, empty kexec slot and
unchanged boot-entry/executor hashes. No MCU relay or SD operation is included.

Sources: [kernel probe API](https://docs.kernel.org/trace/kprobes.html),
[runtime PM policy](https://docs.kernel.org/power/runtime_pm.html),
[Linux 6.1 kexec sequence](https://github.com/torvalds/linux/blob/v6.1/kernel/kexec_core.c).
