> **Audited resident descriptors, offline (2026-11-22):** the dedicated rewrite can reuse a one-shot previous descriptor receipt after the existing full completion audit, compare the eight mutable words, and write/read back only changed words. A composed test of the actual ledger and PCI transport across 120 simulated cores reduced warm-update old-record reads from 15,360 to 960 per graph, excluding unchanged write-readbacks; the completion audit still reads the full descriptors. Tests cover missing/stale/reused receipts, mutable drift, static corruption detected by device descriptor validation, and partial writes. This source-level count is not a throughput measurement. The optimized path remains disabled: the real scope explicitly denies the necessary lifetime-spanning descriptor ownership proof across audits, ioctls and publication, so the driver retains the full-read path. Full offline rewrite and recovery suites, focused fleet and guard tests, and the non-loadable Haiku cross-build passed. Driver SHA-256 `57530e2a50b1e6fcf999887debcb1e22d5da393659fefc8ac6b839c8b7d16546`; default-control SHA-256 `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85` matches the previous build. Physical ownership, launch, reset/reinitialization and stable-driver/27B restoration remain unqualified; no card was accessed and no throughput was measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Route-only peer leases, offline (2026-11-22):** the exclusive rewrite now configures each held peer’s NoC without reading or allocating an 8 KiB L1 backup. Before service installation, cleanup audits the held fleet, refuses an unexpected dirty L1 or command, and restores the recorded NoC and TLB state; uncertain operations retain/quarantine the owner. The removed old peer-lease loop read 2,048 words on each of 119 peers (243,712 32-bit reads), a source-level setup count, not a measured throughput change. Actual lease and composed 120-worker simulations cover success, dropped configuration, unexpected commands/dirty state and failed TLB restoration; the full offline rewrite and recovery suites and non-loadable Haiku cross-build passed. Driver SHA-256 `90abfc3ae9c4a84ba8acb315fb8f1fe86d86e918abdd19311315c21f8b67a493`; the default control driver is unchanged. Physical ownership, release, reset/reinitialization and stable-driver/27B restoration are unqualified. No card was accessed and no throughput measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Dedicated held-fleet preparation, offline (2026-11-22):** model binding for the exclusive rewrite now prepares the 120 held workers’ NoC/store leases without staging or booting the older Q38 dispatcher. Preflight and route checks accept the dedicated held state, and the panel mapping no longer allocates old per-core panel snapshots. If the service never starts, a fleet-wide held audit precedes reverse-order peer lease restoration; uncertain preparation or close retains/quarantines the owner. The client can request guarded reset after a failed pre-service BANK_END. Simulations exercise the actual extracted preparation entry point, rejection of an old booted owner, partial preparation, pre-service close failure and recovery; the full offline rewrite and recovery suites and non-loadable Haiku cross-build passed. Driver SHA-256 `8e54212202bbe54bb7165f223ad88e8b7e6a3bb7ae90bd2c3b30f613fc2ec8ee`, client SHA-256 `2b000245f136195748cee4f87f364e9b446c19773f4e1041d7a487158b9b778c`. The separate default control driver hash remains unchanged. Physical ownership, release, reset/reinitialization and stable-driver/27B restoration remain unqualified; no card was accessed and no throughput measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Dedicated compute takeover without legacy snapshots, offline (2026-11-22):** the exclusive resident service no longer captures prior compute L1/boot state or initializes the snapshot-handoff mailbox. It marks reset retirement necessary before its first hold attempt, so a failed hold, role stage, partial launch or clean stop cannot fall through to old-state restore/resume. The dedicated build requires the shared role owner and reset-retirement path. In a successful prior install, the removed snapshot loop performed two reads of 264 KiB per core: 16,220,160 32-bit reads over 120 cores, a source-level setup count rather than a decode-speed measurement. Dedicated simulations cover all 120 workers, a failed hold that took effect, failed role staging, partial release and reset-required close; the complete offline rewrite and recovery suites and the non-loadable Haiku cross-build passed. Driver SHA-256 `69c1f955b1e669c10c32ddbf3ad7a71ba9dad98903edab75c55c691bc2ff1895`. Real ownership/release, ASIC reset/reinitialization and restoration of stable-driver/27B usability remain unqualified. No card was accessed and no throughput measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Dedicated release contract, offline (2026-11-22):** the exclusive takeover no longer calls the older snapshot-path release proof. Its independent gate must still establish exclusive ownership, partial-fleet stop/hold and reset/reinitialization, and is still hard-denied for hardware. The simulated PCI adapter test requires dedicated release to work with the snapshot proof disabled, while refusing an unavailable dedicated proof and an uncertain retry; it rejects the preceding implementation. The full offline rewrite suite and takeover Haiku cross-build pass, and the driver hash stays `b5ea23fa0e40db9a6b4cd6b3965b1c8a99e286e0a2472f3eb0f57e5e1529444d` because the physical dedicated gate remains false. The [Tenstorrent tt-lab reference](https://github.com/tenstorrent/tt-lab/blob/f601ace5228d2db166fedd9c9c2db7dae51c89e2/src/tt_backend.cpp) takes exclusive ownership and holds, stages and releases workers; that reference does not qualify this Haiku implementation. Physical launch and reboot-free recovery remain unverified. No card was accessed and no throughput measured. Continue only the rewrite until ready, then benchmark and integrate the assistant.

> **Post-reset MMIO mapping attributes, offline (2026-11-22):** the fresh post-reset audit, ARC queue, notification and DBI mappings now pass `B_UNCACHED_MEMORY` in Haiku’s address-specification argument rather than the protection argument. The actual mapping adapter test checks both arguments for every simulated window; it rejects the preceding code and passes with the fix. The recovery suite and non-loadable takeover Haiku cross-build passed; driver SHA-256 `b5ea23fa0e40db9a6b4cd6b3965b1c8a99e286e0a2472f3eb0f57e5e1529444d`. RISC-V currently defaults an unspecified physical mapping to uncached, so the old placement does not by itself prove the old mapping was cached. Physical ownership, launch and reset/reinitialization remain unqualified, no card was accessed, and no throughput was measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Cold/graph fault recovery admission, offline (2026-11-22):** the explicit live-owner reset now admits a retained graph fault even if the service still reports ready, and a cold exclusive session even if service installation did not finish. If close cannot return a result, the client can request the same fault-gated reset; the driver still verifies exclusive ownership, team, generation and detached role scope before quarantine or reset. Simulations cover both new driver states, a rejected healthy session, and close-transport failure with successful and failed recovery. The reload suite and non-loadable Haiku cross-build passed; driver SHA-256 `4ca4d6cc4f7b9deaebf4640aafd4ab8edc4e7e842072a4f74ec70cf0f9d895f2`, client SHA-256 `984202a0a3b72857c9b90f8788d5bbcb763e128b9f7572a269a32eeb95a9acea`. Physical ownership/release, ASIC reinitialization, no-reboot recovery, and stable-driver/27B restoration are unqualified; no card was accessed and no throughput measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Latest rewrite progress (2026-11-22):** a non-loadable explicit fault-recovery request can quarantine a faulted exclusive takeover and attempt the existing guarded same-owner reset, including a failed worker stop; the client requests this after shutdown failure and checks the new idle generation. Simulated tests and the Haiku cross-build passed, but physical release, reset/reinitialization, no-reboot recovery, and stable-driver/27B restoration are not qualified. No card was accessed and no throughput measured. Continue only the Tenstorrent-style rewrite until ready; benchmark and integrate the assistant afterward. Before any card use, independently verify the 27B diagnostic has exited and the stable driver is idle.

> **Same-owner reset handoff, offline (2026-11-22):** after the dedicated role/compute fleet has stopped and the takeover still requires reset, the service now reports an explicit reset-required result and retains/quarantines its session. The existing ticketed reset/reinitialization operation can accept that same live exclusive owner only when the service reports stopped-reset readiness, the borrowed control scope is detached and all ordinary owner/generation checks pass. Successful retirement disposes the context but preserves the still-open cookie; if the fd closes during a pending reset, a separate inspector can resume the same ticket. The rewrite client recognizes the result, requests bounded recovery, handles a lost reply by checking guard state rather than reissuing an unknown reset, verifies the new idle generation, and skips normal BANK_END after successful reset. Simulated tests cover live-cookie lifetime, rejected scope/owner states, cleanup retry without another reset, transfer to a separate inspector, lost replies, timeouts, idle/generation checks and actual client unload outcomes. The reload/recovery suite, full offline dataflow suite and takeover-specific non-loadable Haiku cross-build passed; driver SHA-256 `851cd2cf79cf7718d34a4e35e86771d12fc95ea06ed119a48b0a15547decbe41`, client SHA-256 `0fae1e4978d1da45ca081b70c1ced78358c23d4a562b05998ee8479005823199`, conservative stack estimate 17872/18432 bytes (not a whole-program proof). These are simulated transitions: physical ownership, release, stop, ASIC reset/reinitialization and stable-driver/27B usability remain unqualified; release gates remain closed. No card was accessed and no throughput measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Dedicated role stop and partial-start retention, offline (2026-11-22):** the non-loadable takeover binding now uses a dedicated stop owner instead of immediately refusing after release. For an unconfirmed reset release, it requires the exact attempted release to be drained before classifying the worker as held or running. It requests reader/writer stop, observes their receipts and park samples, requests compute stop, observes its receipt and park sample, then holds and audits the fleet. Issued stop or hold writes are recorded before IO and only observed on retries, never reissued. Simulations cover all 120 workers, mixed held/running partial starts, absent drainage proof, and delayed or uncertain role, compute and hold writes. The full offline suite and takeover-specific non-loadable Haiku cross-build passed; driver SHA-256 `b1803ee573a079366d8c89ac437ccbae9b117899c2f8907fc6c44f5b9b4348b8`, conservative kernel stack estimate 17872/18432 bytes (not a whole-program proof). This is not a hardware stop/recovery proof: the actual release, stop, drainage and ownership gates still deny operations, and touched takeover state still requires verified ASIC reset/reinitialization before retirement. The clean no-reboot unload path, physical recovery and stable-driver/27B restoration remain unqualified. No card was accessed and no throughput measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Dedicated startup sequencing, offline (2026-11-22):** the takeover prototype now has a one-shot ordered release ledger for all 120 workers, a dedicated fixed reset-release adapter that does not require preserving old private RAM, and a service path that starts the new role shims without the old stack-copy handoff. Ready checks inspect the dedicated role images and startup receipts; the takeover running-write filter rejects the obsolete snapshot-release word. An uncertain release cannot be retried or skipped. Dedicated stop currently refuses to proceed after any release attempt, retaining the owner for the separately ticket-checked reset path; the real dedicated-release and other physical ownership proofs still return false. Simulations cover all 120 sequential releases, ordering violations, dropped reset writes, missing recovery authority, no-handoff startup and retained partial startup. The complete offline suite and takeover-specific non-loadable Haiku cross-build passed; driver SHA-256 `689590b039bafbbc32651b6c812dc1a5127489fd95d2f874d7786c5181cf1c52`, conservative stack estimate 17872/18432 bytes (not a whole-program proof). No physical launch or reboot-free reset has been verified. Implement dedicated clean/fault stop and reset/reinitialization, then qualify ownership and stable-driver/27B restoration before hardware use. No card was accessed and no throughput measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Dedicated fleet staging, offline (2026-11-22):** the non-loadable shared-role binding now has an explicitly selected dedicated takeover prototype. It stages the exported takeover reader/writer images across all 120 held workers without saving role private RAM, checks the full code and shim payload before writing any boot entry, then verifies all payload and boot entries again after staging. Any attempted write marks the owner as requiring reset-based retirement; ordinary snapshot restoration and destruction remain denied, while the existing ticket-checked post-reset cleanup can retire it after its mappings close. Release is explicitly disabled, and the actual held/quiet and write proofs still deny physical operations. Simulations exercise the real generated images on all 120 workers, rejected preflight, partial writes, silently dropped code/shim writes, late misrouting, retained bindings, wrong reset tickets and failed mapping cleanup. The full offline suite and the separate `--compile-exclusive-takeover-prototype` Haiku cross-build passed; driver SHA-256 `4ea432abfc1dade6727000a135359692e1879da343d410017388d90be489878a`, conservative stack estimate 17872/18432 bytes (not a whole-program proof). The ordinary build does not select this prototype. Complete dedicated launch, role shutdown, physical ASIC reset/reinitialization and restoration of stable-driver/27B usability before hardware use. No card was accessed and no throughput was measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Dedicated takeover role entries, offline (2026-11-22):** the rewrite now builds separate BRISC and NCRISC entry shims for an exclusively owned, idle fleet. Each sets and verifies the L1 cache-bypass bit, initializes its own local tick state, runs the existing resident role loop, and parks on stop or invalid state. Unlike the snapshot lifecycle, these entries intentionally do not copy prior private RAM or wait for a host snapshot handoff. Tenstorrent documents that each RISC stalls if it accesses its own local RAM while reset initialization is in progress ([Blackhole local RAM](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/TensixTile/BabyRISCV/README.md#local-data-ram)); this says nothing about host or NoC accesses to that RAM. The build exports a distinctly named `PioneerQwen35ExclusiveRoleImages.h` and verifies the linked reader/writer bodies are byte-identical to the existing shared code. Instruction-level tests execute both actual linked shims, covering stop, invalid state, ongoing ticks, and failed cache bypass, and reject any unintended memory access. The full offline dataflow suite and non-loadable Haiku cross-build passed. Reader shim SHA-256 `8663124426bc62f06abe006934ae9454c20fdf943ecc801acccb24cbafa8c98d`; writer shim `48db7fed8a6775c9ebb8afb106d4477cfdbe1212082c0918d4e57b43cf72ecaf`; non-loadable driver `f67017fa7924593021c8be0ddb0e1b5a506fc82557eac0aef4c7bd4dda8c0f03` is unchanged because the new entries are not connected to it. Implement the dedicated fleet staging, launch, shutdown and recovery lifecycle before using these images; verify that stable driver and 27B use can be restored after takeover. No card was accessed and no throughput measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Exclusive rewrite ownership, offline (2026-11-22):** the rewrite driver now implements opt-in exclusive device opens, requires the exclusive cookie for a bank-only resident session and for its borrowed role scope, and releases that file-level exclusion on final free even when a faulted owner is retained for a separate recovery inspector. The rewrite client requests exclusive access and verifies that a second ordinary open is rejected before any upload or device setup; it fails closed on older drivers or unexpected open errors. This follows the [Tenstorrent tt-lab Blackhole runtime](https://github.com/tenstorrent/tt-lab/blob/f601ace5228d2db166fedd9c9c2db7dae51c89e2/src/tt_backend.cpp), which tests exclusive KMD ownership, stages persistent firmware and resets worker RISCs on shutdown. Host ownership is not proof that firmware or another NoC agent cannot write, and tt-lab's tested configuration does not qualify this Pioneer or its firmware. Actual driver-hook tests cover competing and concurrent opens plus retained-fault inspection; the actual client test rejects a driver that ignores O_EXCL. The offline reload and complete dataflow suites and non-loadable Haiku cross-build passed. Driver SHA-256 `f67017fa7924593021c8be0ddb0e1b5a506fc82557eac0aef4c7bd4dda8c0f03`, client SHA-256 `71e20a3c09092f64cf118fa79bef9b9e75d8ebd39c663434c38ae3211b4f2deb`. Hardware launch, reset/recovery and the remaining ownership gates are not qualified; no card was accessed and no new throughput was measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Reference design follow-up:** tt-lab takes an idle dedicated device, overwrites worker L1/firmware, and holds its used RISCs in reset at shutdown rather than snapshotting their old private RAM. Evaluate whether a dedicated, reset-and-reinitialize takeover can replace this rewrite's elaborate dormant-role snapshot protocol while preserving the 27B files and its ability to use the card afterward. Do not treat the reference alone as proof that Haiku's stable driver can safely resume from that state, or bypass actual PCI, firmware, fleet and fault recovery qualification.

> **Fresh firmware DMA observation, offline (2026-11-22):** the non-loadable shared-role staging path now checks the firmware HDMA channel after model loading and the held-fleet sweep, immediately before its still-denied fleet ownership gate. The official v18.5.0 [PCIe DMA handler](https://github.com/tenstorrent/tt-system-firmware/blob/80b056af360a58730756467d5cc8df8db789640d/lib/tenstorrent/bh_arc/pcie_dma.c) accepts a requested chip address, so the original session-start audit alone is not a fresh observation at staging. The new check requires the retained baseline, PCI identity, routes, disabled bus mastering and stable idle channel 0 readings; its temporary TLB window is restored and verified, with uncertain restoration retaining the cold latch. The actual binding simulation exercises busy channels, changing readings, lost source stability and failed restoration; the full offline target suite and non-loadable Haiku cross-build passed. Driver SHA-256 `839833701cb00321b7a1007828d2f86818cf5013ec3efe6f0c971b3c469424ba`; conservative stack estimate 17888/18432 bytes (not a whole-program proof). This is a point-in-time sample, not proof against later DMA commands, other NoC writers, firmware reset, or a different installed binary. The independent ownership and launch proofs remain disabled. No card was accessed and no new throughput was measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Resume a timed-out reset wait, offline:** the non-loadable recovery operation now retains the v2 ticket and the exact reset result after the marker and both timer writes were issued but the marker wait timed out. A later request must validate that ticket before performing bounded read-only polling; it does not reissue any reset write. Completion also requires the expected card/root identity and bus mastering to remain disabled, followed by the existing PCI, firmware, fleet and DMA audits. Pre-trigger failures and uncertain PCI/mapping restoration are still quarantined, and a stale or newly faulted ticket prevents the poll. Simulated tests cover delayed marker clearance, repeated timeouts, invalid evidence, bus mastering, wrong identity and stale-ticket refusal. The full offline target suite, recovery suite and non-loadable Haiku cross-build passed; driver SHA-256 `940c5b6c60d2ac92bf5a7863b0abdae2d5afac860235d6c00ab455ef9d15c35e`. Physical reset/recovery is not qualified, the new guard has not been installed, and no card was accessed or throughput measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Retryable post-reset cleanup, offline:** the non-loadable recovery operation now uses the v2 guard and retains its existing reset ticket after a failed host-side shared-role or graph cleanup. A retry validates the same ticket and fault epoch, resumes cleanup without issuing another ASIC reset, and repeats PCI, firmware, fleet and host-DMA audits before unreserving the device and asking the guard to clear quarantine. A stale ticket, failed re-audit, uncertain unreserve or failed final guard check keeps the cold latch; the context and cookie remain retained if the final guard check fails. At that checkpoint reset timeouts and earlier PCI/mapping failures remained one-shot. Simulated tests cover cleanup retry with exactly one reset, stale or newly faulted tickets, failed re-audit, missing v2 guard and failure at each original stage. The full offline target suite, recovery suite and non-loadable Haiku cross-build passed; driver SHA-256 `8aed490e7932fafb52c1cac62316f930dacfa84b4342e94badee9775e8ca4185`. Physical ownership, reset and recovery remain unqualified, the new guard has not been installed, and no card was accessed or throughput measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Recovery ticket validation, offline:** the pinned-guard implementation now requires both reset and firmware proof generations to match the active recovery ticket before clearing quarantine. A separately versioned v2 recovery module adds a point-in-time check that the retained ticket, driver generation and fault epoch still match; the existing v1 interface remains exported. Tests cover stale or mismatched tickets, mismatched reset/firmware evidence, abort, completion and a new fault. The reload suite and non-loadable Haiku cross-build passed; the guard SHA-256 is `adbc86495803654471b83330ea8f2591f0433f8d71e083d6e9f790685bc985a4`. At that checkpoint the recovery operation did not yet use v2 or retry cleanup; this work has not installed or verified the new pinned guard on the Pioneer. The check is only a point-in-time observation; final fault-epoch validation and physical recovery remain necessary. No card was accessed and no throughput was measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Shared-role reset retirement, offline:** the non-loadable ASIC recovery path now retires the shared-role binding before releasing the graph or context. The role owner checks the same verified reset/firmware ticket, confirms the post-reset mapping is closed and ordinary session mappings were disposed, and unmaps its own BAR areas without restoring stale pre-reset tile snapshots. A failed unmap retains the binding and prevents guard completion. The actual binding simulation checks wrong tickets, incomplete proof, an open recovery mapping, incomplete session disposal, partial unmap failure and a subsequent primitive-level cleanup retry. The extracted reset-operation simulation checks that a role-cleanup failure retains the owner and cold latch and prevents graph release. At that checkpoint the reset operation was one-shot after failure; physical reset/reinitialization and role ownership remain unqualified, and this is not proof of reboot-free recovery. The full offline target suite, reload suite and non-loadable Haiku cross-build passed; driver SHA-256 `ceaa7f3cf4eba7fa25779a243e720361cf4ba1c14bbfda86673f036e6d956124`. No card was accessed and no throughput was measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Previously open shared-role reset cleanup gap:** the disabled in-place ASIC reset path disposes session mappings and the graph owner, but does not explicitly retire the shared-role binding before deleting its context. If that binding retains role mappings or snapshots, its destructor can refuse destruction, yet the reset path can continue toward clearing the cold latch. Do not enable this recovery path until shared-role mappings and the retained binding are safely disposed under the same verified reset ticket, cleanup failures retain ownership, and the guard cannot complete early. The current role hardware gates and reset operation remain disabled.

> **Binding-enforced tile startup, offline:** the actual non-loadable shared-role binding now consumes a one-shot release/handoff order before issuing each operation. It permits the next tile only after the current release is confirmed, both stackless snapshots have been retained and the stack handoff is confirmed. A release or handoff that fails before confirmation cannot be retried or bypassed through this binding, and stop/restore closes the sequence. Focused simulated tests cover all 120 tiles, out-of-order calls, uncertain release or handoff, and closing during cleanup. The full offline target suite and Haiku cross-build passed with driver SHA-256 `7e8cc46635ee5fcc31baf9fce8cda88f06e819d56f98196b963fef560b4a04ef`. This enforces software call ordering only; physical ownership, write drainage, launch and recovery remain disabled and unverified. No card was accessed and no throughput was measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Device graph record validation, offline:** the dynamic fetch path now uses the full record validation already performed immediately before its private token patch, instead of repeating that same validation inside the patcher. The patched record is still validated before command publication, and the graph authorization check still validates it before execution; standalone patch callers also retain the full input check. The unchecked patch function is private to the checked/fetch paths. On a complete 548-step decode across 120 workers this removes 65,760 redundant validation calls by design; this is a static call count, not a throughput measurement. Simulated comparisons cover all nine recorded invocation types on cores 0 and 119 and reject corrupt input, alongside the existing full recorded-graph comparisons. The full offline target suite and non-loadable Haiku cross-build passed with driver SHA-256 `d51afe9fab470c11328326a5e26bd86bac8dd59509b21b9d43b0cc9817cb5d15`. Physical ownership, launch and recovery proofs remain disabled; no card was accessed and no throughput was measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Sequential role release, offline:** after staging and auditing the whole fleet, the shared service now releases one tile, captures both stackless role-RAM copies and completes its handoff before issuing the next tile release. For driver-issued releases this limits a partial launch to at most the current tile lacking a completed RAM snapshot; firmware resets, misrouting, uncertain posted writes and external agents still require independent protection. Focused ASan/UBSan simulations verify the release/handoff order, a copy failure at tile 3, a later failed release, retention of earlier snapshots, restoration and retry across the 120-tile service. The full offline target suite and non-loadable Haiku cross-build passed, with driver SHA-256 `d6f7696283871c202b6e4770a7b9b95a2182b8dbcff9a6a8ed88e4c468d41542`. Physical ownership, launch and recovery proofs remain disabled; no card was accessed and no throughput was measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Stackless RAM timing evidence, offline:** the rewrite records successful per-tile capture of both verified pre-stack role copies. Each linked role shim executes at least 10,240 instructions before publishing its ready receipt; Tenstorrent documents the baby RISCs as in-order, single-issue cores and local-RAM initialization as taking at most 2,048 cycles after reset release. The interpreter now checks that instruction bound, and the actual scope requires the captured-copy observation as well as held/quiet ownership and a separate reset-continuity proof. That continuity proof still denies access: a later reset invalidates the observation, and held/reset samples alone cannot rule it out. The reported v18.5.0 firmware family is not a binary hash; in the pinned official v18.5.0 source, PCIe error interrupts can request a chip reset and explicit requests can reset or reinitialize Tensix, so the driver's lock cannot establish firmware continuity. See [baby-RISC timing](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/TensixTile/BabyRISCV/README.md#local-data-ram), [firmware reset requests](https://github.com/tenstorrent/tt-system-firmware/blob/80b056af360a58730756467d5cc8df8db789640d/lib/tenstorrent/bh_arc/cm2dm_msg.c), and [PCIe error interrupt registration](https://github.com/tenstorrent/tt-system-firmware/blob/80b056af360a58730756467d5cc8df8db789640d/lib/tenstorrent/bh_arc/init_common.c). The full offline target suite and non-loadable Haiku cross-build passed; the driver SHA-256 is `084ac022b36d0d3bc9d93063827e8dded0b0e5b56e8cbb0d11c24e3787f658a4`. The real hardware authority, launch and recovery gates remain disabled; no card was accessed and no throughput was measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Deferred stackless binding, offline:** the non-loadable shared-role binding now selects deferred fleet RAM capture: it stages owned images and reset-preservation controls without initial slow private-RAM reads, captures each role's L1 copies after its pre-stack receipt, and marks that tile's snapshot dirty before attempting stack release. Recovery restores captured RAM while images and handoff evidence remain available, then restores images and rechecks RAM and reset state. The new preparation and unsnapshotted-RAM protection proofs in the actual Haiku scope remain false; this path cannot run on hardware. Composed service simulations cover all 120 tiles, no initial private-RAM reads, a partial copy failure after earlier stack releases, clean restoration and retry. The full offline suite, final focused ASan/UBSan owner/service tests (including exported role images), RV32 target probe without unresolved imports, and non-loadable Haiku cross-build passed; driver SHA-256 `7433bbea26350462a2f4118fd380c72477a48074c6c30e96ff38ea57d0e95088`. These tests do not prove physical reset timing, exclusive firmware ownership, unsnapshotted-tile protection, write drainage or reboot-free recovery. No card was accessed and no throughput was measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Deferred fleet RAM ledger, offline:** the fleet RAM owner now has a separate stackless mode that reserves host storage before release, captures and marks each tile independently, and retains completed snapshots after a partial capture or release failure. Recovery restores captured tiles only while an independent proof protects every unsnapshotted tile throughout each bounded private-RAM operation; loss of that proof prevents or stops restoration. A final audit rechecks all captured snapshots and the unsnapshotted protection after later writes. Tests cover all 120 simulated tiles without initial private-RAM reads, partial capture, loss of authority, a failed restore, later cross-core corruption and retry. The full offline suite, final focused ASan/UBSan test, RV32 target probe with no unresolved imports, and non-loadable Haiku cross-build passed; driver SHA-256 `6c4d82743d32d412dfb3293740990df7b0410fd77398ac4f7842ff19a74bf45a`. This fleet mode is not connected to the real service binding: its proof that unsnapshotted RAM remains protected, including against misrouted writes and uncertain releases, is not implemented, and the current binding still performs initial NoC RAM snapshots. No card was accessed and no throughput was measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Stackless snapshot adoption, offline:** the per-tile private-RAM owner can now capture both role-produced L1 copies into host storage without a slow private-RAM NoC read. It checks the guarded handoff throughout, reads each bounded chunk twice, then rereads the full copies before publishing the snapshot; failed or partial capture exposes no snapshot. A separate dirty transition retains the snapshot before any stack-release attempt. The actual role PCI adapter implements the L1-only copy using guarded bounded reads, but its independent stackless-snapshot authority remains false. Tests cover no private-RAM reads, partial transfer, changes after an earlier chunk, lost authority, restoration and adapter range/route failures. The full offline suite, final focused ASan/UBSan tests and non-loadable Haiku cross-build passed; driver SHA-256 `4b4162235b64c7eb3ea35e39d483a5800170a5d2f77d5a5410db071ea77e9415`. This is a per-tile building block, not yet the fleet lifecycle: the actual binding still captures the original private RAM through the NoC before release. Partial/uncertain fleet launch, no-writer ownership and the physical timing and recovery proofs remain open; no card was accessed and no throughput was measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Tile timestamp investigation, offline:** [Tenstorrent's timer helper](https://github.com/tenstorrent/tt-metal/blob/main/tt_metal/hw/inc/internal/tt-1xx/risc_common.h) waits using the tile wall-clock registers, but its `get_timestamp()` also states that reading the low word samples/freezes the shared upper-word readback. A low-word read therefore has a side effect on other timestamp readers and cannot be treated as an unowned, read-only probe. The currently audited role-register allowlist excludes that offset; a focused test now enforces the exclusion. The cited sources do not establish the counter's relationship to private-RAM reset cycles across the relevant clock/reset states or exclude concurrent timestamp readers, so a 2048-tick observation is not yet a physical timing proof. The safer direction remains the role-local, pre-stack snapshots together with partial-release ownership and recovery. No card was accessed, no driver was built or loaded, and no throughput was measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Stack-release issue ledger, offline:** the shared-role transport now marks a stack-release MMIO attempt immediately before calling the panel write, after address, mapping, reset and write-authority checks. A separate one-shot submission flag prevents retries even if a pre-write check fails. Cleanup can therefore take the existing never-issued cancellation branch after a pre-write failure; a write call that returned with uncertain completion still requires the independent drain proof. The ledger is conservative: it records an attempted MMIO call, not device receipt or completion. Simulated PCI tests exercise denied authority, lost mapping, held reset, a dropped/delayed write, no retry, and a separate successful tile, checking that the ledger is set before the write callback. The full offline suite, focused ASan/UBSan test and non-loadable Haiku cross-build passed; driver SHA-256 `6a12e92bbc22132b0720555957e21a92519a6b971b0cd05e6ad0daf94067bcca`. The real cancellation, ownership and write-drain proofs remain disabled, and the initial private-RAM NoC snapshots remain in use. No card was accessed and no throughput was measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Unconfirmed stack-release recovery, offline:** after an unconfirmed stack-release write, shared-role cleanup can now inspect the handoff only if an independent proof fences that particular write and excludes later writers. A stable released value routes the tile through the normal role-stop path; a stable zero permits cancellation, with the write-drain proof rechecked before cancellation. Invalid or changing handoffs and unavailable proofs retain the fault. The actual Haiku write-drain proof still returns false, so this is not physical recovery or evidence that a role started. Simulated adapter tests cover released/absent values, bad or changing records, proof loss and cancellation denial without proof. The complete offline suite, final focused ASan/UBSan test and non-loadable Haiku cross-build passed; driver SHA-256 `b50fbd85716bdf972f48b10a18918869a9d0623000de23f99afa8d5ddff2c30b`. Next investigate tracking whether the MMIO write was actually issued so pre-write failures can be distinguished without claiming posted-write drainage, then finish the physical ownership and deferred RAM-snapshot design. No card was accessed and no throughput was measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Explicit uncached Haiku mappings, offline:** the rewrite role mapper now passes `B_UNCACHED_MEMORY` in the address-mode argument to `map_physical_memory`, rather than in the protection argument. The shared-role build does the same for its Q38 panel view; other driver variants retain their existing call. The API contract is visible in `headers/os/drivers/KernelExport.h` and the address-mode mask in `src/system/kernel/vm/vm.cpp`. The current RISC-V mapping path already defaults unspecified physical mappings to uncached, so the old placement alone does not prove the existing live mapping was cached. T-Head page attributes are conditional on `gRiscvTHeadMae`; actual hardware mapping and PCI/NoC write drainage remain unverified. Updated read/write mapping mocks enforce the argument contract and reject the preceding mapper. The full offline suite and non-loadable Haiku cross-build passed; driver SHA-256 `29d818adc55e5ab666d1280d2a5f2d9b1ee8eff5b0b6ce00f37cc8e3117944b7`. The separately built default control driver matched its preceding SHA-256 `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`. No card was accessed and no throughput was measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Stackless cancellation, offline:** role shims now publish individual cancellation receipts after seeing the explicit cancel value and park without touching private stacks. The shared-role cleanup is wired to issue cancellation once on released tiles whose stack-release write was never attempted, check both receipts and role PC samples, and then use the existing compute-stop and fleet-hold sequence. A possibly posted stack-release write is not cancelled; a failed cancellation write is only observed on retry, never reissued. A tile with a stack-release or cancellation attempt is no longer classified as never released. The linked RV32 shims passed interpreter checks for release, cancellation, invalid values and no stack write before release; the preceding shim is rejected. Simulated PCI-adapter tests cover missing/changing receipts, wrong or changing PCs, denied scope and a delayed cancellation write. The full offline suite, final focused ASan/UBSan adapter test and non-loadable Haiku cross-build passed; driver SHA-256 `fa47d0863c4629a1d74544b3e63b7ce1c449b8ebf35ee7628c7792d63ee02cbc`. The actual scope still denies cancellation and stopped proofs: ownership, incoming traffic, reset/write drainage, roles that never publish ready, uncertain stack release and reboot-free physical recovery remain unresolved. The composed real binding path has not run on a card. No card was accessed and no throughput was measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Pre-stack role handoff, offline:** BRISC and NCRISC now publish separate L1 receipts after copying their private RAM and wait without stack writes for a host release. Any other nonzero release value parks the shim; a host cancellation/recovery path is not yet implemented. The service initializes and audits the handoff before release, and its host binding waits for stable receipts, checks the running images and both RAM copies against already retained host snapshots, and consumes a one-shot release attempt before writing. Runtime access also requires confirmed stack release. An RV32 instruction interpreter exercises the linked shims through waiting, release, cancellation and failed cache bypass, and rejects the earlier shim. Simulated adapter tests reject missing, changed or already released receipts; service tests retain a partial handoff failure. The full offline suite, final focused ASan/UBSan tests and non-loadable Haiku cross-build passed; driver SHA-256 `b8829c26c95f3b61587804f7ec9c7f6dee2d0d3f1451f69e1b97d8913b254087`. These are simulation and compilation, not hardware proof. The existing initial host NoC snapshots remain necessary; firmware ownership, 2048-cycle timing, posted-write drainage, partial-fleet cancellation/hold and reboot-free recovery are still unresolved, and the real handoff scope continues to deny use. No card was accessed and no throughput was measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Stackless role RAM copies, offline:** the BRISC and NCRISC startup shims copy their respective 8 KiB private RAM into disjoint L1 regions before the first stack write. The role L1 owner saves and restores the additional 16 KiB per worker and checks it is unchanged before release. After both startup receipts, the host binding compares each copy twice against its existing host snapshot before accepting CoreReady. An RV32 instruction interpreter tests the linked images for copy and stack order, the L1 fence and dependency, and cache-bypass failure; it rejects the earlier shim. Adapter simulations detect corruption and changes between passes. The full offline suite, focused ASan/UBSan checks, and non-loadable Haiku cross-build passed; driver SHA-256 `2485faba1c33a6bb1d6b19246a0225d02173914557696ac319f0229559d1a745`. These simulations and compilation do not establish physical ordering. This is not yet a replacement for the initial NoC RAM snapshot: it still occurs before role release, and the required 2048-cycle timing proof remains unavailable. A pre-stack completion marker and handling for uncertain or partial release are needed before relying solely on role copies; firmware ownership, no-writer and recovery proofs remain open. No card was accessed and no throughput was measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **First weight prefetch, offline rewrite:** on an active shared-role projection, the BRISC reader now reserves its first weight page and starts its NoC read before packing the activation. It publishes the activation-ready ticket after packing, then completes the weight read before publishing that page; the compute role can begin its setup while the read finishes. Later pages retain the existing reader/compute queue behavior. If starting or packing fails after a possible issue, the reader attempts one drain and leaves failure retained; a failed completion is not retried or published. Concurrent simulated tests cover Q8_0, Q4_K, Q5_K and Q6_K across repeated commands, page contents and generation, early-publication rejection, packing failure, and an uncertain start; they reject both the previous non-prefetch code and an intentionally early publication. The full offline target suite, final focused ASan/UBSan test and non-loadable Haiku cross-build passed; driver SHA-256 `6aa19bd538b7297d4dc0818abad341f547667863c46a637c859d08dabdb31a83`, reader code SHA-256 `98217af995cf169e8b146e5696cc6dfea2f7d57623c8ef3359d721537da046e1`, role frame-sum estimate 5680/8192 bytes (not a whole-program proof). These are simulations and compilation, not physical output or speed evidence. Hardware ownership, release and recovery remain unfinished; no card access or throughput measurement occurred. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Role access clock preflight, offline:** the shared-role PCI adapter now samples both NoC routes and tile-clock status before held-state role boot, code, private-RAM and reset-control accesses, and before the reset read in role release, unconfirmed-release recovery and hold observation. The NIU status registers remain accessible while the tile clock is disabled; [Tenstorrent warns](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/MemoryMap.md#tile-clock-disable) that a RISC/L1 access in that state can wedge the NoC. The focused simulated test checks each NoC and all four paths, verifies no reset read or release write occurs when disabled, and rejects four independent regressions that omit the new checks. The full offline target suite and non-loadable Haiku cross-build passed; driver SHA-256 `d197a359229b2f2bb56322e78785c479bff4eea54e67019716947894aad297b4`. These are point-in-time observations and do not prevent a later clock change or establish incoming traffic, ownership, private-RAM timing, physical recovery or throughput. No card was accessed. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Multi-worker private-RAM recovery audit, offline:** a stable saved-state mismatch is now distinguished from an unreadable or unsafe worker. The fleet audit records mismatches on every worker in one pass, allowing one subsequent restore instead of discovering only the first mismatch on each retry. An unreadable worker or loss of its stable-RAM proof still stops the scan immediately, with no writes during the failed audit. A focused ASan/UBSan test covers simultaneous corruption on all 120 simulated workers, read failure, loss of stability and verified restoration; the multiple-corruption regression fails with the previous code. The full offline target suite, final focused test and non-loadable Haiku cross-build passed. Driver SHA-256 remains `707599da511e007ba913ffb9be765b707c56f8b468283f9620bccf94f18ad6cd`: the real scope still denies this path and the compile-only build removes unreachable code, so this is not a live recovery implementation or a hardware result. Physical ownership, release and recovery gates remain unfinished; no card access or throughput measurement occurred. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Projection source initialization, offline:** the Tensix compute role now initializes its reserved 1 KiB destination-init source once during each projection setup and reuses it for later output groups. The source is outside the dataflow queues and saved dispatch image, and its L1 visibility fence remains before Tensix use. This removes 256 repeated stores for every additional output group; the amount of any hardware speedup is unknown. The setup entry is explicitly non-inlined to keep the non-shared graph's existing saved-dispatch placement check valid. The full offline target suite and non-loadable Haiku cross-build passed; the linked shared-role image passed a focused source-order/placement check that rejects the prior image. Cross-build driver SHA-256 `707599da511e007ba913ffb9be765b707c56f8b468283f9620bccf94f18ad6cd`. These are compile/simulation checks, not physical numeric or speed evidence. Physical ownership, release and recovery gates remain disabled; no card was accessed. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Running reader/writer image observation, offline:** during installation, after the role release is confirmed, the binding is wired to compare each worker's BRISC and NCRISC boot words, exported shims and code twice through bounded mapped views. Route and read authority are checked around chunks of at most 64 words; an observation is cached only after the separate readiness and PC checks succeed. Tests cover all 120 simulated routes, denied inspection, boot/shim/code corruption, image changes between passes and route drift; inspection performs no writes. The actual Haiku mapper and borrowed scope compile, the complete offline target suite passed, and the non-loadable cross-build's driver SHA-256 is `c26911ba31ce6e1ec5167c0412ab8464e9ca988385ed09efd82a4b8d65b2c1e6`. This startup snapshot cannot prove ownership or exclude later writes; independent physical held, release and runtime proofs still deny the path. No card access or new throughput measurement occurred. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Running reader/writer PC observations, offline:** the shared-role binding is wired to take two BRISC and NCRISC PC snapshots once the independent runtime proof permits checking startup readiness, runtime state, retained faults and acknowledgement, and rejects PCs outside their own exported shim/text image or at that role's stop/failure park loop. Bounds derive from the actual generated images, and simulations cover wrong-role addresses, invalid bounds, changing snapshots and both normal/fault scopes. [Tenstorrent documents](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/TensixTile/BabyRISCV/README.md#riscv-pc-snapshot) that these snapshots may be speculative: they do not prove an instruction executed, an image is uncorrupted, a processor stopped, NoCs are quiet, or the device is recoverable. They may also cause false rejection and their runtime overhead is unmeasured. The independent physical ownership/runtime proofs remain false. The runtime/stop selection also checks tile-clock status before reading reset. The full offline target suite and non-loadable cross-build passed; driver SHA-256 `1619ca4c208f923aad13e18e5a8d30ee01118ddb6b65096bc388472b7edd1cc0`. No card access or new throughput measurement occurred. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Earlier tile-clock guard, offline:** the shared-role driver's `ProveFleetRoutes` now verifies the selected route and clock-enable bit on both NoCs before it calls the existing dispatcher audit that reads reset and L1. The check uses NIU-only reads, which [Tenstorrent documents](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/MemoryMap.md#tile-clock-disable) remain accessible while a tile clock is disabled. Tests cover all 120 selected routes, a disabled or changing clock on either NoC, lost mapping, wrong route and the actual session read filter; the callback making a RISC/L1 read is not invoked on failure. The full offline target suite, final focused clock test and non-loadable cross-build passed; driver SHA-256 `28213f7057684b2d221dea1ab3f20f2f5a0b82cdd62c70d8c30eee54897b2a78`. This sample does not stop a later clock change or protect unrelated operations performed before this rewrite preflight. Incoming traffic, firmware ownership, role launch and physical recovery still require work; no card access or throughput measurement occurred. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Real FIFO-status read filter, offline:** `PioneerSessionIO::ResidentNocRead` uses `BlackholeResidentReadRegister`; the earlier mock had bypassed that filter, so enabled request FIFOs falsely failed the new held-fleet audit because offsets `0x64` and `0x68` were rejected. The non-loadable shared-role build now permits exactly those two read-only NIU status offsets on both NoCs. The simulated session uses the real filter, tests both enabled/empty FIFOs through the actual shared binding, rejects busy/overflow and neighboring/write offsets, and fails on the previous code. The full offline target suite and non-loadable Haiku cross-build passed; driver SHA-256 `c9329241af1c9da36b29b98227066ecc0d1440af46113846b6d8b8b32f52a319`. This corrects the physical adapter path but does not establish incoming-NoC or firmware ownership, release, physical recovery, or throughput. No card was accessed. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **FIFO-aware device transfers, offline:** the shared reader/writer images and graph service now check each enabled NIU request FIFO is empty and has not overflowed before reading completion counters. The shared NoC check is also used by the host held-fleet sample. This follows [Tenstorrent's documented ordering](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/MemoryMap.md#niu-request-fifos): queued requests are not yet reflected in those counters, and availability must return and be tested before counter reads. A host test confirms busy/overflow FIFOs suppress every counter read; RV32 disassembly tests check the exported reader, writer and graph images and reject the preceding versions. The full offline target suite and non-loadable cross-build passed; driver SHA-256 `1091f4df1ab9077f950c5538ff350c0b2052eb1def16aee91884f5db869f4eee`. This assumes exclusive NIU configuration; it does not prove ownership, stop incoming traffic, prove physical recovery, or measure performance. No card was accessed. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **NoC FIFO and clock sampling, offline:** the held-fleet and selected-role PCI audits now sample both NIU configurations before reading the tile reset register. A disabled tile clock, changing clock/FIFO configuration, nonempty enabled request FIFO, or sticky FIFO overflow rejects the sample. Tenstorrent's [NIU documentation](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/MemoryMap.md#niu-request-fifos) says queued requests do not appear in outstanding counters and doorbell zero does not prove an enabled FIFO empty; it also warns that RISC/L1 access to a clock-disabled tile may wedge a NoC. Simulations cover both NoCs, empty and busy FIFOs, overflow and config drift; the clock/FIFO regression rejects the preceding code. The full offline target suite and non-loadable Haiku cross-build passed; driver SHA-256 `3d36c6f0402e3e3b6a0937ea7a4903a1eff228d6a93c9d1e70af5b2ccfb24dc9`. These samples do not prevent a later clock change, audit accesses earlier in installation, prove incoming traffic absent, rule out 8-bit counter wrap, or bound a stuck MMIO read. Firmware ownership, role launch and physical recovery remain incomplete; no card access or new throughput measurement occurred. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Stage-preflight recovery, offline:** after a complete role-state capture, failed held/reset-routing checks at per-tile, 120-tile, or composed image staging now retain original snapshots in a faulted, retryable recovery state. Simulated tests cover pre-write and mid-fleet failures, blocked restore while safety checks fail, later corruption, and complete restoration; each of six focused tests rejects the previous code. The full offline target suite and non-loadable Haiku cross-build passed; driver SHA-256 `69ab68c3f2ad7ec3110c1e296ba00b3c7c167b597241ea56ba2b15354665efc2`. This is not yet physical driver recovery: actual hardware ownership, startup and restoration gates remain incomplete. No card access or new throughput measurement occurred. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Boot routing inspection, offline:** the shared-role adapter now checks stable TRISC0/NCRISC reset-PC override selectors through the actual guarded control views, with held-reset reads before and after and a renewed fleet/route check. The borrowed scope authorizes only these fixed status reads for the selected session/core and reported firmware family; it does not claim that selectors are default. This replaces the unconditional false boot-routing scope stub, while the independent fleet ownership, memory, launch and recovery gates still refuse hardware use. Simulated tests cover both override bits, unrelated held-TRISC selectors, a reset change during inspection, selector drift, wrong core/thread/firmware, and all 120 routes. The full offline target suite, final focused fault tests and non-loadable Haiku cross-build passed; driver SHA-256 `69ab68c3f2ad7ec3110c1e296ba00b3c7c167b597241ea56ba2b15354665efc2`, conservative stack estimate 17968/18432 bytes (not a whole-program proof). No card was accessed and no new throughput was measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Firmware family verified, 2026-11-22:** after confirming the 27B/4B diagnostics absent and stable driver55 generation20 idle, the existing bounded operating-telemetry query reported bundle `12050001` (18.5.0, suffix 01), CM `000e0000`, with PCI/ARC/TLB restoration successful. The post-query stable driver was independently idle again. See [`benchmarks/qwen35-role-firmware-preflight-20261122/RUN.md`](benchmarks/qwen35-role-firmware-preflight-20261122/RUN.md). The official v18.5.0 source resolves to `80b056af360a58730756467d5cc8df8db789640d`; its normal initialization leaves L2CPU RISC reset asserted and it registers Tensix reset/reinitialization requests. Reported version is not a firmware hash and does not prove the installed binary, all future traffic, or exclusive firmware control. The non-loadable role-status scope now requires both expected reported firmware values from its existing session health baseline before any status inspection; tests reject missing/changed fields. The full offline target suite and final targeted binding test passed. Its global ownership proof remains false and no role was launched. Cross-build SHA-256 `6d73c44a383f35ac106ea98e6c4681fbd6ecec28f43e88322a553a47096def7f`. No new inference or throughput was measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Guarded held-fleet status sweep, offline:** during the non-loadable shared-role install callback, the driver can now sample both NoC identities, four initiator doorbells and 32 outstanding/outgoing-ID counters, plus reset state, twice across all 120 selected workers. It requires the borrowed session scope and exact selected route, then reselects and verifies the original route; uncertain restoration latches cold recovery. The separate incoming-NoC and firmware ownership proof still returns false, so these snapshots do not authorize role mappings or launch, guarantee later quiet, identify installed firmware or bound a stuck MMIO read. Host simulations cover every busy register, failed/partial selection, second-pass changes, route drift and scope loss; an actual binding simulation confirms both sweeps run while role launch stays denied. Full offline suite and combined Haiku/RISC-V cross-build passed; conservative kernel stack estimate 17904/18432 bytes (not a whole-program proof), non-loadable driver SHA-256 `0d37f1de459c478a7ab30c618adeea89518ddc5f5c78df96f8c075dad96fbb2b`. No card was accessed and no new throughput was measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Real activation packing in concurrent roles, offline:** the persistent reader/compute/writer test now packs mixed-sign, changing activation vectors with the actual `PioneerQ38ProjectionQuantize` routine and compares every compute input chunk against a separately packed copy of the original vector across 64 commands per case, cycling Q8_0, Q4_K, Q5_K and Q6_K. An intentionally corrupted expected packed word was rejected; ASan/UBSan test passed without corruption. Weight transfers, matrix math, timing and hardware remain simulated, so this is a cross-role data-lifetime test, not inference correctness or a throughput result. Tenstorrent’s [NoC counter documentation](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/Counters.md#read-requests) states read-response count advances after its L1 writes complete, supporting the existing reader completion handoff subject to physical qualification. The raw [memory-map table](https://raw.githubusercontent.com/tenstorrent/tt-isa-documentation/main/BlackholeA0/TensixTile/BabyRISCV/README.md) uses merged cells: BRISC and NCRISC have the full 8 KiB fast local RAM; the 4 KiB limit applies to TRISCs. No card was accessed. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **L2CPU reset observation, offline:** the non-loadable shared-role install witness now samples the ARC L2CPU reset register after the service's worker-hold sequence, requiring the exact session owner and selected PCI route. It checks stable firmware-boot and reset readings, rejects a RISC that is not held in reset, and restores and verifies the prior TLB mapping; uncertain restoration latches cold recovery. Tests simulate changed readings, an incomplete mapping, failed restoration and lost ownership. This observes reset state only: it does not prove earlier NoC traffic drained, prevent firmware from releasing a processor, identify the card's firmware, or bound a stuck MMIO transaction. The actual fleet ownership proof still denies role installation; this witness is not loadable or hardware tested. Non-loadable cross-build SHA-256 `ce6874481b8093371b7ae4dce6478b1dc91e0c1e9c99aa425f642a99d9090199`. No card access or new throughput measurement occurred. Continue only the Tenstorrent-style rewrite until it is ready, then benchmark and integrate the assistant.

> **Failed-graph queue cleanup, offline:** when the retained graph's independent all-core, image, route and NoC audit has authorized a failed-command acknowledgement, the shared worker can now discard a bounded number of unconsumed queue pages after both role bodies have returned. Invalid occupancy, missing role receipts, unaudited cleanup and pending pages on successful work are rejected. The shared driver backend uses this role-aware audit instead of applying the old unconditional balanced-queue check a second time. This clears local queue records only; it does not commit the failed graph or prove a stuck NoC safe. The real physical NoC/role audit still returns false, so this is not yet on-card or reboot-free recovery. The simulated persistent-role test now runs 64 changing-data commands per type case, cycles through all four quantization types, and exercises a reader failure after seven successful commands followed by audited cleanup and stop. The combined non-loadable driver SHA-256 is `291bfacf4922535a151be00a89636f87d526825f41a511af280df4288c33c737`. No card access or new throughput measurement occurred.

> **Tensix source visibility fix, offline:** the compute role now waits for its own L1 stores to be processed before the matrix engine can consume the initialized source tile, destination-init window, affine metadata or activation panel. This uses the same fence/read/dependent-instruction sequence as the NoC fix, in a shared helper; a fence alone does not order stores to L1 against a later instruction-FIFO store. The exported RV32 graph image was checked for all four actual source addresses and the dependent read. The non-loadable Haiku driver SHA-256 is `75050747451fc594d98c6ac8aa8b3c7241df717d8a47c7a69c721a1417685c04`. The full target suite and an independent disassembly positive/negative check were run offline. Physical role ownership, safe startup/stop and recovery remain incomplete; no new hardware result or throughput is claimed.

> **NoC source visibility fix, offline:** every device NoC write now follows the local store-queue fence with a volatile read from its L1 source and a dependent RV32 instruction before ringing the NoC doorbell. This also covers graph barrier outbox payloads: [Blackhole memory-ordering documentation](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/TensixTile/BabyRISCV/MemoryOrdering.md#instructing-the-noc) says the NoC can otherwise read old L1 data even after a RISC fence. The existing transfer still waits for its NoC completion before the source may be reused. The target suite passed, and a new disassembly check confirms the read/dependency/NoC1 write sequence in the actual exported writer image; a deliberately altered disassembly was rejected. The combined non-loadable driver SHA-256 is `e88b8fd77c832413872986f8f90d8af13880934da09d4e852b17c19442ddfe3b`. This fixes one ordering gap; physical ownership, launch and recovery proofs remain incomplete, and no hardware output or throughput was measured.

> **Ownership evidence, 2026-11-22:** upstream [Blackhole CMFW throttler](https://github.com/tenstorrent/tt-system-firmware/blob/6a3c136f20a2c93d7afef46dcfa4234513a9e5b9/lib/tenstorrent/bh_arc/throttler.c) broadcasts a 32-bit throttle value to Tensix L1 address `0x10` on initialization and state changes; absolute absence of incoming traffic is therefore not a valid general ownership contract. The role boot owner writes only address `0x0`, with its other images at `0x12000` and above; this observed non-overlap is not proof that all firmware writes are restricted to `0x10`. Upstream firmware also has explicit Tensix reset/reinitialization paths, so the driver's lock alone cannot exclude all firmware control operations.
>
> The same firmware's [reset binding](https://github.com/tenstorrent/tt-system-firmware/blob/6a3c136f20a2c93d7afef46dcfa4234513a9e5b9/include/zephyr/dt-bindings/reset/tt-bh-reset.h) identifies `0x80030014` as L2CPU reset: bits 0..3 are tile resets and bits 4..7 are RISC resets. Its [reset driver](https://github.com/tenstorrent/tt-system-firmware/blob/6a3c136f20a2c93d7afef46dcfa4234513a9e5b9/drivers/reset/reset_tt_bh.c) reads these bits active-low. The existing ARC scratch view maps the corresponding `0x80030000` page when TLB201 is configured to ARC; sampling the register still needs exact route, serialization, restoration and stuck-MMIO handling. Reset status does not establish that earlier L2CPU NoC traffic drained or that firmware cannot subsequently release a core. The cited upstream source is not an identification of the firmware installed on this card.
>
> The current host TLB helper encodes `0x40` in its high word, corresponding to strict ordering in the [Blackhole TLB definition](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/PCIExpressTile/HostToDeviceTLBs.md). Tenstorrent's [baby-RISC fence documentation](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/TensixTile/BabyRISCV/InstructionSet.md) also states that a device fence alone does not guarantee a store to a different destination has completed. Continue by establishing range-specific ownership and fenced source lifetimes, rather than claiming global NoC silence from Tensix outgoing counters. No card was accessed.

> **Unconfirmed release recovery, offline:** a release attempt that has not been confirmed can now be classified as still held only when an independent proof says that particular reset write has drained and no later release is possible, and two routed reset reads show `0x47800`. Such a tile skips role/compute stop requests and is rechecked before the all-core held audit. The real write-drain proof still returns false: repeated held reads do not fence a posted write, and this change does not enable hardware recovery. Adapter simulations cover denied drain proof, held reset and a release that occurred. The non-loadable Haiku witness compiled with SHA-256 `0985fee58cefcc120db5bce457376d7027b245fb6a3645e7bf0e564a2300b5bf`. No card access or throughput measurement occurred.

> **Role-aware retained-graph audit, offline:** the fault backend now checks each retained tile's stable compute snapshot, both reader/writer terminal receipts or an idle role mailbox, and balanced queues before it can settle a graph fault. Queue failure flags are permitted only for fault auditing; live or partially received role work is rejected. Fault acknowledgement is confined to the retained audit scope and to the exact core and sequence after both all-core audit passes; the authorization is revoked when the acknowledgement loop exits. The separate physical role/NoC proof still deliberately returns false. Normal graph writes cannot use the fault path. Simulations cover pending roles, terminal failures, balanced/imbalanced queues and denied acknowledgements. The non-loadable driver SHA-256 is `9f09d220efce2d6f9c66952b0798430756ef1d2d6fc27610c60543ba40468c9b`. Physical image/NoC safety and handling unsettled queues remain incomplete; no card was accessed.

> **Retained graph recovery scope, offline:** the borrowed lock scope now admits a fault audit only for the exact context-owned graph, retained run and service-owned publisher, and can promote it to a close transaction only when that run's existing fault audit is settled. The scope is one-shot, checks thread/team/generation and exact owner/run throughout, and is explicitly ended before unlock; role memory and control writes remain denied. Simulated tests cover wrong owner/run, attempted reuse, graph replacement, premature promotion, successful promotion and a failed close followed by retry. The role-aware fault audit is now wired but its physical NoC and acknowledgement proofs are still missing, so this does not yet recover a graph on hardware. Non-loadable driver SHA-256 `8339af9efb3c967872084c0c2e4334179733213a459ac6ae9a2da4073b058abd`; no card was accessed.

> **Three-role graph I/O and completion, offline:** the shared build now uses reset `0x6000` for permitted running writes and sparse warm descriptor updates; original dispatcher save/restore retains its separate reset values. Successful graph completion and acknowledgement additionally require stable reader/writer receipts and balanced queues, while the existing compute image, route and NoC audits remain in place. Missing shared ownership never falls back to the single-role path. Tests cover both reset modes, sparse descriptor updates, pending role work, queue imbalance and failed compute. The real role-image/NoC runtime proof still rejects the path, and fault acknowledgement/retained-graph recovery remain incomplete. Non-loadable driver SHA-256 `55bf39bf106c7c536d2e10315b65f7d8fb762c77453d5d55515db8f292e1ab65`; no card access or throughput measurement.

> **Three-role runtime readiness, offline:** shared-owner `CoreReady` now checks the confirmed release, the exact worker snapshot, a stable idle role mailbox (including nonzero role sequences), cleared queues and compute PC within the generated image, instead of accepting the old `0x46800` single-controller state. The shared build always dispatches to this check and rejects a missing shared owner; generation now borrows the existing exclusive lock for its lifetime and releases it with error tracking. The actual image/NoC runtime proof still denies readiness. Later guarded graph audit and running descriptor/ack paths recognize the shared reset, but still need physical runtime and fault proofs before execution. The combined driver remains non-loadable (SHA-256 `addd65ffde67247246283ed64d45628559a5acaaac360e5042ea7a0fe0c7741f`); no card was accessed and there is no new throughput result.

> **Guarded compute stop and three-role hold, offline:** after reader/writer stop receipts and park checks, the shared binding now publishes one TRISC0 shutdown request, waits for a stopped mailbox, cleared queues and the generated compute park PC (`0x7bef8`), then records and attempts each three-role hold and audits the held fleet. Retry observes previously attempted writes instead of resending. The shared graph image build verifies that the named compute park is a self-loop. Simulated tests cover missing proofs, failed/delayed stop and hold writes, nonempty queues and wrong PCs. The real compute/hold/fleet proofs still deny access; role-aware retained-graph fault audit, uncertain release that left a tile held, cross-fleet NoC quiescence, full stack proof and physical qualification remain unfinished. Non-loadable driver SHA-256 `ae8dba9e4a221bb2a8ee09dcb715d7a096810b16341b636446aea1272e4df892`; no hardware access or new throughput measurement.

> **Guarded role-stop publication and park checks, offline:** the shared binding can now publish exactly one reader/writer stop token per released tile after a separate clean- or fault-settlement proof, retain uncertain writes, and observe a later completion without resending. It checks both role stop receipts and two B/NC PC snapshots against the exported firmware park addresses (`0x70044` / `0x700c4`); the image build verifies each address contains a self-loop. The real stop and post-stop fleet/NoC proofs still deny access. The later guarded TRISC0 stop and hold path is still disabled; held-after-uncertain-release handling and role-aware retained-graph fault audit remain incomplete, so this cannot run or recover the card yet. Simulated tests exercise clean/fault tokens, uncertain writes, receipt and PC checks; the non-loadable driver SHA-256 is `bf046a87b1e65f02e62d532f5911591f05420001f2a377de9c75e46723089ed8`. No card access or new throughput result.

> **Guarded three-role reset release, offline:** the shared role binding now has a per-tile release path that checks the pinned route, exact `0x47800` held reset and local-RAM preservation bits, records the tile attempt before MMIO, then writes the fixed `0x6000` reset value to release BRISC, NCRISC and TRISC0. It refuses a second attempt after an uncertain write. A release attempt also prevents the old TRISC-only stop path from claiming cleanup; retained state cannot yet be recovered by that path. The real release authorization remains false until a three-role stop/hold, parked-PC and NoC audit and partial-fleet recovery exist. Tests exercised all 120 simulated routes and dropped reset writes. The non-loadable Haiku build SHA-256 is `ee6e0747aa23b1f4f9e66dc942b82ebb1747b74bbd8457858d014cd5bcf83b97`; no card was touched and no throughput is claimed.

> **Driver startup receipt check, offline:** the shared-role service owner now waits on each of the 120 tiles for two stable role-mailbox reads containing both distinct startup receipts and an unchanged ready compute snapshot. It retries bounded transient snapshot or mailbox changes and rejects failed compute or missing role receipts; simulated tests cover a writer starting between reads, temporary unreadable snapshots, a missing writer and a stale completed compute sequence. The updated owner also compiles against an inert Haiku SDK probe (752-byte install frame). The combined non-loadable Haiku driver compiles with SHA-256 `42285634ce8522e75a856c97ce06ad850fdcff2d6ddf7deb09ec0250daa30459`. This check does not establish PC/NoC safety; physical role reset/release, role-aware driver readiness and fault cleanup remain disabled and no card was accessed.

> **Role startup receipts, offline rewrite:** each reader/writer tick publishes a distinct receipt only after its startup shim has disabled the L0 cache and initialized the private stack. The version-2 role mailbox remains 64 bytes; the coordinator refuses projection issuance until both receipts exist, and a startup-snapshot predicate rejects missing, swapped or non-idle receipts. Tests exercise delayed and concurrent startup and role handoffs. The generated shared firmware header SHA-256 is `c564fc7990d353f9c2e6bc746543720aac36757672810f685d1b4b2de23a8feb`; the graph text and dispatch hashes are `dfbf876580743ee663264f4185db31791296538245e483732cb5f13483053673` and `ca9a8a8780129214c2b308d800702590a1c2c204458140fa548f04bfd64d4100`. The non-loadable combined driver SHA-256 is `9718dccba621cf4926debe696744923e51f502e82f441c47a3eae4ce5e2ad6d1`. The physical startup, driver readiness audit and role-aware reset recovery remain incomplete; no hardware or throughput result is claimed.

> **Shared role owner in Haiku, fail-closed build:** the guarded Qwen35 install and close paths now retain the actual role recovery owner and its snapshot buffers, connecting the borrowed lock to Haiku mappings and the generated reader/writer firmware. The real fleet, RAM and write proofs still deny access, so role capture stops before role mappings, role TLB selection or role writes; the existing install sequence would already have held the compute fleet. Role start and readiness also deliberately fail. A partially failed unmap can remap its missing views for another audit and cleanup attempt. The non-loadable driver compiled with SHA-256 `e7934d2a39a3f837706571cb196c10e873af0ee1ceef700ca106487be2ecbb15`; its `18000/18432` conservative stack estimate is not a whole-program proof. No hardware was accessed and no new throughput was measured.

> **Role recovery-to-PCI adapter, non-loadable witness:** the actual role recovery owner now type-checks against `PioneerSessionIO`, the borrowed-lock scope and fixed Haiku writable views. The adapter translates only the exact boot/shim/code/private-RAM/`DISABLE_RESET` ranges, revalidates selected routes, samples both NoCs and reset, and rejects non-default BRISC/NCRISC/TRISC0 boot routing. Simulated tests exercise all 120 route selections, busy NoCs and each range; the real scope still rejects held-fleet, RAM-stability, memory and control proofs, so this adapter cannot operate on hardware. The full non-loadable driver SHA-256 is `c5a1c6349e5fc3e84fc31e4c34799283b667cadf1f274efa6aa807a66e5c2f22`; do not install it. Its `18016/18432` conservative stack figure does not include a real role callback path and is not a whole-program proof.

> **Borrowed install lock, non-loadable witness:** the actual Qwen35 control path now attaches a one-shot role scope after acquiring its existing session lock and detaches it before unlock, avoiding recursive acquisition during install/close. The scope validates cookie/team/thread/generation and still denies every role-memory and control write. The initial route proof rejected retained graphs; a later exact-run fault scope allows audit and promotes close only after the existing graph audit settles. The earlier non-loadable driver build SHA-256 is `32824d7e35355b3884160df227094003b9b647db3f8e975e61f791e1a2f4ac5f`; no role mapper or reset is invoked by this scope yet.

> **Writable role views, non-loadable Haiku witness:** fixed, uncached mappings now restrict writes to the captured role boot words, shims, code, private RAM and `DISABLE_RESET` bits 6–9; soft reset, PC and NoC writes are excluded. The actual Haiku session/route guard is linked, but its real scope denies every memory and control write until held-fleet, RAM-window and snapshot proofs are bound. Partial mappings remain owned until cleanup. A full non-loadable driver compile succeeded (SHA-256 `310a324b301d0637edeb08c38d08825adfddb360f9e9932bc031d3845455a7fc`); do not install it. This earlier witness predates the guarded install binding; physical recovery remains unimplemented.

> **Reproducible shared-role firmware, offline:** `build_qwen35_shared_role_images.sh` now exports the fixed-address BRISC reader/NCRISC writer code and shims as a C++ header; `build_qwen35_dataflow_service_image.sh` can separately export the graph image that actually calls the shared compute role. A simulated 120-tile install/restore now stages those actual exported reader/writer images through the service and recovery owners. Both build paths require recovery-only flags, check image bounds/instructions/stack estimates, and are not deployable services. The current shared graph text SHA-256 is `342e7e3561f85db731bb5eb3636c76468ddf51be06bf322bab5175d1b7584ec8`; no hardware run or speed result exists for these images.

> **Shared-role service lifecycle, guarded witness:** the real service owner now has a recovery-only path that calls role capture/staging while the fleet is held, initializes each role mailbox, arms a one-shot release, rechecks all resident images/boot/queues/mailboxes/barriers across the fleet, starts through a role-aware IO callback, and stops/restores role state before resuming the old fleet. Fault-injection tests exercise partial starts, a corrupt prior tile and retry after stop/restore/audit failures; a simulated adapter also joins the actual service and role recovery owners. The held-only PCI range admits the role mailbox in this witness; running writes remain denied. **The callbacks are now wired into a fail-closed Haiku build; physical role resets and a live service remain unimplemented.** The Haiku SDK compiled the owner with fake IO; its audit/install frames were 624/752 bytes, not a whole-driver stack proof.

> **Role reset preservation, simulated:** the 120-tile role recovery owner snapshots `RISCV_DEBUG_REG_DISABLE_RESET`, temporarily sets only bits 6–9 before any BRISC/NCRISC release, verifies every tile, and restores the original words after the saved role state. This uses Tenstorrent's documented local-RAM and debug-register preservation controls; it does not establish physical timing, cache/NoC quiescence or full driver recovery. A second audit after restoring the resident TRISC fleet catches overlap or misrouted writes and keeps recovery retryable. IO, control writes and role release are not hardware-bound.

> **Lifecycle integration constraint:** the existing Haiku role-view witness acquires the session lock itself and requires the resident service to be `Ready`; it cannot simply be called from service installation, which already holds that lock and enters `PhaseInstalling` before holding the fleet. Role snapshots and staging must instead be integrated into the existing locked install transaction after all TRISC cores are held and before any core starts; teardown must settle the additional roles before restoring/resuming the original fleet. `PioneerQwen35DataflowPciMemory::StartCore` and `StopAndAuditAll` currently only accept the old `0x47800`/`0x46800` reset states, so they cannot start or recover running BRISC/NCRISC roles unchanged. The newer guarded install binding still lacks role-aware startup and fault recovery.

> **Graph-to-role bridge, compile-only:** the actual resident graph executor now has a recovery-witness-only path to the shared-role projection coordinator; it maintains a separate contiguous role sequence across graph steps. The old single-controller projection is excluded from this build, and compile guards reject using the bridge without the recovery-only flag. This is not a loadable driver or running service: installing and starting roles, fleet recovery, NoC completion/visibility and actual model output remain unproved.

> **Shared-role cache policy, offline:** BRISC/NCRISC startup and the TRISC0 shared-role coordinator now fence, set and check Blackhole `cfg0.DisLowCash` before reading shared queues. Hardware reset enables that non-coherent cache; Tenstorrent's firmware disables it by default. These are non-loadable role witnesses, and the live TRISC0 lifecycle, previous state recovery and NoC visibility remain unproved.

> **Selected-tile held/NoC samples, non-loadable witness:** the real Haiku view can sample the exact all-held reset value and both NoC logical IDs, initiator doorbells and outstanding-request counters twice. Host tests cover all 120 selected routes and every sampled busy slot. This is a local status sample, not proof that the fleet is quiet, that incoming traffic has stopped, or that the private-RAM reset window has elapsed; memory access remains denied and there is no live ioctl. The new compile-only driver SHA is `3bc066501ce05df368915f2de16909266888eb70e5b84932665bc63d925ae62c`; do not install it.

> **Non-loadable Haiku session witness:** actual driver cookie/team, same-owner lock, generation, pinned BAR and selected-route types compile together in the recovery-only driver witness. Memory reads remain denied and no ioctl is registered. A full compile succeeded (witness SHA `1c1a0d39e7d73e66fc954fc72aa5f9ffd6b962c04019ca2a930fbef756c6b47d`); do not install this non-loadable binary.

> **Haiku mapping work, compile-only:** fixed read-only BAR views and a selected-tile guard now compile against the Haiku kernel SDK. Tests reject unsafe register offsets, missing memory/route proofs and wrong TLBs, and exercise partial-map cleanup and retry. The real session scope is connected only in the non-loadable witness above; no live ioctl, mapping call, write access or card operation is connected yet.

> **Private RAM recovery ledger, simulation only:** all B/NC private RAM snapshots join the image/boot snapshots; all 120 are marked dirty before a one-shot release attempt and must be restored after the RAM initialization window. The ledger performs no hardware release, and reset, cache, PC/NoC, PCI and existing TRISC0 recovery still need physical integration.

> **Composed fleet image recovery, simulation only:** code, boot words and low shims are captured before staging, then restored in boot-before-code order and rechecked across all 120 tiles; injected cross-region corruption requires a retry. Private local RAM, reset, cache, NoC, queues and physical PCI integration remain outside the contract, so this is not yet a complete driver recovery path.

> **Fleet code ownership, simulation only:** all 120 proposed extra code regions are captured before staging and verified fleet-wide after staging and restoration. Partial and misrouted-write fault tests retain snapshots and permit a verified retry. Boot words, low shims, local RAM, reset, NoCs and physical PCI integration are still outside this contract.

> **Offline startup witness:** low-L1 BRISC/NCRISC shims now link with the shared-activation roles and park on stop, but no boot vector, reset control, physical owner, cache qualification, or full restoration exists. The existing driver must not load these witnesses. See `QWEN35_TTMETALIUM_REWRITE_STATUS.md`.

> **Shared-activation rewrite candidate:** an offline alternative now packs one activation per projection, publishes one readiness ticket, and reuses the immutable cache for all weight tiles. Concurrent simulated Q4/Q5/Q6/Q8 runs pass; an injected reader failure retains the command. The role bodies compile and fit provisional slots but have no physical launch, cache-visibility proof, graph integration or hardware throughput measurement.

> **Rewrite in progress, offline:** separate reader/writer roles, queued Tensix compute and per-tile coordinator now compile for RV32IMF and pass concurrent queue-loop, mailbox and fault-retention simulations. Proposed extra L1 code slots are `0xb0000..0xb7fff`, outside the current owner's saved view. No physical role startup, memory ownership, cache/NoC proof, graph integration or hardware recovery exists yet; do not launch the witnesses. Continue the rewrite before assistant integration and throughput acceptance.

> **Latest hardware: tag0168 qualified one profile run.** Correct ` Paris.`; authentication 67.541939 s, vocabulary 13.236900 s, load 610.801086 s, prompt 18.771527 s, one decode 4.033940 s = 0.247897 tokens/s. Projection cycles fell 5.35% against tag0167; this single-token diagnostic is not a sustained benchmark and 100 tokens/s remains unmet. Guard passed; clean unload and stable driver55 generation20 independently verified idle. See `benchmarks/qwen35-dataflow-tag0168-20261122/RUN.md`.

> **Latest: tag0167 profile completed.** Correct ` Paris.` output, load 610.592493 s, prompt 19.426467 s, single decode 4.175606 s (0.239486 tokens/s). Core0 projection operations consumed 57.975% of measured cycles and sync/other 36.774%; neither is a pure matrix or barrier measurement. The guarded client exited 0, unloaded cleanly, and stable driver55 generation18 was independently verified idle. See `benchmarks/qwen35-dataflow-tag0167-20261122/RUN.md`.

> **Tested tag0168 rewrite optimization:** Q4_K and Q5_K panels skip two provably zero matrix fidelity phases; Q6_K/Q8_0 keep all four. The bounded profiler timer rejects unstable reads. Exact arithmetic, full target tests, audited image placement, and trial/normal/non-loadable witness builds pass; one correct hardware profile and clean unload are recorded above. Trial driver SHA-256 `900c4f427d63778184caeee65ed496c34b1bd8da854110fa4e536284177993f4`, client SHA-256 `6e64e2f9ff63bd26386700ed7313c09eb6a9dc3faf5d138e336aa0fae9a7bd21`, under `/private/tmp/pioneer-q35-tag0168-phases-trial-b/`. Recheck prior diagnostics and stable idle status before testing.

> **Latest milestone: two valid warm outputs, clean unload.** Tag0166 produced ` Paris.` (`11751,13`) twice. Authentication 69.030922 s, vocabulary 13.081918 s, model load 614.806798 s; prompts 19.469486/19.464716 s; single-token decodes 4.183633/4.184250 s = 0.239027/0.238991 tokens/s. Guard validated both, client exit0, clean unload, stable driver55 generation16 independently verified idle; both model files present at recorded sizes. This satisfies the two-output check but not the 100 tokens/s goal. The tag0167 cycle profiler has since run and cleanly unloaded, as recorded above. See `benchmarks/qwen35-dataflow-tag0166-20261122/RUN.md`.

> **Completed tag0167 device timing:** after rechecking idle stable driver55 generation16, this diagnostic candidate can measure one decode on core0 using BabyRISCV `cycle`/`cycleh`. It buckets embedding, residual, RMS, projection, convolution, delta, SiLU, dense attention, argmax, record fetch and authorization; the remainder includes synchronization, queue work and measurement overhead and is not a fleet-wide utilization measurement. Records are diagnostic only and published after graph completion. The full target suite and trial/normal/non-loadable witness builds pass; hardware profile completed successfully as recorded above. The worker stack envelope is 7760/8192 bytes with a 1024-byte helper allowance, not a whole-program proof. Current trial driver `/private/tmp/pioneer-q35-tag0167-profile-trial-b/driver/tenstorrent_pci_probe-driver60` SHA-256 `805d79bf27d4758d8ceb1d1df008b219693f4c7c167a7dc68965e0588cf0d2c3`; client `/private/tmp/pioneer-q35-tag0167-profile-trial-b/pioneer-q35-dataflow` SHA-256 `6e64e2f9ff63bd26386700ed7313c09eb6a9dc3faf5d138e336aa0fae9a7bd21`. Earlier tag0167 profile builds are obsolete.

> **Tag0166 first correct result:** one diagnostic produced verified IDs `11751,13` (` Paris.`), with model load 617.160613 s, prompt 19.503739 s, and one decode 4.190600 s (0.238629 tokens/s). Client exit0 and clean unload verified; stable driver55 generation14 was independently idle. The one-run guard restored safely but its final awk check had a syntax error, so it did not certify the result. The corrected two-run guard passed its Haiku self-test and both warm runs passed, as recorded above. The 100 tokens/s goal remains unmet. See `benchmarks/qwen35-dataflow-tag0166-20261122/RUN.md`.

> **Latest hardware result tag0165:** graph deadline at step8 with zero output after 1.339335 s prompt work; no decode rate. Same-owner recovery returned closed with no recovery required, and the client unloaded cleanly without reboot. Stable driver55 generation12 was independently verified idle with no diagnostic, owner or lock; both model files present at recorded sizes. The build still used incorrect integer conversion for FP32 output; test tag0166 next. See `benchmarks/qwen35-dataflow-tag0165-20261122/RUN.md`.

> **Prepared tag0166 readback fix:** tag0164/0165 read projection FP32 values with config3 set to signed-integer conversion (`0x10000`), so their zero Dst traces cannot establish the underlying FP32 contents and output readback is wrong. The rewrite now switches config3 to `0` after the SFPU drain for FP32 diagnostics/output, then restores `0x10000` for the next integer matrix-dot check. Full target suite and trial/normal/non-loadable witness builds passed; the first hardware result is recorded above. Trial driver `/private/tmp/pioneer-q35-tag0166-fp32-trial/driver/tenstorrent_pci_probe-driver60` SHA-256 `570b17c4674843e6aea6527671b3ec50200ca757872661d481f3c3dc8be3987d`; client `/private/tmp/pioneer-q35-tag0166-fp32-trial/pioneer-q35-dataflow` SHA-256 `55cc323f6c59e5c6f357c98477f441d29bd11fbc0de4114e50c1b729f680a126`. Tag0166 has since produced one correct output and unloaded cleanly; the two-run check is in flight.

> **Latest hardware result tag0164:** layer0 QKV source coefficient and matrix dot samples matched the independent reference. Dst samples read as zero, but config3 was in integer-conversion mode, so they are inconclusive about actual FP32 values. Output `0,0` failed correctness; load 610.548229 s, prompt 19.384042 s, one decode 4.174285 s (0.239562 tokens/s, invalid output only). Clean unload and driver55 generation10 independently verified with no process, owner or lock; both model sizes intact. See `benchmarks/qwen35-dataflow-tag0164-20261122/RUN.md`.

> **Prior tag0165 affine and unpacker fix:** the tag0164-and-earlier worker copied coefficients over minima due to a 512-versus-576-byte panel-prefix mismatch. Tag0165 places actual fields directly at `0x91200`/`0x91240`. Tag0164 also sampled zero Dst rows in integer-conversion mode, which is inconclusive for FP32. Independently, the rewrite now restores configuration register73 to `0x13` before SFPU metadata unpack, matching the fixed-format prerequisite, after the matrix has drained. Both changes pass focused tests, the full target suite, and trial/normal/non-loadable witness builds; the run faulted and recovered as recorded above. Its trial driver `/private/tmp/pioneer-q35-tag0165-affine-selector-trial/driver/tenstorrent_pci_probe-driver60` SHA-256 `e9d726d2a7654e4e2f63ab0bbf597b79244a41642a8735d158016028df4d3ebc`; client `/private/tmp/pioneer-q35-tag0165-affine-selector-trial/pioneer-q35-dataflow` SHA-256 `55cc323f6c59e5c6f357c98477f441d29bd11fbc0de4114e50c1b729f680a126`. The earlier tag0165 affine-only build is obsolete. Tag0164 unloaded cleanly and stable driver55 generation10 was independently verified idle; recheck before the next card test.

> **Prior candidate tag0164:** diagnostic-only additional Dst samples after the first panel SFPU accumulator: coefficient row96, minimum row100, accumulator row108. Offline reference predicts nonzero accumulation, but its ordinary host rounding is approximate. Full target suite and trial/normal/non-loadable witness builds pass. Trial driver tag `2026112100020164`, `/private/tmp/pioneer-q35-tag0164-accum-trial/driver/tenstorrent_pci_probe-driver60`, SHA-256 `ea86c72652595ae36d34bd877068050c78b00ba3370d4ac2e8a709f3a8217e55`; client `/private/tmp/pioneer-q35-tag0164-accum-trial/pioneer-q35-dataflow`, SHA-256 `55cc323f6c59e5c6f357c98477f441d29bd11fbc0de4114e50c1b729f680a126`. Its failed hardware result and clean restoration are recorded above; do not redeploy it.

> **Latest hardware result tag0163:** core0 packed activation, affine coefficient and Tensix dots `1530,3395` match the independent offline reference. The sampled projection output is still zero; output `0,0` fails correctness. Model load 611.043880 s; prompt 19.239290 s; one decode 4.142027 s (0.241428 tokens/s, invalid output). Clean unload and stable driver55 generation8 independently confirmed with no owner, lock or diagnostic; both model files present at recorded sizes. See `benchmarks/qwen35-dataflow-tag0163-20261122/RUN.md`.

> **Prior rewrite candidate, tag0163:** added the qualified Tensix source-window zero initialization and initial destination clear missing from the rewrite. Four extra diagnostic slots capture the first projection core0 packed-activation words, scale, matrix dots, and affine coefficients on-device; these records are diagnostic-only and published after step completion. Its failed hardware result is recorded above. Trial driver `/private/tmp/pioneer-q35-tag0163-matrix-trace/driver/tenstorrent_pci_probe-driver60` SHA-256 `b28ddf2cb1f71929ce71637cb44025d43ca0747a737e99fcf5de51491a9f2649`, client `/private/tmp/pioneer-q35-tag0163-matrix-trace/pioneer-q35-dataflow` SHA-256 `4cf64874bc11c1f2ab81374632081a002fc5216b843c172f31c8f84ffc8b13f9`. Earlier tag0163 init-only builds lack the trace and are obsolete. That candidate ran with the results recorded above; do not redeploy it.

> **Latest hardware result: tag0162 found zero projection samples.** The token0 embedding rows16/31 match independently decoded GGUF values and layer0/final RMS samples are nonzero; sampled first QKV, gate, and final vocabulary projection pages are zero. Output `0,0` (`!!`) fails the verified prefix. Authentication 67.472414 s, load 611.687363 s, prompt 19.208158 s, one decode 4.134968 s (0.241840 tokens/s, invalid-output diagnostic only). The model unloaded cleanly and stable driver55 generation6 was independently verified idle with no lock or diagnostic; both model sizes remain intact. See `benchmarks/qwen35-dataflow-tag0162-20261122/`. Investigate the Tensix projection path; each sample is only one 16-value panel.

> **Latest hardware result: tag0161 completed generation with incorrect output.** The SiLU software divide passed the former convolution stall and the graph produced token IDs `0,0` (`!!`) rather than verified `11751,13` (` Paris.`). Model load took 612.118705 s, five-token prompt processing 19.186788 s, and the one subsequent decode 4.130616 s, reported as 0.242095 decode tokens/s. This is a failed correctness diagnostic, not an acceptance throughput result. The client unloaded cleanly and restored stable driver55 generation4 without reboot; independent checks found no owner, diagnostic process or lock, the stable SHA-256 `041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403`, and recorded 4B/27B file sizes. See `tools/pioneer/benchmarks/qwen35-dataflow-tag0161-20261122/`.

> **Prior candidate: tag0162 numerical trace.** This candidate includes the rounding-independent SiLU follow-up and records raw output-page samples from core0 at steps 0, 2, 3, 4, 7, 9, 545 and 546 into separate saved L1. It exposes these samples and the audited argmax score only through bounded diagnostic queries after successful graph completion; they are never used as inference or release proof. A hash-matched 32 MiB GGUF prefix was copied read-only and `tools/pioneer/qwen35_token0_embedding_reference.py` independently decoded token0 rows16/31 as `0xbd2f21c0` and `0x3cb4c800`; compare these only if the final graph's input token is 0. The full target suite and normal, trial and non-loadable reset-witness builds pass. Trial driver tag `2026112100020162`, SHA-256 `40928b58024724081781f581b6db8d2a79203a182da3f449b3649a118d8c1aa5`; client SHA-256 `d5fdc6924ad7a3452e9c025fa1ec85280cc3d5e28d0308ea2784c2a965a848d6`; artifacts `/private/tmp/pioneer-q35-tag0162-trace-b`. Earlier tag0162 base artifacts are obsolete. Hardware result and clean restoration are recorded above and in the tag0162 evidence.

> **Scope and recovery:** only the Tenstorrent-style rewrite is in scope until complete; performance acceptance and assistant integration follow. Recheck prior diagnostics have exited and the stable driver is idle before card access. The bounded same-owner recovery retry can help only when workers settle; ASIC-reset recovery remains unqualified. Use the NVMe Fedora-to-Haiku path and no SD card operations. Earlier diagnostic evidence is preserved in `benchmarks/qwen35-dataflow-tag0158-20261122/` through `tag0161`.

# Qwen3.5-4B Pioneer bring-up: next-session handoff

Continue the active goal from checkpoint `94117b2279` and
[`QWEN35_4B_BRINGUP.md`](QWEN35_4B_BRINGUP.md). Finish with two valid warm
Work outputs on Haiku, Q4_K_M projections using all 120 Tensix cores, no CPU
inference or fallback, at least 100 decode tokens/s, separate model-load,
prompt-processing, and decode timings, clean Work unload, intact Qwen3.8-27B
setup, and the tested checkpoint pushed to GitHub. Historical Tag70 measured
0.115 tokens/s; Tag96 produced no valid output. No run has reached 100 tokens/s.

## Current checkpoint: tag0153 graph qualification failed

The first tag0153 hardware qualification on 2026-11-22 authenticated the
pinned 4B model and installed the all-120-worker resident graph. Model load
took `607.268323` s for `2740498432` resident bytes; authentication took
`101.192867` s and tokenization took `13.030747` s. The first prompt graph
failed after `0.902885` s with generation result `31`, stage `32770`
(`0x8002`: graph session retained), zero output tokens and no decode time.
This does not identify whether staging, publication, or completion failed.
It is not a throughput result. The no-reboot graph recovery attempted to close
the service but returned resume failure and then stop-audit failure; the
wrapper retained the candidate and lock with `cold1 active_owner1` rather than
restoring unsafely. Logs and the exact one-shot wrapper are in
`benchmarks/qwen35-dataflow-tag0153-20261122/evidence/`. A single authorized
OFF hold was sent after syncing, followed by one ON pulse and the pinned NVMe
Fedora-to-Haiku handoff. The guarded recovery restored the stable driver, and
an independent check reported driver55 generation2, `cold0 active_owner0`,
the expected stable SHA-256 `041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403`,
and no recovery lock. The 4B and 27B files remain present at their recorded
sizes; these size checks are not fresh full-content hashes.

Only the Tenstorrent-style rewrite is in scope until complete. Diagnose the
graph fault and the failed in-place recovery before another qualification;
benchmark acceptance and assistant integration follow afterward.

Tag0154 adds publication/completion/commit failure stages (including failed
core), exposes service-close details on recovery responses, and retains the
settled-graph proof across a failed close. After snapshots are restored, a
retry repeats only the fleet resume audit if all starts were confirmed; an
uncertain partial start remains retained. Fault-injection tests cover the
retry, absence of repeated reset writes, and refusal to retry partial starts.
The RV32IMF target/simulation suite, runtime recovery tests, and normal,
qualification and non-loadable witness builds pass. This is not yet hardware
proof. The qualification driver tag is `2026112100020154`, SHA-256
`152edda1d9214b869eb74299010d1e973da91c6070e7f8f191a4cbb6f597c223`;
the client SHA-256 is
`21e63fed6cdcc05bc3b168d022042245ba3e6eae0c153aaaeb49ad9a23c2f564`.
The checked worker stack envelope is `7632/8192`; the ioctl estimate is
`13856/18432` including the helper allowance, not a whole-program proof.

The guarded tag0154 one-run qualification completed with no token. After
`618.119882` s model setup, the first prompt failed after `0.912492` s with
stage `0x5200`: publication completed, but worker 0 did not pass completion.
The no-reboot recovery reached the final restored-fleet audit and reported
stage 3/core 1/detail `0x111` on each attempt, then left the card cold and
owned. This identifies a stale service-image cache: the original dispatcher
and the dataflow service share boot word `0x0006c06f`, so the cached service
proof remains true after the original image is restored. The rewrite now
invalidates it before any held-core write and before restoration, including
partial failures. This fix is not yet qualified on hardware.

Tag0154's logs and guarded recovery script are in
`benchmarks/qwen35-dataflow-tag0154-20261122/evidence/`. After one OFF hold,
one ON pulse and the pinned NVMe handoff, the guarded script restored stable
driver55 generation2. An independent check confirmed `cold0 active_owner0`,
the expected stable SHA-256, no lock and both model files at their recorded
sizes. This was a correctness diagnostic, not the two-run performance
acceptance.

Tag0155 invalidates the cached service-image proof before held-core writes and
snapshot restoration. A device fault now records its graph reason, completed
step and peer failure in the existing mailbox; the host exposes the retained
worker snapshot through a CPU-only query before recovery. A timeout can still
leave the detail unavailable. The RV32IMF suite and normal, qualification,
and non-loadable witness builds pass; no hardware run has tested this fix.
Qualification tag `2026112100020155` driver SHA-256:
`517c14d423493fe7f5fb8b33223da45076dff0d2b69e2b4a73bfcb04a537ee3f`;
client SHA-256:
`0ebd53497aeb455cf0d069cb43b61f62db6f12ece9520a87bd904bb3540dd988`.
The worker envelope is `7648/8192` including the 1024-byte allowance and the
graph ioctl envelope is `16000/18432` including a 2048-byte helper allowance;
neither is a whole-program proof.

The guarded tag0155 run failed before producing any token. Worker 0 reported
`DataflowGraphFetchFault` on the first graph record, before completing step 0.
The corrected same-owner graph-fault recovery succeeded without reboot: it
closed the service and model bank cleanly, then the wrapper restored stable
driver55. Independent status showed no owner or lock and both models at their
recorded sizes. The wrapper reported stable generation4; the first independent
check reported generation5, and two further checks remained at generation5.
The reason for the intervening increment is unknown. This demonstrates
recovery for this settled graph fault only. See
`benchmarks/qwen35-dataflow-tag0155-20261122/RUN.md`.

Tag0156 adds staged fetch diagnostics, including the NoC transfer stage,
counter-match mask and initiator-busy mask, without changing the inference
path. The target suite and all three builds pass; it has not run on hardware.
The qualification driver SHA-256 is
`b1c63ae8b7078d0d8cc11445e7acb4c3ca99ca45dd5183036a3eddf34577aa1c`,
and the client SHA-256 is
`a4de8f624f48c363f6f4e4bf9b97368e4a53deb8ad289e7dd56561b208a4270d`.
Its worker stack envelope is `7600/8192` including the helper allowance and
its graph ioctl envelope is `16000/18432` including the helper allowance;
neither is a whole-program proof.

Tag0156 identified the first-record fetch failure as a NoC counter timeout at
unsupported register `0x20c`; the supported counters matched. The same-owner
recovery and bank unload succeeded without reboot, then the wrapper restored
stable driver55 generation7. The lock is absent, driver hash is stable, and
both models retain their recorded sizes. See
`benchmarks/qwen35-dataflow-tag0156-20261122/RUN.md`.

The next rewrite no longer reads or waits on the unsupported counter. It
retains supported counter, command readback, idle and outstanding-request
proofs. Host fault-injection tests cover reads, writes, wraparound and a
supported-counter mismatch; the RV32IMF suite and all three builds pass, but
hardware qualification is still required. Tag0157 qualification driver SHA-256:
`69dedf00a22b3c833fb26643fd55767db6546470b6e370465346f0d6ddbb6c88`;
client SHA-256:
`a4de8f624f48c363f6f4e4bf9b97368e4a53deb8ad289e7dd56561b208a4270d`.
Its worker stack envelope is `7600/8192` and graph ioctl envelope is
`16000/18432`, both including helper allowances and neither a whole-program
proof.

The guarded tag0157 run passed the first graph fetch and completed embedding
and residual-save steps before worker 0 failed at RMS step 2. It produced no
token. Same-owner recovery and model-bank unload succeeded without reboot,
and the wrapper restored stable driver55 generation9. Independent checks
found no owner or lock and both models at their recorded sizes. See
`benchmarks/qwen35-dataflow-tag0157-20261122/RUN.md`.

The next rewrite fixes a definite RMS bug: its 64-byte scale record was passed
from the worker stack outside the NoC's permitted local buffer range. It now
stages the record through the aligned input page and adds RMS/NoC failure
details to graph receipts. Host tests and target compilation pass; hardware
qualification remains to be completed.

Tag0158's host RMS/fault tests, RV32IMF suite, and normal, qualification and
non-loadable witness builds pass. The normalization worker and graph bridge
are linked into the verified dispatch image to keep the primary code out of
the queue region; the image build checks their placement. Hardware has not
yet verified this fix. Qualification driver SHA-256:
`60fba6994ad95b23762fa20a59db3cbefbc37ba8bf65691d5c7c8bf04e0411fb`;
client SHA-256:
`c1e42aaad89ddaec8ff0aba994f61a2b3370ef4b9cbc04456fb4b6a31770353b`.
The worker stack envelope is `7632/8192` including the helper allowance, and
the graph ioctl envelope is `16000/18432` including its helper allowance;
neither is a whole-program proof.

## Earlier checkpoint: tag0120 failure and recovery rewrite

Only the Tenstorrent-style resident device-program rewrite is in scope until
that rewrite is complete; benchmark acceptance and assistant integration
follow afterward. The standalone Haiku client and resident TRISC0/Tensix
service use all 120 Tensix cores and have no CPU inference/fallback path.

Pioneer uses Fedora on its existing NVMe and kexec to Haiku; SD cards are no
longer used. Tag0120 loaded the full 4B model, then failed the first core0
embedding read. Authentication took `99.767714` s, tokenization `12.726700` s,
model load `337.246020` s, and failed prompt processing `0.612460` s. It
produced no token; decode time and dataflow decode rate are unavailable. The
NoC0 response counter was one behind expectation, with transaction ID0
outstanding. Tag0120 preserves the failed read's remote address, route, and
size in the 64-byte worker mailbox, with a read-only `QUERY_READ_REQUEST`.

Tag0122 corrects a concrete NoC route defect in the resident rewrite. The old
code read mutable command register `0x14` as its own NoC coordinate; the
worker and driver now read logical-ID register `0x148`, and read/write
commands use marked static VC1. This could explain the missing embedding
response, but has not been verified on the card. The host protocol fixture,
RV32IMF role compile and Haiku/RISC-V target build pass. The tag0122 default
driver SHA-256 is
`6a4c03b152b01ec623b078755992b8fc342a558af41567e2d8775a1e9b2c9957`.
Tag0123 additionally masks the logical ID's software-available upper bits,
checks the NoC0 identity against each selected worker, and permits documented
hardware changes to reserved `NOC_CTRL` bits when comparing completed
commands. The NoC1 logical-route and destination-translation assumptions are
not yet verified on hardware. The tag0123 normal driver SHA-256 is
`5163d79689ce225100913bbc26b4bc8329e9de22642d5641808c805de839130f`.
Tag0124 adds a read-only NoC1 audit for all 120 workers and the seven bank
routes before the service starts; it rejects identity, translation, drift or
busy-state mismatches. The fixture and Haiku/RISC-V builds pass, but no card
run has verified the audit. Its normal driver SHA-256 is
`3336b78873687f2c491314751e39e0f2942a0bcf0dfccd2d3f9bcdf4f4cc79ea`.
Tag0125 adds a post-reset ARC state0 exchange on firmware queue 0 to the
disabled recovery prototype. Simulated tests cover wraparound, dropped
writes, timeout, bad responses, partial mappings and retained resources; the
normal 27B application-queue tests still pass. This is not a complete or
hardware-qualified recovery sequence. The normal tag0125 driver SHA-256 is
`fc736ae48530f37785f11ca2be56626077e549cae05e3a321db841ecd9caa366`.

Tag0126 connects the internal PCIe DBI payload setting to the non-loadable
recovery compile witness. It captures a stable value through the healthy
session's audited PCIe route before workers start, and requires that snapshot
and matching PCI configuration before reset. After reset it audits the held
fleet and firmware HDMA, restores through a fresh mapping, and retains the
owner and mappings if a write, route, or TLB restoration is uncertain. It
preserves unrelated control bits and writes zero to the status half. Simulated
tests exercise the extracted session adapter, fresh mapping, dropped writes,
busy DMA, route drift and the recovery operation. No tag0126 code has run on
the card; complete PCI capability and firmware recovery, resumable failures,
and physical qualification remain open. Recovery remains disabled in the
loadable build. The normal tag0126 driver SHA-256 is
`51b8ccacdb2efaa1a7d65588040aa9e8a835fe265463d0eedb48c331660faad3`;
the non-loadable recovery witness is
`43bd02e3763c49866f24ebbd9fbef9236ea0edb649d82c914f7981ec0975979f`.

Tag0127 removes the repeated 576-byte local activation copy before every
weight block in the resident projection worker. The Tensix path reads each
already-packed, immutable activation chunk directly from its bounded L1
cache. ASan/UBSan checks cover all chunk bytes for Q8_0, Q4_K, Q5_K and Q6_K,
including Q8's eight blocks per chunk; the RV32IMF service and both Haiku
builds pass. This is an offline change, not a performance measurement.
The normal driver SHA-256 is
`f6c6ef24e5e41bbcd97a960cf85e49f458776655e1718af7eb1a4aedbeeee4e9`,
and the service text SHA-256 is
`529c47979f45426c2114241e7063b2e74fc03403a296cb0afd4a19fbe8492ff9`.

The main remaining throughput architecture issue is dispatch. The current
decoder executes 548 separate resident worker operations for a generated
token, each of which the host stages, publishes, audits and acknowledges per
core; this is a source-level count, not a timing result. Continue the rewrite
toward device-side queued graph execution with bounded per-core descriptors,
device-side synchronization and fault receipts, while retaining all 120
matrix workers and validated model ownership. Tenstorrent's
[dispatch kernel](https://github.com/tenstorrent/tt-metal/blob/main/tt_metal/impl/dispatch/kernels/cq_dispatch.cpp)
and [prefetch kernel](https://github.com/tenstorrent/tt-metal/blob/main/tt_metal/impl/dispatch/kernels/cq_prefetch.cpp)
show device-consumed command queues and bounded staging, not a claim that
the current Haiku implementation already runs TT-Metalium.

Tag0128 adds the first device-dispatch synchronization component. A bounded
NoC1 peer transport admits only the per-worker receipt and release slots;
worker 0 remains a compute worker and collects one committed receipt from all
120 workers before sending releases. Payload and commit are separate ordered
64-byte transfers; stale or torn commits do not release work, and transport
uncertainty requires retaining the full graph. A partial release can leave
some workers advanced and must be treated as a whole-graph fault. The existing
saved L1 view covers these slots; their clear/readback while cores are held is
compiled only into a non-loadable dispatch witness. Simulated tests cover all
120 peers, failed and partial sends, stale/torn markers, explicit timeout,
held-core initialization and full L1 restore. The concrete NoC transport also
compiles for RV32IMF without atomic instructions or runtime helpers.

This is not yet a working queued graph: trusted descriptor-stream storage and fetch,
fresh-epoch ownership, worker service integration, bounded deadline and fault
propagation, and silicon validation are still open. Neither the barrier nor
its peer NoC writes have run on the card. Normal tag0128 driver SHA-256:
`6ae463bb56390e8e1693c57bbdf9b11684b54742e74b379e598d322e1af8b4b7`;
non-loadable dispatch witness SHA-256:
`dfa3472ad3714c1276bc88e072185c9a3c8574278eb16dce41e466c89f6de1e1`;
service text SHA-256:
`b544ef4561f1ec4463b9b73f55a05ead65c81becd7b610aed9634840c1cb35d1`.

The device-dispatch rewrite now also has a stream-format and workspace-plan
prototype. Each core's 2 KiB records hold an existing typed invocation, epoch,
step, core, and corruption checksum; the NoC0 fetch reuses the idle second
weight page and publishes command magic last. The prior command is invalidated
before a read, so an unsuccessful fetch cannot leave it runnable. The planner
reserves disjoint per-core ranges inside the packed workspace after the rotary,
activation and graph-scratch ranges; 548 full records on all 120 cores require
134,676,480 bytes before other workspace use. Synthetic workspace tests check
all 120 leases, overlap, capacity, forged ranges and record corruption;
embedding and projection records round-trip. The concrete fetch compiles for
RV32IMF without runtime helpers or A atomics. The checksum detects accidental
corruption; it does not authorize model access. The kernel must independently
verify workspace ownership and each command, keep the stream immutable, and
bind epochs before the worker can use it. The resident service does not yet
execute this stream; dynamic token/position patching, worker integration,
completion/fault handling and hardware validation remain open. Rebuilding all
records every token would also move about 135 MB per token, so the final path
must reuse fixed records and update only dynamic fields.

An offline link witness places the current worker image in `0x72000..0x7ffff`
and the stream/barrier compile probes in `0xa0400..0xa9fff`, between the
Tensix destination-init window and the invocation slot. Both ranges lie in
the existing saved L1 snapshot. The witness links 54,452 bytes of primary
text and 7,308 bytes of dispatch text within their respective limits, with no
unresolved imports. Tag0129 adds bounded installation and readback of both
regions while all cores are held, includes the second region in image identity
and program-counter checks, and refuses writes to it while workers run.
Injected write failures, failed or corrupt readbacks and the held/running
access rules pass simulated tests. This installation exists only in the
non-loadable dispatch witness: its second section contains compile probes,
and the resident worker does not execute the queued service. The normal
tag0129 driver SHA-256 is
`8f7a25dc4b2cbddeaf8b6d691a9342386f7e8e5e4c9d565f7b3e766f4ca5eb31`;
the non-loadable witness SHA-256 is
`22f4d211d27e3ae0a49c6040a6a3b02871b55e94c98d2fc1997f85f8f6248072`.
Neither has run on the card.

Tag0130 extracts the existing worker's single-command executor from the
host-driven service loop so a future queued service can reuse the same Tensix
and auxiliary operations. The caller remains responsible for proving command
authority, publication, fleet coordination and fault ownership; extraction
does not supply those checks or connect the stream. The resident image is
54,536 bytes, within its 57,344-byte allocation, and the RV32IMF and simulated
role tests pass. Normal and non-loadable Haiku builds pass. Normal driver
SHA-256: `66fc673b05a400cf090de5d621840c1ef73a42c68b1222b1d7c21c24d766d75d`;
non-loadable witness SHA-256:
`c9b51667a271c350701729e240af5f90af06d50ddcb7e0a7b8249635d15a27e8`.
The refactor has not run on the card and establishes no new token rate.

Tag0131 adds a per-worker graph control loop that fetches, authorizes and
executes each command, then waits for the 120-worker barrier. It uses bounded
poll counts and retains the graph on failed fetch, authorization, execution,
barrier, timeout or queue reset. A successful barrier may release some peers
before a different peer fails; a local completion is therefore not a fleet
completion proof. Simulated tests run two actual encoded embedding records on
120 workers through stream fetch and barrier logic, using a fake executor and
fake authority; they exercise failed fetches, denied authority, uncertain and
reported execution failures, partial release and timeout. They do not verify
model math or hardware operation.

A non-loadable RV32IMF witness links this control loop to the real NoC fetch,
barrier and extracted worker executor, with 54,652 bytes of primary code and
8,164 bytes of dispatch code. Its authorization hook rejects every command and
the resident service never enters the graph. Kernel validation of every exact
model-bound command, immutable stream ownership, fresh non-reused epochs,
dynamic token/position handling, fleet completion proof, fault reporting and
physical qualification remain necessary. Reusing an epoch/step can accept a
stale barrier marker; do not reuse one. A full stream per token is too large to
upload repeatedly. The normal tag0131 driver SHA-256 is
`9046b4e81acb393102d79c6c4c1b533819cde9f1b0b2e579393fff7caa5e947c`;
the non-loadable witness is
`8d40af3f685cb64b584da5841cc8faa86cfb7c6f46b28d4b677f6b596e19d362`.
Neither has run on the card; there is no new decode-rate measurement.

Tag0132 adds a metadata-only stream recorder using the existing decoder
sequence and a separate planning copy of the live model authority. It builds
each model-bound program, binds and encodes each of the 120 invocations, and
advances only the private candidate state. A sink interface demands verified
writes and a seal; no real storage or sealing backend is connected. It must
never promote the candidate to live state without a complete physical audit.
Synthetic full-shape model tests record 548 steps across 120 cores for two
successive positions, cover all nine command types, reject forged worker IDs
even with a recomputed checksum, and exercise failed writes, begin and seal.
They use a fake sink and do not execute model math. The tests exposed and fixed
an RMS record bug: its core field is at a different offset from the other
invocations. The RV64 kernel-shaped compile witness reports a largest single
frame of 3,488 bytes; a full call-chain stack audit is still required before
integrating it into the driver. The witness remains non-loadable, and its
device authorization hook still denies all commands. The current encoder
would regenerate about 135 MB per token, so efficient dynamic updates and
non-reused barrier epochs remain required.

Normal tag0132 driver SHA-256:
`b082707b8dafdfc3e2e76bd2b99349d7d02a6bd4a7180114ee54ff71448aa1b4`;
non-loadable dispatch witness:
`3ffaf3a86e85d45aaa3a067f4d9f9ffc2a0d8e410c89b1e9ca2a65c9db60c313`.
The recorder is a separate compile/simulation witness and is not connected to
either driver. No tag0132 code has run on the card.

Tag0133 adds a private page sink in the non-loadable witness. It derives the
workspace starts from the model authority's already-validated manifest,
revalidates the 120 stream leases, checks the initialized-page ledger, and
claims those ranges in the seven-bank owner. Two 2 KiB records share a 4 KiB
page; an odd final record is zero-padded. The existing resident page transfer
does full device readback. Ordinary bank range/page access refuses the
claimed ranges, and the claim's owner cannot write after sealing. Failure of
a transfer quarantines the owner. The actual driver adapter is compiled only
as a non-loadable witness; no request invokes it and the recorder is not yet
connected to it. A simulated-hardware test exercises the real bank resolver
and page transfer, page pairing, odd records, overlap/initialized-ledger
rejection, sealed access and an uncertain transfer. The existing seven-bank
control tests and both Haiku builds pass. The generic interlock only covers
bank-API access: audit every other mutation path and retain the owner, model,
stream, and fresh epoch through execution before treating storage as sealed.
Efficient dynamic updates and physical qualification also remain open.

The tag0133 normal driver SHA-256 is
`761d8c9458d7f8d80a2ccfbde53987129edbb76ff203f0d9ebc659cf08152108`;
the non-loadable witness is
`b717dfff0fee41c63f7fc6861e483a47ff0edadccd85e4c1fe157e832aabf1a4`.
Neither has run on Pioneer; there is no new decode measurement.

Tag0134 connects the metadata recorder, bound-workspace validation, private
page sink and resident transfer in one non-loadable driver compile witness.
The full synthetic graph test records 548 operations for 120 cores through
the real page sink, bank resolver and simulated transfer, and reads back the
first and last pair for each core. That is 32,880 verified page writes per
token with the current format: it is not a viable decode strategy for 100
tokens/s. Another test covers the 545-operation prompt-only sequence. Public
bank uploads, span uploads, legacy store/stream/bulk uploads and legacy
Qwen35 dataflow actions now refuse while a stream claim exists; the actual
bank control fixture checks that outside-lease model writes are rejected.
Other diagnostic and lifecycle paths still require a complete ownership audit.

The combined non-loadable build reports a conservative recorder stack
envelope of `17408/18432` bytes including a 2,048-byte allowance. This is
not an audit of a real ioctl caller; the normal dataflow envelope in the same
witness is `18160/18432` and the headroom is limited. The owner is not wired
to a request, graph launch or clean session release. Its device authorization
hook still rejects all commands; an actual lifetime/epoch protocol, fleet
completion and failure audit, efficient fixed-record reuse and physical
qualification remain necessary. The normal tag0134 driver SHA-256 is
`8966cc4906c61d36dfc8c76fec02c1917f034a190bbaffba0426d41b93717dc5`;
the non-loadable witness is
`a41ba66116553b520bc08ca37d152faad379e5687f9142b422eb582b6aae8514`.
Neither has run on Pioneer and there is no new tokens-per-second result.

Tag0135 prototypes reusable template records. Each worker fetches a sealed
template into local L1, checks it, and patches the token/embedding address,
position and recurrent ping-pong selectors in the private staging copy. A
synthetic full-shape test compares every one of the 548 operations on all
120 cores against newly generated model-authorized plans at positions 1 and
2, including a token in another embedding shard. Typed-builder tests also
cover the final context position, last vocabulary token, and active/inactive
cores. A failed patch leaves the command magic unpublished. The worker link
witness uses 54,652 bytes of primary and 9,996 bytes of dispatch code; it
remains non-loadable and its authorization hook still rejects all commands.

Template epoch is now distinct from each run's barrier epoch and sequence
range. RMS scale and argmax receipts use the command sequence, so recycling
the same sequence on a later token could accept stale data. A session-ticket
prototype allocates monotonic, non-overlapping ranges and refuses exhaustion;
it is not yet bound to the live session or an audited graph lifecycle. The
run's token, position and embedding row table must be constructed from the
live validated model, published immutably to every worker and tied to those
fresh tickets. Completion/fault receipts, guarded teardown, mutation-path
audit, hardware qualification and measured throughput remain open. Records
are still fetched from device memory each operation; no physical performance
result exists. The normal tag0135 driver SHA-256 is
`90ef29e142c91a321b32d48dd2a99ae464345969ccb5c1bc3f2dce9dbe752d67`;
the non-loadable witness is
`93fd0da9854f76674f12980c0d26288458e1d8701dea0057daf3257e724ffe27`.

Tag0136 adds a fixed 512-byte per-core graph-run descriptor in the saved L1
view at `0xab100`, after the barrier release slots. It contains the template
binding, lease, audited routes, token context, fresh run ticket, trigger and
poll bound. A corruption checksum is not authorization. The non-loadable
resident-worker witness recognizes a graph marker, validates a local copy of
the descriptor and enters the graph loop; its per-command authorization hook
still rejects every command. The host-side publisher preflights all cores,
stages and reads back every descriptor and marker, then publishes triggers.
A failed stage or partial publication retains the graph. Simulated tests
cover descriptor corruption, staged readback failure, partial trigger and
L1 write bounds; no worker has executed this on Pioneer.

The split worker witness now uses 55,812 primary and 10,220 dispatch bytes.
Its build calculates a conservative `7824/8192` worker-stack envelope with
1,024 bytes allowed for small helpers; it is not a whole-program stack proof.
The non-loadable driver's conservative recorder stack envelope is
`17424/18432` with a 2,048-byte allowance and no real ioctl caller. Both
have limited headroom. The publisher is not connected to a live session,
and sealed ownership, trusted dynamic context/ticket publication, worker
authorization, full fleet completion and fault/teardown auditing are still
required before physical use. Normal tag0136 driver SHA-256:
`7e89661ffae9792945a07b4583a2de876e588e0653ec084d2e1666d3e15e2951`;
non-loadable witness:
`7984c3bd895bf9341289537ed2d1f6128a73a1ad479e55b74df860fca64fcf03`.
There is no new throughput measurement.

Tag0137 corrects the ready check to accept a fully acknowledged worker with a
nonzero sequence while rejecting stale snapshots, shutdown and unexpected
reserved fields. Its non-loadable graph completion prototype audits all 120
terminal workers and the stable decode argmax before any acknowledgement;
faults and partial acknowledgements retain the graph. Simulated prompt,
decode and failure tests pass. The real ownership gate denies every graph,
and no live model state is committed. Complete immutable session ownership,
worker authorization, exactly-once state commit, and cleanup before hardware
qualification. The Haiku/RISC-V normal and non-loadable builds and the
resident-bank regressions pass. Normal driver SHA-256:
`d11e3806d24913a1b248d612b0b06dd78413b3f324009a342464c1b83f31c9ea`;
non-loadable witness:
`4d5bddfc3b5f5e576c00ee585a24c31acfbd8af67da0af184271d34cdca52dbc`.
The non-loadable kernel stack estimate is `18160/18432`; recorder and worker
estimates have the limitations noted above. This does not diagnose the tag0120
embedding failure and is not a decode performance result.

An offline graph-metadata replay now predicts the candidate authority without
regenerating 120 per-core records. Its simulated prompt and decode states
match the full recorder across two positions while the original state remains
unchanged. The standalone Haiku/RISC-V kernel probe compiles with a largest
individual frame of 3,536 bytes; a whole call-chain bound is still required.
It is not connected to the driver and cannot promote state; connect it only
under the future immutable-session and audited-completion ownership protocol.

Tag0138 permits a 545-operation prompt run to consume the prefix of the same
sealed 548-record decode stream; decode runs use all 548 operations. Simulated
tests compare all core records for prompt positions zero and one, including
dynamic token patching, and check worker prefix termination and bounds. The
publisher now requires ownership validation before staging, including the
template's fixed activation slot. The actual callback still rejects every run;
the future session must retain and verify that fixed slot, sealed plan, model,
fresh ticket and live authority before it can authorize publication. The
target suite and both Haiku/RISC-V builds pass. Normal driver SHA-256:
`5996e2b386eea9c8df5660830186458cc2ab98e17e57ed6f76468126b03640c6`;
non-loadable witness:
`f56bd0561a08005dd5e80a77bb98fc0ffe3dcc812879f4af6c316aa431f71ae6`.
No hardware run or performance measurement has been made for this change.

Tag0139 gives the recording witness a sealed-stream identity proof: it retains
the fixed activation slot and routes, checks the full plan against the sink,
and verifies the owner and all 120 ranges against the sealed bank ledger.
Simulation covers a wrong owner, altered plan or range, changed baseline and
quarantine. This is not a service safety, model identity or ticket proof;
the publication callback still denies all runs. The existing `Unsafe()`
reports an installed service as unsafe, so the new read-only seal check does
not substitute for a future controlled resident-service safety audit. The
target suite, 27B shared-bank regression and both Haiku builds pass. Normal
driver SHA-256:
`305bed311286cb667932385e755129f7dfe08be450779d08a11d20fc71d21236`;
non-loadable witness:
`55d17dccbe446ac1adea65ff7738933b5dc241463266552a3ddb7df385db841c`.

Tag0140 adds a non-loadable resident-service graph phase. It binds a single
publisher and reserves the next mailbox trigger, prevents other work and
close during the graph, and advances the service sequence only after the
matching fleet completion audit and a successful model-state commit callback.
Only the same untouched publisher can cancel a preflight. Partial staging,
unaudited completion and commit failure retain a graph fault; ordinary
projection recovery and close do not clear it. Simulated consecutive runs
and failure cases pass, as do the target suite and Haiku builds. The actual
commit callback still denies all runs: create and own the candidate, bind it
to the immutable session, and promote it atomically only after the full audit.
Retain the publisher, snapshots and allocations on any uncertain graph.
Normal driver SHA-256:
`e90017ffe37b80e4e8f3ffa20318aaa5c7a929cf114010958b069f7eabb305cc`;
non-loadable witness:
`df9177d08b62e3989ba3b23edf41db799e3b8a4fbf92e9cb15802cc54b635788`.
No physical test or decode performance measurement has been made.

Tag0141 adds a graph model transaction that predicts a private candidate,
binds the token, ticket, fixed slot and publisher, and compares compact
snapshots of the original and candidate mutable metadata before promotion.
After the matching fleet audit it swaps the live authority pointer exactly
once; the owner retains the retired object until no references remain.
Simulation joins this transaction to the service owner for consecutive
prompt and decode steps and checks forged token layout, stale original or
candidate state, wrong publisher and missing or repeated commit. The actual
driver callbacks still deny execution: session allocation, sealed-stream and
ticket ownership, worker authorization and safe fault cleanup remain to be
connected. The target suite and Haiku builds pass. Normal driver SHA-256:
`9207eb940020793ef91e0510f7fdb8051cde16aa5854003d3f2ca3d6ddd06eb1`;
non-loadable witness:
`a2abbd831a7841ed3325e4af7f289d29e845c1ff020314c59e353a7d0cd6656c`.
The model-planning stack envelope is `9472/18432` with a 2,048-byte helper
allowance, excluding a real ioctl caller. There is no hardware throughput result.

Tag0142 joins the graph pieces into a session coordinator that exclusively
claims one sealed stream, issues non-reused tickets and runs publication,
completion and model-state commit under the session lock. Simulation covers
two consecutive prompt/decode runs, a duplicate stream claim, preflight
rejection and retained audit fault. The non-loadable driver compile witness
uses the real sealed-bank proof and PCI adapter; its controlled safety check
allows only the bound ready/running dataflow service while preserving the
other context safety checks. There is no real session allocation or public
graph request yet, and the device command authorization hook still denies
execution. Implement their ownership, teardown and fault handling before
hardware use. The target suite, 27B shared-bank regression and both Haiku
builds pass. Normal driver SHA-256:
`60f47279614f341405f471e35dcdf578d7cf0d16124676ab2eebc919447378cd`;
non-loadable witness:
`d5d1959ceadfa18e2ca0577747334dd34ea2fd8eeae04e5c7057a962e4d9ca66`.
The session stack envelope is `13552/18432` including a 2,048-byte allowance,
excluding a real ioctl caller. No new decode rate exists.

Tag0143 replaces the worker's unconditional graph-command rejection with a
local consistency check on the fetched, dynamically patched record and staged
L1 invocation. It checks core, step, structure, checksum and exact program
words before entering the typed worker. This is not authentication; the
kernel must guarantee the host-generated stream remains sealed, disjoint from
device writes and bound to the private model/session. Simulated tests cover
corruption, bounds and all nine recorded invocation families. The dispatcher
source is now in `PioneerQwen35DataflowDeviceDispatch.cpp`. The target suite
and both Haiku builds pass; normal driver SHA-256:
`ae683b12983e8fd759a0231f6216b6f85342fe72220dd76ede07352ea243f3d4`;
non-loadable witness:
`48e65c1b0c8a52e92be259c8c997b3b85f01069174fe78d7d08a9f8737b43eb5`.
The worker stack envelope is `7792/8192` with a 1,024-byte allowance and no
whole-program proof. Actual session allocation, graph request, safe teardown
and all hardware validation remain open; these checks are not a speed result.

Tag0144 introduces a context-owned graph lifetime for the non-loadable driver.
Its runner allocates each run and candidate, transfers the candidate on audited
success, and retains uncertain resources until a full fleet-restoration proof.
The context unsafe check retains faulted graph ownership. Simulation tests
allocation failures, two successful transfers and proof-gated fault release;
the real restoration callback still denies release. Haiku kernel graph objects
use explicit 64-byte aligned allocation and matching destruction because the
kernel lacks C++17 aligned delete; model authorities retain normal new/delete.
The allocator test, target suite, 27B shared-bank regression and Haiku builds
pass. Normal driver SHA-256:
`d35020cfd9200901d585f9f163e44a1d4ea3536273f301f931178819a966d2d6`;
non-loadable witness:
`f56e29a6a6ddb5e29b580a247a2c95fec5f343f7207d4d5ec201f5e320eafa00`.
Recording and graph-run stack envelopes are `17552/18432` and `13744/18432`
with 2,048-byte allowances, excluding a real ioctl caller. Connect the owner
to a graph generation request and safe clean/fault teardown before hardware;
no new decode rate exists.

Tag0145 connects graph recording and binding to install and graph execution to
the existing generation request in the non-loadable build. The fixed template
uses activation slot 0. Prompt processing includes the first selected token;
decode timing covers later tokens, so the client uses `output_count-1` for
decode throughput. Recording is included in model-load time. Simulated tests
cover ordering, EOS, invalid output, timing, two warm requests and a retained
fault. The target suite and both Haiku builds pass. Normal driver SHA-256:
`813d44299bb77faeb02bc3d45f805afbe4d499fdd9456f87aeec5bfa22557582`;
non-loadable witness:
`6d4d5c72f94fc90187df7889b4cf6022ef2dcc7b0632af991e42e746e1c149d7`.
The install and generation call-path envelopes are `16400/18432` and
`15872/18432`, each with a 2,048-byte helper allowance and no whole-program
proof. Before hardware use complete graph-fault recovery and teardown review,
then verify the prior 27B diagnostic has exited and stable driver is idle.
No new decode rate exists.

Tag0146 connects retained graph allocations to the non-loadable ASIC-reset
recovery prototype. Release requires the active reset ticket, successful
firmware/fleet/NoC/host-DMA proofs, closed temporary mapping and completed
old-owner mapping disposal. Tests exercise proof refusal, partial disposal,
graph-release failure and the real recovery operation's ordering in simulation.
The hardware reset and firmware sequence is still unqualified, and ordinary
graph-fault teardown without reset is not implemented. The recovery suite,
target suite and both Haiku builds pass. Normal driver SHA-256:
`44817035ee8ff1728fbf5052c1711c288858727cbbd2aee9dbc70058fd8db93f`;
non-loadable witness:
`48ed831cec7bf16e01ac42d9965d1539683e81c7d678307b8b8ee7f6aec0b3d3`.
The generation stack envelope is `15920/18432` including a 2,048-byte helper
allowance, not a whole-program proof. No card run or decode rate exists.

The next offline graph-fault prototype adds a bounded all-core settlement
audit and a close-only service transition. It requires a backend proof that
earlier host writes are fenced and that both NoCs, queues, routes, worker
image and graph peer/fetch traffic are safe; it checks stable worker receipts
twice before acknowledging any terminal worker. Simulated tests cover partial
publication and acknowledgement, unsettled or changing receipts, failed
fences and unsafe cores. At tag0147 the prototype was not connected to a
hardware backend or the driver recovery ioctl and did not itself restore or
release the fleet. The target suite
and both Haiku/RISC-V builds pass. Tag0147 normal driver SHA-256:
`48ba20fbd4f3118cf1d8a099f37bad8d505a61e3e45eadf3d2dfb5bed337e7ba`;
non-loadable graph witness:
`24eca47a2b71b115147bc4263a486960619b20e81827b915e36d67f1c909fa31`.

Tag0148 reduces host writes for repeated graph descriptors in the non-loadable
dispatch build. After the first fully written record, it checks the entire
previous record and its sequence against the ready worker, rejects unexpected
static changes, and writes only changed trigger, poll, ticket, token/position
and checksum words. The publisher still performs a complete descriptor
readback before triggering any worker; an uncertain or partial update retains
the graph. The fixture's prompt-to-decode example uses seven writes instead
of 128 per core. Full-record reads, per-core auditing and metadata replay are
still present; this is an operation count, not a hardware speed measurement.
The target suite and both Haiku/RISC-V builds pass. Normal driver SHA-256:
`96c8f9b2d749c54cce873be27d2b6fc184dd3bc4827a1e02c9f48c8d3d83646b`;
non-loadable graph witness:
`343a690d5ab7e6a189e1bfbee13df75f7ed53b6988e47494f23464d114f4490e`.

Tag0149 connects the graph-fault audit to the non-loadable driver's RECOVER
request. Under the retained owner and session lock, it fences prior host
writes, checks stable worker state, both NoCs, queues, routes and image across
all 120 cores, acknowledges settled terminal workers, and permits only a
service close. It releases retained run allocations only after the service
has restored its fleet and the context proves safe; a failed or partial audit
keeps ownership, and a later attempt fences and re-audits the entire fleet.
The session is never made ready for inference after a fault. Simulation covers
rejected audit, partial acknowledgement retry, close and release. This path is
still non-loadable and not hardware-qualified; outstanding traffic or other
uncertain state may prevent recovery without reset. The target and reload
test suites and both Haiku/RISC-V builds pass. The new recovery call-path
stack envelope is `6608/18432` including a 2,048-byte helper allowance, not a
whole-program proof. Normal driver SHA-256:
`fdab1921ab9ed3f439ed78d8bef826dc1c881709ed709aae3e8caca6a12b30f6`;
non-loadable graph witness:
`61063443a267b8e7b10e15a57107429e53bac6ee29b92ce4c91e74ea52e615bf`.

Tag0150 separates a dedicated, loadable graph qualification build from the
non-loadable reset witness. The graph trial uses driver tag
`2026112100020150`, explicitly excludes the unqualified ASIC-reset sequence,
and includes the graph-fault close path. The service image matches the graph
witness byte for byte. It has not been installed or run; the normal driver,
graph witness and trial builds plus target and reload suites pass. Trial driver
SHA-256: `aaf92ec23d14be2b62edadf230f1c3fc317e4d2bee67f76647a55cdb814a15f6`;
normal tag `2026112100010150` driver:
`385db06798142123d7efe56bbe8f671217f8a9525d1faab455901df8603681f1`;
non-loadable witness:
`2e7c2b32598dfa51ff026fba5750782bfe4a44f764c0cf69aba4b9935ee3a05f`.

At tag0150 the rewrite had a serious synchronization throughput issue: each
of 548 decode steps made worker 0 send two acknowledged
NoC writes to each of 119 peers, serially: 130,424 release writes per token.
At 100 tokens/s, a 10 ms token budget would allow about 77 ns per release
write even if computation, arrivals, host work and all other costs took no
time. This is a source-level count and arithmetic bound, not a measured rate.
Investigate a device-side multicast/semaphore pattern as used by Tenstorrent,
while proving actual Pioneer router masks, NoC1 coordinates, destination
membership, acknowledgement counts and payload-before-commit ordering. The
Blackhole [NoC memory map](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/MemoryMap.md)
documents multicast VC and path-reservation constraints, and the
[broadcast receiver rules](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/README.md#broadcast-receivers)
show why the actual router masks must be checked. Do not assume a generic
rectangle or unverified receiver count is safe. The trial is preparation for
qualification, not a reason to bypass the user's rewrite-first order.

Tag0151 changes release distribution to a fixed four-way tree. Worker 0
collects all 120 arrivals and sends the decision to four children; each
recipient forwards it to its own children only after validating the payload
and commit. Every worker still participates in compute. The NoC transport
admits only the exact parent-to-child edge, and the driver now checks all
worker destination translations for each sender. The coordinator's release
transfers fall from 130,424 to 4,384 per decode token. The fleet still sends
the same total number of unicast release packets, and direct all-core arrival
collection, per-token host audits and metadata replay remain. Simulated tests
cover all 120 workers, out-of-order polling, failed relay sends, forged edges,
stale/torn releases and NoC mapping drift. This is not a measured speedup.
The trial is not installed or hardware-qualified. Target/reload suites and
normal, witness and qualification cross-builds pass. Qualification driver
`2026112100020151` SHA-256:
`d3580922016123209304a033ac25ca56a25b3630688673a26714125d1cdf090a`;
normal driver `2026112100010151`:
`ac7945b2b377c2da2ffc0f4eef2b0a48b5dc59ffafe7220c3fe9acb15bcb2bb0`;
non-loadable witness:
`1fa285843b4615fffb638e451df5c1353beaf2f05d1dd19219dc18c00f0beb6d`.

Tag0152 also aggregates arrivals through the four-way tree. Each parent
waits for committed receipts from its own children before sending one
aggregate upward; worker 0 checks four arrivals rather than 119. Failure
reports must identify a member of the reporting subtree, and a release cannot
erase a failure already observed below it. Per-parent arrival slots are
cleared and read back while the cores are held. Simulated tests cover all 120
workers, out-of-order polling, subtree forgery, lost messages, partial
releases and failure propagation. The same number of unicast packets still
crosses the fleet; no hardware throughput has been measured. The worker
stack envelope is `7632/8192` with a 1,024-byte helper allowance, not a
whole-program proof. Normal, non-loadable and qualification cross-builds,
target tests and reload tests pass. Qualification tag `2026112100020152`
driver SHA-256:
`1d527077af67276ca9024f8011f91f56e9fde8b7f053fe4268000cea041da82d`;
normal tag `2026112100010152`:
`dc31e3e98d3f0fd84e2f44baecb06534d7360f87a8439aaa6e6c2ce2d4dabb70`;
non-loadable witness:
`7d0b349751e8324769fb71400eca5d23a24cd08b846201e5260c8c01fe64c121`.

Tag0153 keeps an exact session-owned ledger of the 120 staged descriptors.
The existing staging transport verifies every first-run word and each changed
warm-run word; for warm runs it first checks the complete old descriptor and
its immutable fields. Only after that proof and marker readback does the
ledger accept a descriptor. It requires the same ticket/token across the
fleet, all 120 stages and ordered single-use command publication. This removes
the two additional full 512-byte descriptor reads formerly made between
staging and each command: 30,720 32-bit MMIO reads per graph by source count.
The session lock, sealed stream, and restricted peer writes keep descriptors
owned until command publication; the full post-run descriptor audit remains.
The ledger stores 61,440 descriptor bytes in the existing aligned, heap-owned
graph object. Simulated tests exercise two runs, mismatches, partial staging,
replay and publication order. The target/reload suites and normal, witness
and qualification cross-builds pass. No card result or timing exists.
Qualification tag `2026112100020153` driver SHA-256:
`7d2319caa8c40a57913f567a13e1639b8a10f54ab2aa5ceac2bfd8793601f6b5`;
normal tag `2026112100010153`:
`f8a88cffab42bacab19c07c6aa0cf7084e0a2b85a771719dbe956fdd965c6dc0`;
non-loadable witness:
`23b00c9419c017301e7eca6dea9a11218b553c9eb3bf266194a0bcac1992d146`.

After the user restored power, strict SSH to the pinned Haiku identity
succeeded again on 2026-11-22. This was an OS identity check only; the prior
27B process and stable-driver idle preflight must be checked again before
touching the card. The active boot path remains NVMe; no SD cards are used.

Remaining firmware policy: the local Tenstorrent KMD checkout at
`22cca4cb33455d962cf8340a8f5b896dae696fd7` restores PCI state and DBI MPS,
sets MRRS to 4096, sends state0, then requests watchdog timeout message
`0xc1` (default 10,000 ms), and optionally configures firmware log DMA. Its
log buffer setup requires IOMMU translation. The Haiku prototype currently
restores the prior MRRS, sends state0, and does not set that watchdog or
enable firmware log DMA. Determine the pinned firmware's watchdog semantics
and Haiku's safe policy before sending `0xc1`; do not assume the Linux
default or introduce an unowned DMA buffer. Full PCI state restoration remains
a separate open requirement.

In the saved tag0124 recovery, Haiku was reachable at `10.0.0.247`. Following one authorized power cycle
and the pinned NVMe/kexec handoff, the saved recovery script restored stable55
at generation2: `cold0 active_owner0 open_count1`. The prior diagnostic has
exited, the recovery lock is absent, and both model files retain their
recorded sizes. The current `B_KEEP_LOADED` guard is still the legacy version;
after the new guard is staged, it requires another fresh Haiku kernel boot
through the NVMe/kexec path to activate. See
[`tag0124 recovery`](benchmarks/qwen35-dataflow-tag0124-20261122/RECOVERY.md).

Offline recovery-guard work adds a 48-byte kernel proof for reset completion,
firmware readiness, PCI restoration, rebuilt mappings, all 120 cores held,
idle NoC/DMA, and released owners. Ticket and fault-epoch checks reject
concurrent faults. A separate, fail-closed helper now encodes the exact three
Blackhole ASIC reset PCI writes and bounded reset-marker wait; fake-PCI tests
pass. It is connected to a default-disabled recovery prototype. Focused
recovery tests and the full Haiku/RISC-V build pass with stack usage
`15712/18432`. PCIe capability restoration, remaining firmware initialization,
and silicon qualification remain open. See
[`tag0119 RUN`](benchmarks/qwen35-dataflow-tag0119-20261121/RUN.md),
[`tag0120 BUILD`](benchmarks/qwen35-dataflow-tag0120-20261121/BUILD.md), and
[`tag0120 RUN`](benchmarks/qwen35-dataflow-tag0120-20261121/RUN.md).

An offline audit checks both NoCs across all 120 held cores twice and fails on
unreadable, busy, or outstanding status. The guard now publishes fault epoch
and cold state atomically; an injected fault during latch clearing leaves the
old generation recoverable. Both fixtures pass, as does the RISC-V target
build. A fresh post-reset mapping backend now validates PCI BARs, topology and
each selected core, and restores a TLB value captured after reset. Simulated
MMIO tests cover mapping and cleanup failure cases. The mapping and audit are
integrated into the default-disabled prototype and have not run on the card.
A post-reset PCI helper now restores the fixed active BAR/bridge layout with
bus mastering disabled; simulated PCI tests cover lost writes and unsafe
states. A simulated-tested audit of both PCIe pending states and the pinned
firmware's channel-0 HDMA now follows the fleet audit and restores its fresh
TLB snapshot. Recovery orchestration and owner retirement are wired into the
default-disabled prototype; remaining firmware initialization, complete PCI
capability restoration, and physical verification remain open.
The disabled prototype also captures and restores PCIe configuration-space
payload and read-request settings, with simulated failure tests. Tag0126 adds
the separate internal DBI payload shadow; remaining PCI capability state is
still required for full reset restoration.

The post-reset backend now requires a bounded ARC scratch readiness poll and
the pinned firmware telemetry and harvesting values before fleet and DMA
auditing. Simulated tests cover delayed or failed firmware, wrong telemetry,
interrupted wait, and failed TLB restoration. The new state0 queue exchange
and remaining initialization have not been run on the card.

The default build now targets dedicated tag0153. Since tag0121 it records final release of a
retained owner under the driver lock and provides a read-only, tag/generation
checked owner-lifetime status ioctl. Extracted real close/status tests and the
RISC-V build pass. The status ioctl does not reset the card or retire any
resource. A separate recovery-run prototype is disabled by default until PCIe
capability restoration and remaining firmware initialization are implemented and
the path is qualified. Do not install or run tag0121 until the required clean
Haiku boot and guarded hardware preflight.
Tag0121 now also compiles a proof-gated session-area retirement method;
simulated tests cover invalid evidence, duplicate areas, host-buffer release
ordering, and partial failure. It is called by the disabled prototype only.
The recovery guard now resolves through the existing pinned reload-guard
image path; its previous separate path could not be found by Haiku's module
resolver. A simulated-tested PCI teardown restores the idle BAR/bridge state
without replaying stale TLB snapshots. The recovery sequence is compiled in a
separate non-loadable prototype build. Hardware checks and resumable recovery
after a failed attempt remain open.

The last valid output measurement remains a legacy Work-path result at
`0.115` decode tokens/s. No dataflow run has produced a valid token or reached
the 100 tokens/s target.

## Saved Pioneer instructions and access

The root [`AGENTS.md`](../../AGENTS.md) records the user-authorized
development-box restart procedure and its limits. Follow one OFF, wait,
check power evidence, then one ON. Never repeat OFF to confirm shutdown.

Pioneer `10.0.0.247` SSH accounts, identity paths, and separate pinned host
keys are documented locally in [`PIONEER_SSH_ACCESS.md`](PIONEER_SSH_ACCESS.md).
Use strict host-key checking and the profile for the OS currently running.

## Saved tag0124 hardware state

At the time of the saved recovery, Haiku SSH was reachable and the tag0120
client had exited. The independently verified stable driver status was:

```text
P100A_RELOAD driver55 tag2026102100010012 generation2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY
```

The recovery lock has been released. Both models retain their recorded sizes:
Qwen3.5-4B `2,740,937,888` bytes and Qwen3.8-27B `18,973,870,528` bytes.
The proof-based in-place recovery operation remains disabled and unqualified;
complete it and activate its new guard before the next model diagnostic.

## Tag61: inner matrix receipt was overwritten

Tag61 passed idle, geometry, and command setup in its prior tag60 checkpoint,
then the ROOT matrix returned zero. It added shape, scale, and activation
receipts to the matrix image. The run again showed
`dispatcher_stage=0x5e000007 detail=0`, but the outer dispatcher overwrote the
matrix receipt before terminal reporting. Authentication took `67.641308` s,
tokenization `12.638489` s, and model loading `223.297109` s for all 3,450
spans. No prompt prefill, valid output, or decode rate was produced.

Recovery passed again: Work STOP proved safe idle, clean unload reported
`state=2 active=0 cold=0`, and stable041 hot-restored at generation10 without
reboot. Next copy the inner stage/detail into ROOT progress context before
the dispatcher writes its outer failure marker. See [`tag261 BUILD`](benchmarks/qwen35-work-tag261-20261119/BUILD.md)
and [`tag261 RUN`](benchmarks/qwen35-work-tag261-20261119/RUN.md).

## Tag60: ROOT matrix returned zero bands

Tag60 added checkpoints around the ROOT idle checks, matrix command setup,
matrix return count, and result copy. It loaded from stable041 generation6 at
generation7, authenticated in `68.115594` s, tokenized the 18-token prompt in
`13.258253` s, and loaded all 3,450 spans in `220.424219` s. The Work run
faulted before prefill with zero commits. Idle, geometry, and command checks
passed; `q35_work_matrix_band()` returned0 instead of160
(`dispatcher_stage=0x5e000007`). The ten-tile job is type13/Q5_K, 2,560 inputs,
one group, and route `0x492`. No valid output or decode rate exists.

STOP passed the safe-idle proof, the client unloaded cleanly
(`state=2 active=0 cold=0`), and the wrapper hot-restored stable041 at
generation8 without a reboot. The client and shared lock are gone, both model
files are intact, and stable041 is idle. See [`tag260 BUILD`](benchmarks/qwen35-work-tag260-20261119/BUILD.md)
and [`tag260 RUN`](benchmarks/qwen35-work-tag260-20261119/RUN.md). Next add
failure receipts inside the ROOT matrix routine to separate shape, scale, and
activation validation returns.

## Tag58 hardware recovery and remaining fault

Tag58 fixes the stale cached peer-nonce comparison in the second safe-idle
audit and returns pre-reset proof failures as retryable without quarantining
the owner. Its client repeats the read-only STOP proof while the driver FD is
open. The warnings-as-errors cross-build and WorkOwner retry regression
passed. See [`tag258 BUILD.md`](benchmarks/qwen35-work-tag258-20261119/BUILD.md).

Tag58 loaded from stable041 generation2 at generation3. Authentication took
`99.633210` s, vocabulary/tokenization `12.869909` s, and model loading
`223.299044` s for all 3,450 spans. It faulted before prefill with zero
commits at `dispatcher_stage=0x58778210 detail=0x4`, the pending worker119
4-byte write to remote doorbell `0x74100`. No valid output or decode rate was
produced.

The client then passed safe-idle proof, completed explicit STOP and clean
unload (`state=2 active=0 cold=0`), and the wrapper hot-reloaded stable041 at
generation4 without a board reboot. Postflight reported
`SAFE_FAULT_RECOVERY_VERIFIED`. The retryable proof-failure branch passed its
offline owner regression but was not triggered by this hardware run. The
recovery path is verified; the worker119 doorbell fault still blocks prompt
prefill and throughput measurement. Continue from the stable041 idle state;
no restart is needed to load another candidate.

## Verified explicit recovery through tag250

The latest hardware run used tag250 from stable041 generation6. It loaded all
3,450 resident spans (`2,740,498,432` bytes); authentication took `67.576945`
s, prompt tokenization `12.875780` s for 18 tokens, and model load
`222.802717` s. ROOT faulted before prefill with zero commits at
`dispatcher_stage=0x58778210 detail=0x4`, identifying worker119's 4-byte write
to remote `0x74100` with the NoC command still busy. No output or decode rate
was produced.

The Work STOP reproof and clean unload passed. Stable041 hot-reloaded at
generation8 (`cold0 active_owner0 guard1`), without reboot; the shared lock
was absent and both model files remained intact. Tag250 logs and checksums are
in [`tag250 evidence`](benchmarks/qwen35-work-tag250-20261119/evidence).

## Tag52 error cleanup

The tag52 offline build keeps card I/O out of the close callback. The client
now tracks whether BANK BEGIN, a span upload, or Work PREPARE may have taken
effect. On model-load failure it explicitly tries SPAN END, Work STOP when
PREPARE may have bound the ROOT, then BANK END before close. START/POLL errors
also flow through Work STOP and BANK END. The warnings-as-errors cross-build
and existing WorkOwner regression passed. Tag250 is the hardware proof for
STOP recovery and clean unload without board reboot; tag52 is not yet installed
on the card. Hashes are in
[`tag52 build record`](benchmarks/qwen35-work-tag252-20261119/BUILD.md).

## Tag53 hardware fault receipt and recovery

Tag53 extends the terminal Work poll result with the ROOT's last ten NoC
command words, eight counter values/deltas, and worker119's stable 16-word
status plus two PC samples. The driver reads them only after the all-120-core
safe-idle proof; there are no additional packets, resets, or retries. The
Work protocol is version 3. The warnings-as-errors cross-build and WorkOwner
regression passed. Hardware reproduced the pending 4-byte worker119 doorbell
fault with zero commits. The ROOT NoC snapshot and worker119 status/PC samples
were both valid. Worker119 status reason `0x50` is the peer image's
`panels != count` fault, but detail words 7/8 both read zero; the mismatch
remains unexplained. Authentication took `99.599927` s, prompt tokenization
`12.441273` s, and model load `224.169305` s. No decode result or valid output
exists.

The client completed explicit STOP and clean unload (`state=2 active=0
cold=0`). Tag53 was quiesced and stable041 hot-reloaded at generation4 without
reboot. The lock is absent and both model files remain intact. Full logs and
hashes are in [`tag253 evidence`](benchmarks/qwen35-work-tag253-20261119).
Continue with a diagnostic that captures the peer's nonzero job/count fields
and validates the fault snapshot mapping. Do not restore tag251's close-time
I/O experiment.

## Tag251: failed automatic close recovery

Tag251 tried to run the full fleet proof and teardown from the device-release
callback when the client exited after a terminal fault. The guarded test
reached the known worker119 doorbell fault, then `close()` did not return and
Haiku became unreachable. This automatic close path was reverted for tag52;
card I/O stays in explicit STOP/END, and client close retains the owner.

The board was recovered with the authorized single OFF/wait/ON sequence and
the pinned Linux-to-Haiku kexec path. The tag251 recovery helper hot-reloaded
stable041 at generation2; current status is idle and both model files are
intact. The raw post-cycle logs are untrusted because their writers were
interrupted. Details and hashes are in
[`tag251 RUN.md`](benchmarks/qwen35-work-tag251-20261119/RUN.md).

Tag250 and tag253 verify explicit STOP reproof, clean unload, and stable041
hot reload without board reboot. The latest tag253 trial already restored
stable041 at generation4. The next hardware candidate must pass a new strict
idle preflight and retain explicit STOP/BANK END on every fault path.

## GitHub progress

The current branch is `codex/qwen35-4b-p100a` on `origin` in
`armaautomotive/haiku-riscv64-pioneer`. The user asked to push incremental
Qwen3.5-4B progress as milestones are verified. Push source/build evidence
and run records on that branch; do not include local access credentials or
unrelated workspace changes. Commits through `88cb300ed0` are pushed. Tag258
hardware recovery and tag257 cold-latch recovery evidence should be pushed as
the next checkpoint; do not stage local access credentials or unrelated
workspace changes.

No valid decode-rate measurement exists yet. Per-run evidence is under
`benchmarks/`.
