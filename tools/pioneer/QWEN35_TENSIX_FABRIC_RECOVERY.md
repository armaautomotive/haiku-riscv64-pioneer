# Qwen3.5: candidate Tensix-fabric recovery

> **Fresh session for retained resident roles, offline (2026-11-25):** the non-loadable fresh recovery session now has a distinct fabric role purpose that pins the exact retained role binding and graph owner without requiring the diagnostic scratch owner. It requires the controller's same-ticket old-role-view disposal record; the record is published only after disposal and a final guard check, and revoked if that check fails. Fresh role views and conversion now require a ready role-purpose session with the same ticket. The role-restoration receipt is an intermediate scope-supplied result and releases no owner or DMA buffer. ASan/UBSan simulations covered role/graph identity changes at every setup boundary, absent disposition, missing or changed owners, rejected scope and ticket loss, as well as controller flag rollback and the actual binding with simulated resources. The non-loadable fabric/tree Haiku cross-build passed (driver SHA-256 `c63c33e4e3612f9ceff2bab17900134973ed39a329d9cd24a186c63b2974d410`; `/private/tmp/q35-fresh-resident-session-build-20261125.log`). Target suite 84877 overlaps these edits; focused tests were run separately. The updated reload suite runs in session 47242 (`/private/tmp/q35-fresh-resident-session-reload-20261125.log`). The role session has no real controller or physical ownership scope, its setup and PCI close still contain unguarded multi-access operations, and no full role restoration or owner retirement is connected. No card access, no-reboot recovery, model output or throughput was demonstrated. Continue only the rewrite and recovery, then benchmark and assistant integration.

> **Mixed auto/core-local fabric recovery, offline (2026-11-25):** the resident role-fleet owner can now transition an interrupted auto-restore when some captured cores were already restored or cancelled. Each such core is revalidated against its retained RAM snapshot and separately owned parent handoff; pending captured cores continue through the existing core-local shim path, and retirement rechecks both kinds. The real binding exposes which cores are already restored only while the exact fresh session remains usable. ASan/UBSan simulations covered a restored or cancelled core alongside a pending core through full role-state retirement, loss of the parent-handoff proof, corrupted already-restored RAM, and all 120 cores alternating restored/cancelled through simulated image and reset-control retirement. The old non-fabric role-owner regression passed. The non-loadable fabric/tree Haiku cross-build passed (driver SHA-256 `072481be4794bf4f1429202162e74a87a14ad1199c2487c1125522df607a18a3`; `/private/tmp/q35-fabric-partial-auto-final-build-20261125.log`). The previous reload suite 69457 reached its final PASS marker but its process handle was no longer available for exit status. The updated target suite 84877 (`/private/tmp/q35-fabric-partial-auto-target-20261125.log`) began before the final test-only 120-core case; that case passed independently. These are simulations with supplied IO proofs, not live writer exclusion or physical restoration. Fresh role-session authority, graph-owner retirement, no-reboot recovery, model output and decode speed remain unverified. No card access occurred. Continue only the rewrite and recovery, then benchmark and assistant integration.

> **Interrupted resident auto-restore to core-local recovery, offline (2026-11-25):** the real role-fleet owner can now make a one-shot transition from interrupted auto-restore mode to its core-local recovery sequence after a fabric hold. It refuses recorded auto-restores/cancellations or prior core-recovery work, and requires the supplied IO scope to bind the retained ticket and parent handoff, all held routes, saved-copy identities, reset controls and untouched uncaptured cores. A failed audit retains auto mode and cannot be replayed. The actual shared-role binding requires the exact fresh session IO and fresh views, and poisons admission if authority is lost after conversion; its concrete physical scope still denies the operation. ASan/UBSan simulated a complete single-captured-core conversion, core-local RAM/handoff/image restoration and original-state verification; it also checked a 120-captured-core conversion and staging of core 119, and loss of each conversion check, altered copies/reset controls, missing uncaptured-core proof and already-completed auto restore. This is not a 120-core end-to-end restore or physical evidence. The existing role-owner regression passed. The non-loadable fabric/tree Haiku cross-build passed (driver SHA-256 `a5c9031aa586f91d41af5d9a7dd7b9fb5109c285e43db42b14398de58e2ca849`; `/private/tmp/q35-fabric-auto-role-final-build-20261125.log`). The prior reload run 35538 reached its final PASS marker but its process handle was gone, so exit status was not observed; the updated reload suite runs in session 69457 (`/private/tmp/q35-fabric-auto-role-reload-20261125.log`). Target suite 50906 overlaps these edits. No card access occurred. Fresh role-session ownership, real writer exclusion, partially completed auto-restore cases, full graph-owner retirement, no-reboot recovery and model output remain unfinished. Continue only the rewrite and recovery, then benchmark and assistant integration.

