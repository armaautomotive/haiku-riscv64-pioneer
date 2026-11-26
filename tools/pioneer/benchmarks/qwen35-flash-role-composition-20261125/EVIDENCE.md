# Guarded flash code and role recovery composition — 2026-11-25

The optional `PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS` composes the extra
per-worker code window with the existing role-image and recovery owners. It
requires independent retained-session, routing, snapshot, image and retry
proofs from its IO provider. The guarded parent supports only holding the
fleet, capturing and staging images, and restoring them before any release.
The processor-release and general recovery paths explicitly refuse operation.
Role images, role RAM and flash-window snapshots must be disjoint; image
sources cannot alias the retained snapshots. Restoration rechecks both image
regions, detecting cross-region writes.

`run_pioneer_qwen35_flash_decode_partial_tests.sh` passed, including the actual
composed owners with fake memory under address and undefined-behavior
sanitizers. Tests cover common-authority rejection and retry, overlapping
snapshots, stale images, misrouted writes in both directions, partial writes,
retry refusal until new proof, early and full held-only rollback, preservation
of originals and explicit processor-release refusal. The existing 120-worker
role recovery regression also passed. A Haiku/RV64 compile-only probe built
with optimization and warnings as errors, but deliberately leaves physical IO
and guard methods unresolved.

This is not a loadable driver or physical recovery proof. The original extra
code region remains simulator-only and overlaps legacy scratch. Live mapping
identity, locked controller and route ownership, firmware/DMA exclusion,
NoC drainage, private RAM, processor release and recovery after uncertain
physical writes remain unproved. No card was touched. Full-model baseline and
two-token hybrid simulations were still running at this checkpoint; neither
simulation provides hardware throughput. Work continues on the rewrite and
recovery before benchmarking or Assistant integration.

## Flash BAR and role IO composition

`DataflowFlashRolePciScope` routes only the fixed flash-code range to the
existing PCI view; the separate role-image range continues through its role
IO base. Its externally supplied guard must bind both to the same retained
session and protect the buffers and route for the whole operation. The adapter
does not supply physical authority. The sanitizer suite passed the actual
PCI view, owner and adapter together over fake platform and role memory. It
tested a rejected common authority, out-of-range access, held stage/rollback,
and a partially effective flash write that prevents rollback until a
separately authorized drain. The restored flash, role images, RAM and reset
control all matched the fake originals. The Haiku/RV64 compile probe for this
composition passed with the IO and guard methods unimplemented. The fake role
IO is not the physical role PCI adapter, and these checks establish neither
physical ownership nor safe processor release.

## Actual role PCI adapter composition — 2026-11-26

The optional flash-owner macro made the legacy role-only compile witness
instantiate a recovery owner without the newly required flash callbacks. That
legacy witness is now excluded only under the flash-owner macro; it remains
active in ordinary builds. The new combined test instantiates the actual
role PCI adapter, flash PCI view, flash role scope and held recovery-owner
operations together. Fake-MMIO runtime checks route ordinary role L1 and
flash L1 reads to their separate views and reject a range crossing the flash
window. This runtime test does not perform the complete held recovery sequence
with the actual role adapter; the full owner sequence is type-checked only.

The ASan/UBSan flash suite and ordinary 120-route role PCI adapter regression
passed. The combined fake-platform test also cross-compiled for Haiku/RV64
with warnings as errors. Neither test supplies real shared-session ownership,
NoC or firmware/DMA exclusion, release authority or physical recovery proof.
No card was accessed, and no throughput was measured.

## Held owner through both actual PCI adapters — 2026-11-26

This extends the previous routing-only fake-MMIO test. A per-core role window
model and separate flash window model now run the actual held recovery owner,
role PCI adapter, flash role scope and flash PCI view together. All 120
workers' role images, boot words and flash windows are captured, staged and
restored, then compared with the modeled original memory. The owner's release
attempt is rejected. The authorization and MMIO devices are fake, including
the held-image permission; no physical fleet, NoC, firmware, DMA or session
proof is supplied. The ASan/UBSan flash suite passed, and the combined test
cross-compiled for Haiku/RV64 with warnings as errors.
