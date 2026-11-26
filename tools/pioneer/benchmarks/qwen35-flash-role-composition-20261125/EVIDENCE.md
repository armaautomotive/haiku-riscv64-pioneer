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