> **Fresh resident role views, offline (2026-11-25):** the retained role-view owner and actual shared-role binding now have separate guarded, one-shot fresh-map and fresh-close operations using the exact pinned BAR windows after old role views were removed. The fresh session, PCI reservation, fabric metadata and caller-supplied area-exclusion/ownership checks are required; partial maps or closes remain retained, ordinary close/reopen cannot bypass the one-shot, and saved role snapshots remain allocated. Prior unconfirmed image-write evidence remains sticky across remapping. ASan/UBSan simulations exercised every guard boundary, map failure, null view, duplicate/protected area, close failure, unconfirmed write and the actual role binding with a simulated fresh session. The non-loadable fabric/tree Haiku cross-build passed (driver SHA-256 `5629bf78e644d6740736cd0daf492e0147674b5488f4035d472bb36a2f424b43`; `/private/tmp/q35-fabric-fresh-role-final-build-20261125.log`). The prior controller reload suite 67115 exited 0 with final PASS output; target suite 50906 overlaps these edits. The updated reload suite runs in session 35538 (`/private/tmp/q35-fabric-fresh-role-reload-20261125.log`). Additional fresh-close unmap and alias failure cases were added after the updated suite started and passed separately. The fresh-view API is only type-checked in the non-loadable driver; no real ownership scope/controller calls it, and no role RAM/image restoration, physical writer exclusion, no-reboot recovery or model output has been established. No card access occurred. Continue only the rewrite and recovery, then benchmark and assistant integration.

> **Fabric controller retains and disconnects role views, offline (2026-11-25):** the non-loadable fabric controller now checks role-view area IDs for duplicates and overlap against all old session MMIO and retained host-buffer areas before deleting any old session mapping. After guarded PCI idle, audit-object release and the bounded PCIe health samples, it invokes the actual role-binding cleanup under the same host scope, context, guard and ticket; the role owner, its code/RAM snapshots, PCI reservation and cold state remain retained. Failures stop the one-shot path. ASan/UBSan simulations exercised the actual controller plus host scope with simulated role operations, the real binding plus simulated scope, duplicate/alias checks, unmap failures and ticket loss. A non-loadable fabric/tree Haiku cross-build passed (driver SHA-256 `3a1e539bfd7c49f413b15ed86e7ef866a20ac8927b40634b5c6124bdd93e961e`; `/private/tmp/q35-fabric-role-controller-build-20261125.log`). Prior reload suite 24845 and target suite 25553 exited 0 with final PASS evidence but overlapped later edits; fresh reload and target suites run in sessions 67115 and 50906 (`/private/tmp/q35-fabric-role-controller-reload-20261125.log`, `/private/tmp/q35-fabric-role-controller-target-20261125.log`). A final test-only alias-check revocation case was added after these suites started and passed separately. No card operation occurred. The controller still stops with recovery pending: fresh role mappings, saved role-state restoration, writer exclusion and graph-owner retirement are not implemented or physically qualified. There is no no-reboot recovery, rewrite output or decode-rate result. Continue only the rewrite and recovery, then benchmark and assistant integration.

> **Retain resident role snapshots while retiring old views, offline (2026-11-25):** the actual role-view owner now supports a fabric-only, one-shot guarded removal of its CPU mappings. It rejects duplicate or protected area IDs before any unmap, stops on a lost check or unmap failure, and denies ordinary close/reopen after an attempted fabric removal. The real shared-role binding checks retained fabric metadata and the host-buffer area IDs, keeps the code and RAM snapshot allocations attached, and refuses ordinary owner destruction after any attempted fabric close. ASan/UBSan tests covered every check boundary, each unmap failure, aliases and the actual binding with a simulated proof; the existing ordinary role-view regression also passed. The non-loadable fabric/tree Haiku cross-build passed (driver SHA-256 `1db5441b3492b5e59f3971f9c8c1062da43e61929be6c5dd05d44b7d46d74529`; `/private/tmp/q35-fabric-role-views-build-20261125.log`). The reload regression is running in session 24845 (`/private/tmp/q35-fabric-role-views-reload-20261125.log`); the earlier target suite 25553 overlaps later source changes. This method is compile-witness only and is not invoked by the recovery controller: the controller must first check role-view IDs against all old session areas before removing either owner, and a fresh role mapping, guarded role-state restoration and physical ownership proof remain missing. No card operation, no-reboot recovery, model output or throughput has been demonstrated. Continue only the rewrite and recovery, then benchmark and assistant integration.

