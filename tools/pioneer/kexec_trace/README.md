# Late-shutdown UART trace

This native Linux userspace diagnostic marks systemd's **late userspace**
shutdown hook, not kernel reboot entry or successful payload execution. It
does not load an image, call reboot, touch PCI/disks/flash, or change saved
boot arguments. The kernel may still hang after this marker.

Build on native RISC-V Linux with
`sh pioneer_kexec_trace_build.sh OUTPUT_DIRECTORY`. Run portable host tests
with `sh tools/pioneer/tests/run_kexec_trace_tests.sh`.
Native builds explicitly use RV64GC/LP64D, avoiding accidental RVV 1.0 code
on the Pioneer's older vector implementation.

The helper ignores ordinary poweroff/reboot/halt invocations. For `kexec` it
requires root, riscv64, the exact known UART driver path, a root-owned regular
single-link 0600 `/run/pioneer-kexec-trace/armed` file containing 32 lowercase
hex digits plus newline, and kexec_loaded=1. The volatile arm file vanishes
on cold reboot, making a left-over hook inactive. `--self-test` requires the
same arm file but kexec_loaded=0 and emits a distinctly labeled marker.

After the existing Linux/FDT/UART recovery gates and explicit saved-work/test
approval, a reviewed binary can temporarily be installed as root:root 0755 at
`/usr/lib/systemd/system-shutdown/pioneer-kexec-trace`. Refuse to overwrite an
existing file. Arm it with a fresh token only for that test. Run the self-test
and observe its token in a fresh serial slice **before** any payload load.
Confirm the restored extlinux SHA remains unchanged. Use the previously
reviewed private kexec loader and normal `systemctl kexec`, never an immediate
`kexec -e` shortcut. Observe a bounded serial window and recover with the
existing relay off/wait/on procedure. Verify normal NVMe Linux, all 64 CPUs,
an empty kexec slot and unchanged boot-entry hash before declaring recovery.
Remove the temporary installed hook only after checking it still matches our
binary hash; do not remove an unknown replacement or another user's arm file.

The helper configures only the verified native ttyS0 to 115200 raw, no flow
control and no hangup-on-close. Writes are nonblocking, deadline-limited to
one second and 512 calls; drain polling is bounded to one second. A three-
second process alarm covers setup and both phases. A wedged kernel/device
operation could still defeat userspace timeouts; this is not a recovery system.
Host tests cover token policy, markers, partial writes, EINTR/EAGAIN, bounded
timeouts and invalid backend results. They do not validate actual hardware,
systemd invocation, final kernel shutdown, or post-kexec UART survival.
