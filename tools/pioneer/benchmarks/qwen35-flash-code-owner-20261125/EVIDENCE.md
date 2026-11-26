# Extra hybrid code-window ownership witness, 2026-11-25

The simulator-only hybrid image places extra code in each worker's fixed
`0xd0000..0xd8000` L1 window, inside the legacy ROOT scratch range and outside
the existing saved role-image region. A guarded, non-loadable fleet owner now
captures and double-reads this entire window on all 120 workers into a retained
3,932,160-byte host snapshot before staging any image. It checks the staged
image and untouched tail, reaudits every worker after all writes, retains the
snapshot on uncertain writes, restores every captured window after an
independent safety proof, and can recheck the original fleet after other image
owners restore their own regions. Image/snapshot host ranges must be aligned,
non-overflowing and disjoint.

A separate fixed BAR view and composition adapter connect that owner to a
platform/guard interface. They check the pinned BAR, exact range, mapping area,
selected worker route, host-buffer access and pre/post permissions. A failed
or uncertain write is sticky: subsequent reads, writes and close are refused
until a guard independently proves that all previous writes have drained.
Retrying fleet restore additionally requires a stronger retry proof. A failed
unmap leaves mapping state uncertain. The view can reconcile it only if an
independent retained-area identity proof establishes that the original mapping
is absent or exactly unchanged; only the latter permits a guarded close retry.
No real mapping-identity proof is supplied. Nothing here obtains a BAR lease,
selects a real route or supplies physical authority.

`sh tools/pioneer/tests/run_pioneer_qwen35_flash_decode_partial_tests.sh`
passed ASan/UBSan. Fake fleet and fake PCI platform tests exercised full capture,
staging and restore, partial-effect write failure, explicit drain, denied
unsafe retry, later misrouted corruption, post-restore re-audit, host-buffer
alias rejection, route/area denial, partial map and failed-unmap retention.
The owner and composed PCI view also cross-compiled for Haiku/RV64 with all
platform and guard methods deliberately unresolved. Their reported individual
capture/stage/restore frames were 368/368/368 bytes and view read/write frames
112/128 bytes; these are not a whole-kernel stack proof.

No existing driver or recovery path calls this code. It still needs real
retained device and controller leases; validated held/quiet fleet, firmware,
DMA and route exclusion; protected snapshot and image lifetimes; explicit
integration with the other image owners; a real safe retry and failed-map/unmap
lifecycle; and qualification of the extra region before physical use. It is
not proof of in-place recovery, a clean hardware unload or throughput, and the
simulator-only image remains non-packageable. No card was accessed.

A further fake-platform test exercised failed-close reconciliation for both
proved-absent and proved-identical outcomes, and refused retry without either
proof. The target wrapper compiled with both mapping-identity methods still
undefined. This is not evidence that Haiku can presently establish those
proofs or recover a real mapping without reboot.