> **Guarded fabric scratch transfers, offline (2026-11-25 UTC):** the non-loadable fresh fabric recovery path checks its actual session ticket and a separate metadata scope around individual scratch-page PCI, NoC, L1, host-page and mapping accesses. A revoked check is sticky, preserves uncertain DMA and bank ownership, and prevents subsequent underlying accesses; the existing ASIC-reset witness keeps its separate path. Simulations under ASan/UBSan exercised a full 680-page restore, ticket and scope revocation within a page, and page read/write revocations sampled throughout plus immediately before and after every PCI command write and NoC doorbell. The non-loadable fabric/tree Haiku cross-build passed (driver SHA-256 `b8ba7d287319579ad978dfcfde36fb2c84fede307ff5d3ba05508f33f0e66a0c`; `/private/tmp/q35-guarded-ticket-page-build-20261125.log`). The earlier target suite 2842 is not a pass: despite the process handle reporting exit 0, its log ends with a shell parse error because its script was edited while it ran. A replacement target suite is running in session 25553 (`/private/tmp/q35-guarded-page-target-20261125.log`); it started before the final ticket-callback changes, so the focused tests and final build are the evidence for those changes. No card was accessed. The real ownership scope, guarded fresh setup and PCI close, role-owner retirement and physical writer exclusion remain unfinished; no hardware recovery, rewrite output or throughput has been verified. Continue only the rewrite and recovery, then benchmark and assistant integration.

> **Guarded fabric lease close, offline (2026-11-25 UTC):** the non-loadable fresh fabric session now wraps the real resident lease-close helper with sticky ticket and metadata admission checks around each PCI, NoC, L1, reset and TLB access. On revocation it makes no further underlying accesses, marks recovery required and invalidates any bookkeeping restoration claims. A simulated transient revocation at every check across a full L1, NIU and TLB close passed under ASan/UBSan; the composed fresh-session test covered lease-admission failures and retention. The non-loadable fabric/tree Haiku cross-build passed (driver SHA-256 `244534759cfc903a26dc9067f5010dd4d2c3b75b04a63d1de3e00077593e1013`; log `/private/tmp/q35-guarded-lease-build-20261125.log`). The target suite started before these changes and thus overlaps them; the new focused tests passed separately. Scope admission remains synthetic and is not physical writer exclusion; fresh mapping setup, page transfers, and PCI close still require guarded operations, and resident graph/role retirement is unresolved. The hardware path stays disabled; no model output, no-reboot recovery or decode speed has been established. Continue only the rewrite and recovery, then benchmark and assistant integration.

> **Guarded fresh-session audit, offline (2026-11-25 UTC):** the non-loadable fabric scratch-recovery session now uses the existing guarded mapping, firmware, fleet, L2, host-DMA, root-AER and mapping-close routines after transfers. A metadata-only scope callback and recovery ticket are checked around individual audit accesses, with sticky revocation; an uncertain result retains the audit mapping and owners. ASan/UBSan session simulations exercised each callback and guard-loss boundary and each audit phase; separate actual guarded-mapping simulations passed. The non-loadable fabric/tree Haiku driver cross-build passed (SHA-256 `0f829edc6a43668b520ced9b77d4b29261c9ac3815d5301237536d612fde2c33`, log `/private/tmp/q35-fresh-guard-build-20261125.log`). The prior reload regression exited 0; the updated target suite is running in session `2842` (`/private/tmp/q35-fresh-guard-target-20261125.log`). The scope is still synthetic and the lease close, transfer, mapping setup and PCI close are not yet guarded at each hardware access. Physical writer exclusion and the retained role-owner retirement remain unresolved, so no hardware recovery or model run is authorized by this evidence. Continue only the rewrite and recovery, then benchmark and assistant integration.

The pinned Tenstorrent system-firmware v18.5.0 source includes a Tensix reset
sequence in [`app/smc/pytest/e2e_smoke.py`](https://github.com/tenstorrent/tt-system-firmware/blob/80b056af360a58730756467d5cc8df8db789640d/app/smc/pytest/e2e_smoke.py),
`tensix_reset_sequence`. It is a candidate for recovering a faulted resident
rewrite without resetting the PCIe endpoint. The complete sequence is wired
inside a retained, non-loadable recovery transaction and has been simulated.
A one-shot private recovery request now prepares the guarded mapping and
calls that transaction for a retained faulted session; it is present only in
the non-loadable witness and has not been tried on this Pioneer. Its
context-owned queue adapter, eight RISC reset-unit writes and
120-worker unicast soft-reset pass retain the owner on uncertain failure.
Unicast coverage and the expected all-ones reset-unit readback remain
unverified on the card. The owner also requires an automatic-clock baseline,
250 MHz in forced mode, and automatic mode after release; configurations
without automatic clock mode are currently rejected. The reported
v18.5.0 firmware family is not proof
that the installed binary matches the source.

The upstream test performs these operations in order:

1. Send `FORCE_AICLK` message `0x33` with 250 MHz.
2. Write zero to each of the eight active-low Tensix RISC reset-unit words at
   `0x80030040 + 4*i`.
3. Send `TOGGLE_TENSIX_RESET` message `0xaf`; check the firmware response.
4. Send `REINIT_TENSIX` message `0x20`; check the firmware response.
5. Broadcast `0x47800` to Tensix `SOFT_RESET_0` at `0xffb121b0`.
6. Write `0xffffffff` to the same eight RISC reset-unit words.
7. Send `FORCE_AICLK` with zero to release the forced clock.

This is a whole-fabric operation, not a per-worker reset. The pinned firmware's
[`ReinitTensix`](https://github.com/tenstorrent/tt-system-firmware/blob/80b056af360a58730756467d5cc8df8db789640d/lib/tenstorrent/bh_arc/init.c)
clears NoC translations and calls `NocInit`, which reprograms NoC nodes beyond
the Tensix tiles. It also restores translation and clock-gating settings when
configured. Existing routes and saved device state therefore cannot simply be
assumed valid afterward. The test checks one Tensix scratch bit after the
sequence; it does not demonstrate recovery of this driver's 120-core workload,
its saved RAM and DRAM contents, or stable-driver reload.

The last attempted recovery on the Pioneer used a different ASIC+DMC firmware
reset and produced root PCIe error status before the first endpoint poll; see
[`benchmarks/qwen35-firmware-trace-20261124-ae/RUN.md`](benchmarks/qwen35-firmware-trace-20261124-ae/RUN.md).
That result does not establish whether the Tensix-only sequence succeeds or
fails. Do not repeat the failed ASIC reset as a substitute for testing this
candidate.

Before any physical trial, complete restoration and retirement in the
retained-owner recovery path. Establish exclusive admission and serialization
with firmware messages, PCI mappings and all NoC users; retain owner, snapshots
and quarantine on an uncertain step; and define
which steps can safely be resumed without reissuing a reset. Check the actual
firmware identity and message response conventions, clock state restoration,
all eight reset-unit words, the broadcast's harvested-core coverage, and the
post-reset NoC/DRAM routes, host DMA, all 120 workers and stable-driver state.
The normal idle/27B/model-hash/staged-restore preflight remains mandatory.
A timeout, changed identity, uncertain posted write or failed audit must not be
reported as successful recovery. Do not use this candidate until its real
controller, failure handling and bounded trial are ready.

The transaction now rechecks firmware boot, queue identity and pinned
telemetry after its fleet and DMA audits. It restores the selected route on a
successful audit and stops on an unconfirmed route write or lost guard; the
owner and mappings remain retained on failure. This is simulated evidence,
not a binary identity for the installed firmware or a completed recovery.
The non-loadable transaction also requires three read-only, clear root-port
uncorrectable AER samples. It does not clear AER, establish continued PCIe
health or establish that other device writers are excluded.
After those audits it attempts to restore the saved idle PCI layout once,
checking the ticket and readback around each changed config write. The
non-loadable controller then removes the old CPU MMIO views and closed audit
objects, with guard checks and no automatic retry. Host DMA buffers, the role
owner's mappings, bank claims, snapshots and cold state stay retained. Scratch
restoration, complete mapping disposal and recovery are still unfinished.

The fresh scratch-restoration session and coordinator can consume a separate
fabric proof in the non-loadable witness. The controller now records bounded
PCIe-health evidence after three matching idle-layout, link, Device Status and
AER samples. These samples do not prove continuing health or writer exclusion.
No real scope or controller calls the fresh session, and its existing
multi-access transfer and close helpers still need physical ownership and
guard qualification before use on the card. PCIe link field definitions are
documented in [Linux's PCI register definitions](https://github.com/torvalds/linux/blob/master/include/uapi/linux/pci_regs.h).
