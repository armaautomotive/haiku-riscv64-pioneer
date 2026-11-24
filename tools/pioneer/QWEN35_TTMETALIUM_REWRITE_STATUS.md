> **Actual-model preload qualification staged (2026-11-24 UTC):** restricted tag `2026112300030018` and its client authenticate the pinned 4B GGUF, build the rewrite layout, upload its exact 2740498432 payload bytes using the same strict seven-bank span path, then end the span and bank sessions; no graph, tokenization, inference or CPU fallback is admitted. The Haiku cross-build passed (driver SHA-256 `0dcabe55d701e1288ad97fb3a8ac61500b9d1b09b804582c36303de644973109`, client `dfb25a19e906bf7031e5dbb22ce093edff12b5cd672cd2dcbd2f09d2ce016842`). Actual-client begin and session simulations cover missing support, lost/failed operations, close-only uncertainty and clean bank teardown after a completed upload with invalid final metadata. Build guard negatives and the one-shot recovery wrapper tests pass. Candidate, client, stable backup, wrapper and recovery script are staged and hash-verified at `/boot/home/pioneer-q35-model-preload-20261124-aa`; wrapper SHA-256 `cfae26a7af142f64b544e8584d673519202b925e43209a0f25a77cb724b1632b`, recovery SHA-256 `d7d7980247dc787e0536de1aea343a4ff0c5029b48b07cefd67de517099ec754`. Full rewrite suite session `42675` is running. Before any card operation, await it, freshly verify stable55 idle, no 27B diagnostic or lock, and both model hashes; the wrapper repeats preflight and verifies restoration. Tag0018 has not run. No inference or throughput is measured. Continue only the rewrite, then benchmark and assistant integration.

> **Seven-bank strict preload passed on hardware (2026-11-24 UTC):** after full preflight and the snapshot fix, tag `2026112300030017` loaded as driver60 generation3. Seven fixed 4096-byte transfers, one per bank, each completed with exact device readback and host echo; the snapshot/image was restored and both span and bank sessions ended. The wrapper hot-restored stable55 generation4 without reboot. Independent postflight confirmed `cold0 active_owner0`, expected stable/guard hashes, no lock or diagnostic process, and expected model metadata. Evidence: `tools/pioneer/benchmarks/qwen35-supported-span-20261124-z/RUN.md`. Both full post-run model hashes matched; evidence is in `tools/pioneer/benchmarks/qwen35-supported-span-20261124-z/post-model-hashes.txt`. This is not a full model upload, larger-span qualification, projection, inference, general recovery or throughput result. Continue only the rewrite, then benchmark and assistant integration.

> **Snapshot-fix qualification z running (2026-11-24 UTC):** after y recovery, both full model hashes matched and the complete updated rewrite suite session `21937` passed. The staged tag `2026112300030017` wrapper is running as Haiku process 580 and repeating both full model hashes before any card operation. Poll that same PID, `/boot/home/pioneer-q35-supported-span-20261124-z/{wrapper.exit,state,wrapper.stdout,wrapper.stderr}`, and the serial trace from the offset in `tools/pioneer/benchmarks/qwen35-supported-span-20261124-z/serial-start-offset.txt`; do not restart or reuse the tag. Stable55 generation2 was idle before the wrapper. If it retains a cold owner, save the trace and use the verified pre-staged recovery script; general no-reboot fault recovery remains unqualified. No inference or throughput has been measured. Continue only the rewrite, then benchmark and assistant integration.

> **Snapshot fix staged for qualification z (2026-11-24 UTC):** after y recovery, stable55 generation2 is idle, the exclusive lock is absent and model metadata matches. Tag `2026112300030017`, fixed client, stable backup, one-shot wrapper and y/z-aware recovery script are staged and hash-verified at `/boot/home/pioneer-q35-supported-span-20261124-z`. Wrapper SHA-256 `69616000942340059170ae8573c32cec9639d9c682784ee478a823e4536288da`, recovery script SHA-256 `f4d1b9683bc329e0f33dd38b936e4e60600dea5386e89a77bc3c1c2c2947a25d`. Full post-recovery model hashes session `33267` and full rewrite suite `21937` are still running; poll them and do not touch the card until both pass, then fresh idle/lock and full preflight checks remain required. Tag0017 has not run. No inference or throughput measured. Continue only the rewrite, then benchmark and assistant integration.

> **Stable55 restored after preload qualification y (2026-11-24 UTC):** one authorized OFF/ON cycle and pinned Fedora-to-Haiku handoff completed. The pre-staged y recovery script restored stable55 generation2; independent postflight shows `cold0 active_owner0`, expected stable/guard hashes, no lock or diagnostic process, and expected model metadata. Both full post-recovery model hashes matched; evidence is in `tools/pioneer/benchmarks/qwen35-supported-span-20261124-y/post-model-hashes.txt`. The root preload snapshot fix is checkpoint `21ea6f3a75`, and its full updated suite passed in session `21937`. The 0017 candidate must not touch the card until its tests, both model hashes, fresh idle check and staged recovery verification pass. Evidence: `tools/pioneer/benchmarks/qwen35-supported-span-20261124-y/RUN.md`. No model output or throughput has been measured. Continue only the rewrite, then benchmark and assistant integration.

> **Preload snapshot defect fixed; y recovery running (2026-11-24 UTC):** restricted tag `2026112300030016` failed before device program release: trace shows root resident lease and host buffers prepared, then span preparation entered without reaching source verification; receipt `started=0 completed=0 nonce=0 recovery=1`. The exclusive-takeover build omitted the root panel snapshot required by preload. `MapQ38Panel` now allocates exactly that one root snapshot for preload and no fleet snapshots; `BeginResidentSession` preallocates it before PCI reservation, so its allocation failure returns before touching the card. An extracted test of the actual mapping rejects the old code and passes the fix; a simulated pre-reservation allocation failure makes no device reservation. Fresh tag0017 Haiku cross-build passed (driver SHA-256 `dde75fc1700b9c542ffc6b21aef8d6920d7ce27603ce984fe041782b21732011`, client `696c41afab92c7808fe0a34ff4aa7ffeb3e4028e2149032a1479d2c80c803676`); full updated suite session `21937` is running. The y owner was retained cold and its logs/staged recovery files were verified; one authorized OFF/ON cycle was sent and Fedora booted. Restore stable55 using the pinned no-SD Haiku handoff and staged y recovery script before any further card access. Evidence: `tools/pioneer/benchmarks/qwen35-supported-span-20261124-y/RUN.md`. No inference or throughput measured. Continue only the rewrite, then benchmark and assistant integration.

> **Instrumented preload qualification y running (2026-11-24 UTC):** stable55 generation2 was idle, no diagnostic/lock was present, and both post-recovery model hashes matched. Candidate tag `2026112300030016`, client, stable recovery copy, wrapper and recovery script are staged and hash-verified at `/boot/home/pioneer-q35-supported-span-20261124-y`. The one-shot wrapper is running as Haiku process 568, performing its own full model hashes before any card operation; poll that same process, `wrapper.exit`, `state`, and logs. Serial capture `/private/tmp/pioneer-haiku-recovery.kFZP6j/screenlog.0` begins at the offset in `tools/pioneer/benchmarks/qwen35-supported-span-20261124-y/serial-start-offset.txt`. Do not restart the wrapper or reuse tag0016. If it retains cold, preserve evidence and use the pre-staged hash-verified recovery procedure; no-reboot recovery is not yet qualified. No inference or throughput has been measured. Continue only the rewrite, then benchmark and assistant integration.

> **Preload failure localization candidate (2026-11-24 UTC):** tag `2026112300030016` adds qualification-only serial milestones across resident lease, host buffers, image preparation, device release, completion and readback; it does not change the transfer checks or authorize inference. The client logs the returned upload receipt only when the ioctl replied, and labels an absent reply separately. Driver SHA-256 `2c43af6bbe0e709ae20c03f4e48e3d0fb33a13f39e53662537e14a530b4b6475`, client `696c41afab92c7808fe0a34ff4aa7ffeb3e4028e2149032a1479d2c80c803676`, selected stack envelope 13296/18432 bytes (not a whole-program proof). Haiku cross-build, supported-counter image/controller/service simulations, exclusive-open and guard negatives, and y one-shot/retained-recovery host simulations passed. This candidate has not run on the card; no-reboot recovery for a retained upload fault remains unimplemented. After confirming both post-recovery hashes and fresh idle state, stage and verify the recovery bundle before any one-shot. Continue only the rewrite, then benchmark and assistant integration.

> **Stable driver restored after qualification x (2026-11-24 UTC):** the authorized single OFF/ON cycle and pinned Fedora-to-Haiku handoff returned the machine to Haiku; the verified staged script restored stable55 generation2. Independent postflight shows `cold0 active_owner0`, expected stable/guard hashes, no exclusive lock or diagnostic process, and unchanged model metadata. Both full post-recovery model hashes matched; evidence is in `tools/pioneer/benchmarks/qwen35-supported-span-20261123-x/post-model-hashes.txt`. The first bank preload failure remains unexplained; tag0015 is consumed. No-reboot recovery for this failure is not implemented. Full offline rewrite suite session `12344` passed. Continue only the rewrite, then benchmark and assistant integration.

> **Supported-counter qualification x retained cold (2026-11-24 UTC):** after fresh idle/lock/model-hash preflight, tag `2026112300030015` loaded and the exclusive client failed at bank0 strict span upload. It produced no successful receipt; driver60 generation13 became `cold1 active_owner1` and retained its owner and exclusive lock. This candidate exposes no recovery ioctl; no-reboot recovery is not available for this fault. Evidence: `tools/pioneer/benchmarks/qwen35-supported-span-20261123-x/RUN.md`. The staged recovery script, stable driver copies and candidate hashes were verified, logs were saved, and one authorized front-panel OFF hold was sent. SSH timed out after the hold. Allow the board to settle, check power-state evidence, then send ON once; do not repeat OFF. Follow the pinned Fedora NVMe/kexec path and staged recovery procedure; no SD or hardware changes. Tag0015 is consumed. The full rewrite test suite passed offline in session `12344`; it does not qualify this physical failure. No inference or throughput was measured. Continue only the rewrite, then benchmark and assistant integration.

> **Supported-counter physical qualification w failed cleanly (2026-11-23):** after verified full preflight hashes and an idle stable driver, restricted tag `2026112300030014` loaded on driver60 generation11; its fixed-transfer client exited 1 without diagnostic output or a success receipt. The stable driver55 hot-restored at generation12 with `cold0 active_owner0`, no lock and no diagnostic process; both post-run model hashes matched. The client omitted the exclusive open required by `BeginResidentSession`, a concrete defect and likely cause, but the uninstrumented run does not establish its precise failure stage. No transfer or inference is qualified. Evidence: `tools/pioneer/benchmarks/qwen35-supported-span-20261123-w/RUN.md`. The client now uses and verifies `O_EXCL`, reports failed stages, and a fresh tag `2026112300030015` has been cross-built; offline exclusive-open, close-only and guard tests pass. Candidate driver SHA-256 `e0cd6d579fbb2622ec37dc70567c55602b2e5dabd2ec85afafcc51c920213949`, client `731efea4edb6381e969397856d8303e0915cc00c94524fa874ab940cad105447`. Do not reuse tag0014. No model output or throughput has been measured. Continue only the rewrite, then benchmark and assistant integration.

> **Supported-counter preload qualification prepared (2026-11-23):** added a separately tagged Haiku driver that exposes only strict preload, bank-session and reload operations, with exactly one fixed 4 KiB pattern per bank in bank order. It uses the actual supported-counter image and checks the same readback receipt as model preload. The Haiku cross-build and simulated fixed-client, malformed-reply and build-guard checks pass; a reload routing test rejects inference and recovery operations. Driver SHA-256 `7fee6e7322394343c54acb628b1ab5d3fcff938cffb873d8e9979a76821f36f2`, qualification client `1a6cf9547a3087f74171584702ddf6d49267ce847bcea46a1e3c849464e525dd`; conservative selected-frame stack estimate 13264/18432 bytes, not a whole-program proof. Candidate, client, stable driver and recovery script are staged and hash-verified on Haiku at `/boot/home/pioneer-q35-supported-span-20261123-w`; stable55 was idle at generation10 and both model hashes matched. This is not yet a card test, model upload, inference, recovery proof or throughput measurement. Continue only the rewrite, then benchmark and assistant integration.

> **Verified model preload, offline (2026-11-23):** the dataflow client now uses the strict span transport during initial model loading and requires the device readback/host echo to match every uploaded word before accepting each span. Work inference remains device-side; this CPU byte comparison is load verification, not inference. This may increase model-load time and does not establish decode speed. The actual Q35 span ioctl, service, controller and generated RV32 image passed ASan/UBSan simulation across seven banks and page/hash publication; a corrupted device write was rejected and retained before publication. The normal build entry point now offers `--compile-supported-span-prototype` only with a non-loadable device-dispatch witness. The combined Haiku build passed and rejects loading (driver SHA-256 `7bd6ce064f3fc3d9692b0af9176741fabc75f965af4156409d22924bc4d16216`, client `fedbc13c4ce28be11cde124c0bdc71349777840888a1d47b943113385a9a9de9`); the default control is unchanged. The expanded preload tests passed; the full rewrite suite passed in session `60890`, log `/private/tmp/q35-verified-preload-suite-20261123.log`. The candidate has not run on the card; fleet ownership, no-reboot recovery, model output and throughput remain unverified. Continue only the rewrite, then benchmark and assistant integration.

> **Supported-counter Q35 preload candidate, offline (2026-11-23):** the rewrite's model loader uses a separate span firmware and host controller, which also read unsupported register `0x20c`. Added a separate Q35 firmware image and matching host summary that avoid the register, check supported response/request and write counters, and reserve the unused summary slot as zero. An opt-in `PIONEER_QWEN35_SUPPORTED_SPAN_COMPILE_ONLY=1` build generates and embeds the image only in the non-loadable rewrite witness; both recovery qualification and device-dispatch trial builds are explicitly rejected. The candidate is not selected by the normal runtime and has not been run on the card. The actual generated RV32 image and host controller passed ASan/UBSan simulation across all seven banks, 4 KiB to 1 MiB, strict and Work transfers, exact simulated bytes, corrupt summaries, missing responses and retained faults; both forbidden build-guard tests passed. The old strict and Work span images rebuilt to their original hashes and passed their existing controller/image controls. The Q35 image text SHA-256 is `517c0be9ab3c8c33815ae55e3bbc81562b1a2c88c14c25c1427649f45088ca6b`; the non-loadable integrated driver is `7bd6ce064f3fc3d9692b0af9176741fabc75f965af4156409d22924bc4d16216`, and the default control remains `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`. Full rewrite suite session `41698` passed with the image tests included; log `/private/tmp/q35-integrated-span-suite-20261123.log`. The fleet proof gates and no-reboot recovery remain incomplete. No new hardware inference or throughput was measured. Continue only the rewrite, then benchmark and assistant integration.

> **Blackhole supported read counters, offline (2026-11-23):** the private Q35 scratch and graph-stream bank transfers no longer read NoC register `0x20c`, which Tenstorrent's [Blackhole counter documentation](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/Counters.md) marks unsupported. They use the supported read-response, accepted and read-request counters together with the existing outstanding/idle checks; the historical 27B path is unchanged. The retained-transfer observer also skips `0x20c`; its unused counter slot is zero. A simulated Blackhole mock that rejects that register failed on the previous code and passes with this change, including all three upload stages, graph-stream verification and an accepted request with no read response. The legacy store test and ASan/UBSan rewrite tests passed. The non-loadable Haiku snapshot cross-build passed (driver SHA-256 `3b138e660970be2d83652c005028bb9b576cbebdfe264304df09e0453f055ad2`, unchanged default control `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`). Full suite session `68058` passed; log `/private/tmp/q35-supported-counter-suite-20261123.log`. No card run has verified these counters, and the fleet proof gates still deny launch. Continue only the rewrite, then benchmark and assistant integration.

> **Fleet-check route restoration, offline (2026-11-23):** the diagnostic's held, installation, runtime and stop checks now share a fleet sweep that returns to worker zero after a denied per-core proof. A failed or partially applied selection, failed return, lost session identity or wrong starting route retains the scratch owner and quarantines the bank, host page and lease; an uncertain selection is not followed by a competing TLB write. The real Haiku scope passed focused ASan/UBSan tests with simulated fleet proofs, including failures at worker seven, during return, and after identity loss. The non-loadable Haiku snapshot cross-build passed (driver SHA-256 `b1f92196fc47dabe12b8f5ae40cf3d61da9aa1c636b59e952780aa46a98e5153`, unchanged default control `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`). The prior expanded suite session `67535` passed before this fleet-route change; the final change passed the focused scope test and cross-build. The fleet proof gates still deny real service launch, and no new inference or throughput was measured. Continue only the rewrite, then benchmark and assistant integration.

> **Retained transfer observation, offline (2026-11-23):** a still-attached diagnostic session can now read the retained transfer's command and sample its NoC counters twice under the original caller/generation and retained-owner checks. It checks the held worker, route, mappings, PCI status and exact counter deltas, and rejects unsafe dispatch, changed commands, changed counters, lost authority or a still-pending request. Even matching samples leave the bank, host page and lease quarantined: they are observations, not lifetime exclusion or permission to restore, retry or unload. There is no ioctl, no ability to reattach after detachment, and no completed no-reboot recovery. Focused ASan/UBSan tests passed for capture and all upload stages and for the real scope over simulated IO; the non-loadable Haiku snapshot cross-build passed (driver SHA-256 `07a9dbc5c9fbd29f3bea52829e41d6ae2abe6f16ab3ab6b0ca157d5f35418bd1`, unchanged default control `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`). Updated suite session `67535` passed, log `/private/tmp/q35-late-observation-suite-20261123.log`; it started before the final scope and test refinements, which passed separately. No card access or throughput measurement occurred. Continue only the rewrite, then benchmark and assistant integration.

> **Retained NoC command evidence, offline (2026-11-23):** the private scratch failure record now includes the exact ten command words, transfer direction and stage recorded immediately before an attempted NoC doorbell. The trace is cleared before each command, so a rejected command cannot inherit an earlier command's evidence. Simulated timeout tests check capture reads and all three upload stages; a scope denial has no issued trace. The default resident-store regression and scratch ASan/UBSan tests pass. The non-loadable snapshot Haiku cross-build passed (driver SHA-256 `feca11f04eab2bad4b4dd9992693d6a32820b55331f09b96adadb04d834ea078`; unchanged default control `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`). The full updated suite passed in session `45239`, log `/private/tmp/q35-command-trace-suite-20261123.log`; the previous suite reached its final PASS marker, though its exit status was not preserved. The trace does not prove completion or authorize a retry, release, or no-reboot recovery; the real fleet proof gates still deny execution. No card access or throughput measurement occurred. Continue only the rewrite, then benchmark and assistant integration.

> **Retained scratch-transfer failure evidence, offline (2026-11-23):** the scratch owner now keeps the failed page, operation and transaction phase, captured/attempted page counts, the pre-quarantine lease flags, and the underlying resident transfer result including NoC counters. This distinguishes a denied transfer, a capture read timeout, a host-to-L1 timeout, a potentially issued GDDR write and a restore mismatch. ASan/UBSan tests inject these cases and confirm the retained session owner still exposes the receipt. The information is internal only: no diagnostic ioctl, late-completion proof, retry authority or no-reboot fault recovery is implemented. The non-loadable Haiku snapshot witness cross-build passed with driver SHA-256 `7b682e63f0e3b6a813eacdddf88082b481c45c9a585637d5588e9c2aa38673af`, unchanged default control `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`; stack estimates do not cover a real ioctl caller. The expanded suite reached its final PASS marker, log `/private/tmp/q35-transfer-receipt-suite-20261123.log`; its process exit status was not preserved. No physical card use or decode speed measurement occurred. Continue only the rewrite, then benchmark and assistant integration.

> **Captured but unmodified scratch cancellation, offline (2026-11-23):** a diagnostic may now abandon after snapshot capture and before staging if the program is unstarted, the private claim still matches and the Haiku scope independently approves the held/quiet session. The sticky claim and holder survive until bank-context destruction; no bank writes are needed for this cancellation. The actual session abort wrapper and Haiku scope passed ASan/UBSan tests with simulated fleet proofs, including checking all 680 pages remain unchanged. The separate staged-then-restored unstarted abort and published-run refusal also passed focused tests. The snapshot witness cross-build passed with driver SHA-256 `158b60a4540cb8b347fa15b392375b6bdaba3ccc76cf1f2d1051e5d3f566f2d0`, unchanged default control `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`. Full suite session `22896` passed; log `/private/tmp/q35-real-scope-final-suite-20261123.log`. Physical fleet proofs still deny access; post-start no-reboot recovery, hardware output and decode speed remain unverified. Continue only the rewrite, then benchmark and assistant integration.

> **Haiku diagnostic scope and pre-run cancellation, offline (2026-11-23):** the non-loadable witness now uses a concrete Haiku scope that checks the borrowed session identity, exclusive bank claim, firmware status, and existing per-worker held/quiet and runtime proof gates. Those fleet proofs still fail closed, so this is not permission to access or launch the card. Scratch operations run through the actual session wrapper with checked, scoped internal admission. After a staged but unstarted run, the owner may retire only after all scratch pages are restored and verified; a rejected run preflight leaves that clean-abort option, while started or uncertain work remains retained. ASan/UBSan tests execute the real scope and extracted session wrappers with simulated proofs, including denied fleet proof before reservation, identity loss/quarantine, complete scratch restoration and bound-but-unstarted cancellation. The snapshot Haiku cross-build passed with driver SHA-256 `ebb441132c29c6e224515ae31c9b7e58ad111df4b5e522fe8fd772f065b90a88`; the non-snapshot shared-role build passed with `2e610d2f573ac005af06c9613ecb26b9f1b6e3e20ec203935a87274eca4de04b`; default control remains `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`. Stack estimates do not prove a real ioctl caller. Expanded suite session `11325` passed after it began before the last test-script edits; log `/private/tmp/q35-haiku-scope-stable-suite-20261123.log`. A fresh final suite is running in session `22896`. Hardware ownership, fleet release, post-start fault recovery and output remain unverified. No card use or throughput measurement occurred. Continue only the rewrite, then benchmark and assistant integration.

> **Diagnostic ownership and teardown, non-loadable witness (2026-11-23):** the Haiku store context now owns the diagnostic scratch, fixed program and saved-page buffer in a holder declared before the bank object, so the sticky bank claim is destroyed before its owner. An active holder blocks ordinary session use and session-end teardown; checked diagnostic calls bracket a temporary internal admission. Clean retirement requires an audited projection, stopped service, verified scratch restoration, a matching claim and scope approval. Tests execute the actual extracted Unsafe, session-end and reset guards and cover unclaimed abandonment, retained transfer failure and full simulated scratch/service retirement. Generic reset is intentionally rejected while this holder is active, and its finish path refuses to free an active holder: no-reboot recovery for this diagnostic is still missing. The scope is compile-only and does not establish hardware ownership; the witness rejects loading. Haiku cross-build SHA-256 `59fe6fb60d8d90e2d361369b31814c21366f3a2dc7a2c711d3c664fdbc72d83e`, unchanged default control `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`; the estimates do not cover a real ioctl caller. Expanded suite session `30516` passed; log `/private/tmp/q35-owned-session-suite-20261123.log`. The final reset guard was added after that suite started and passed its focused test separately. No card access, inference or throughput measurement occurred. Continue only the rewrite, then benchmark and assistant integration.

> **Fixed-projection resident installer, non-loadable witness (2026-11-23):** factored the existing resident-service allocation/install body behind its original model-authority and program-workspace checks, and added a separate compile-only diagnostic entry that requires the staged scratch owner, a scope admission, no model/graph authority and the real fleet-route preflight before using that same installer. The witness compiles these calls with `PioneerSessionIO`; its scope is a stub and the driver rejects loading. The actual locked scope, retained owner, hardware release/stop and recovery are not implemented, and this does not run on the card. The Haiku cross-build passed with driver SHA-256 `68752f8ea7ce17eaee050280663810d714e3a8e3be9c28351a394d54af81a196` and unchanged default control `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`; stack estimates do not cover a real diagnostic ioctl caller. The route-compatible full suite passed; it started before the installer refactor, which was separately cross-built. Log: `/private/tmp/q35-route-compatible-suite-20261123.log`. No physical card use, inference or speed measurement occurred. Continue only the rewrite, then benchmark and assistant integration.

> **Diagnostic scratch and fleet-route audit compatibility (2026-11-23):** the real `ProveFleetRoutes` resolves page zero of every bank, so the previous bank-zero scratch claim at offset zero would have made installation fail. The fixed diagnostic now starts at bank-zero offset 4096 (`0x04001000`), leaving page zero available. The driver and simulated scratch test share the exact bank-route audit helper: the test confirms an offset-zero claim fails the audit and the new claim passes it, then runs the fixed 120-worker projection and full scratch restoration in simulation. The Haiku non-loadable snapshot witness cross-build passed with driver SHA-256 `b86f7641709b60cc9726ac14fe118d52da07f2f0f1ce292866f1d3cb81fdcf08` and unchanged default control `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`. The route-compatible full suite passed; log `/private/tmp/q35-route-compatible-suite-20261123.log`. This fixes a real integration conflict but does not authorize the diagnostic, prove the rest of fleet preflight, execute hardware or measure throughput. Continue only the rewrite, then benchmark and assistant integration.

> **Fixed projection service and claimed scratch joined, offline (2026-11-23):** a one-shot diagnostic owner now binds only its internally built Q4_K program to the staged scratch lease, checks the live bank claim before launch and requires a scope-provided all-worker completion audit. A separate bank-owner identity check supports repeated authorization without scanning all pages on every command; sealed-stream behavior remains covered by the existing test. The diagnostic retains failed or partially published runs. ASan/UBSan tests cover successful 120-worker simulated reader/compute/writer execution, exact numerical results, wrong leases, copied programs, failed receipts/audits, loss of scope after publication, and a combined scratch capture/stage/run/close/output/680-page restore. The test matrix and NoC are simulated. A non-loadable Haiku witness instantiates the adapter with `PioneerSessionIO` and the actual projection receipt audit (driver SHA-256 `3f60ca6d70eff6c14af9664de65385fa78ae7600f668c4ccc6188fe06318aeb2`; default control `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`). The expanded dataflow suite passed, including both new diagnostic service tests and the scratch test; log `/private/tmp/q35-qualification-service-suite-20261123.log`. No physical control or real authority scope is connected: locked session, scope/owner lifetime, firmware/reset continuity, full-fleet start/stop, no-reboot recovery and hardware output remain unverified. No card was accessed or decode speed measured. Continue only the rewrite, then benchmark and assistant integration.

> **Private fixed-projection scratch, offline (2026-11-23):** the qualification now has a heap-owned 680-page transaction using the existing sticky Q35 bank claim and the production `PioneerResidentStorePage` transfers. It captures all original pages before staging, denies transfer while the simulated fleet runs, validates every output, restores the full range and re-reads every page after the final write. Simulated NoC tests cover claimed-page exclusion, initialized-page rejection, output mismatch, an uncertain transfer and corruption of an earlier page during a later restore. The claim and snapshots remain retained on failures; actual no-reboot recovery is not established. A non-loadable Haiku witness instantiates the transaction with real `PioneerSessionIO`; cross-build SHA-256 `6591749287bdaed3f31932b6e6da3421d424926ec95b679fdd6b3b0a93726591`, normal control `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`. The normal model-control install rejects this active bank claim; a separate guarded diagnostic control, real session scope, scratch-owner lifetime/retention, service authorization, fleet-stop and recovery proofs are still missing. The expanded suite is running in local session `45946`, log `/private/tmp/q35-scratch-suite-20261123.log`; poll it rather than restart. No physical card access, model inference or speed measurement occurred. Continue only the rewrite, then benchmark and assistant integration.

> **Next fixed-projection hardware integration:** `PioneerResidentStorePage` uses worker-zero L1 and NoC resources; capture and stage every scratch page before resident service installation, then read output and restore/verify all saved pages only after the service has stopped and worker-zero ownership is re-established. The current held-fleet role-stage qualification requires `uploadedPages == 0`, so ordinary public bank uploads cannot be used to bypass that gate. Implement a private, exclusively owned scratch transaction with retained snapshots and failure handling before connecting this fixture to hardware; do not release or reuse an uncertain range. This is an implementation constraint, not hardware execution or an ownership proof.

> **Resident projection service integration, offline (2026-11-23):** the fixed 120-worker Q4_K fixture now runs through the real service owner and scheduler with simulated device I/O and concurrent reader, compute and writer queue functions. ASan/UBSan verifies all output rows, a failed worker receipt without result commit, simulated recovery, a subsequent correct projection and clean service close. The driver completion and queue audit has been separated from model authorization for later reuse by a fixed diagnostic; model authorization remains in place, and the diagnostic is not connected to live hardware. The existing full dataflow suite passed before this refactor, the new integration test passed separately, and the non-loadable snapshot driver cross-build passed (SHA-256 `91e5013b59a2e3aa6650019b4bd2bc6ce87e9f9f65640c13866533a08197293c`; selected stack bounds are not a whole-program proof). No physical card use or throughput measurement occurred. Next: bind exclusive scratch and the fixed diagnostic to the real driver under the outstanding ownership/recovery gates. Continue only the rewrite, then benchmark and integrate the assistant.

> **All-worker Q4_K projection fixture, offline (2026-11-23):** a fixed synthetic projection now builds through the resident graph scheduler with one 16-row group per each of 120 workers. Its expected output is distinct for every row. An independently decoded GGUF Q4_K reference and concurrent simulated reader, compute and writer roles passed under ASan/UBSan. This is a host simulation only: it has not run on the card, measured throughput or provided a CPU inference fallback. The next hardware qualification requires an exclusive scratch lease and saved-state restoration plus real resident-service ownership and recovery proofs; the fixture alone does not grant card access. Continue only the rewrite, then benchmark and integrate the assistant. Repeat idle-driver and 27B preflight before card use.

> **Resident role fixed-fault passed on Pioneer (2026-11-23):** restricted tag `2026112300030013` captured all 120 held workers, deliberately submitted an invalid fixed command to worker zero’s actual B/NC resident roles with TRISCs held, observed both failure receipts and expected queue failures, audited NoC quiet, requested fault stop and observed both stop and local-restore receipts and parked PCs. It restored the invocation, queue controls, mailbox, handoff, reset-preservation word and fleet images, closed mappings and cleanly ended the session. The wrapper hot-reloaded stable55 without reboot; independent postflight found generation 10 idle with no diagnostic process or lock and expected driver/guard hashes and model metadata. See [stage-v evidence](benchmarks/qwen35-role-fault-20261123-v/RUN.md). Both complete model SHA-256 hashes matched before and after the run; see [post-run hashes](benchmarks/qwen35-role-fault-20261123-v/post-model-hashes.txt). This verifies one deliberately rejected command on one worker only: no TRISC release, projection, full graph, general fault recovery, firmware exclusion or decode speed is established. Continue only the rewrite, then benchmark and assistant integration; perform a fresh idle/27B preflight immediately before any further card use.

> **Resident role fault one-shot running (2026-11-23):** after final process/lock checks, stable55 generation 8 idle, exact staged/recovery/driver hashes and both full model hashes matched, the single version 5/tag `2026112300030013` wrapper was started in local session `57329`. Output is `tools/pioneer/benchmarks/qwen35-role-fault-20261123-v/wrapper-output.txt` and `wrapper-error.txt`; preflight and model hash evidence are in the same directory. The wrapper repeats both full model hashes before touching the card, then runs its fixed one-worker fault and guarded stable-driver restoration. Poll the existing session; do not start it again or repeat the one-shot marker. If it retains cold or unknown, do not write/copy files to the Pioneer until the authorized power-cycle recovery is revalidated against the pre-staged script hash `8110467433fee194b9eb670362da3aedc09e9a961b741f55df7bad70b3a58453`. This is a rewrite qualification, not inference or throughput evidence. Continue only the rewrite, then benchmark and assistant integration.

> **Resident role fault candidate staged (2026-11-23):** separately guarded version 5/tag `2026112300030013` is built reproducibly and staged at `/boot/home/pioneer-q35-role-fault-20261123-v`. Candidate SHA-256 `a5c45df13f5a4ba8c352167b99f308a4b81f03930a930e325415a4cc3af41e22`, client `b74d14e2dc9b4ee0a8ee2b082bac5c7632782f76557c27b845cd87d86d7f32c2`, one-shot wrapper `ac1ce6c20ae6b743069bcfef278f985700d9cbe02c0e5785242d00cedd5254b6`, pre-staged recovery `8110467433fee194b9eb670362da3aedc09e9a961b741f55df7bad70b3a58453`; remote hashes matched. The selected kernel frame sum plus 4096-byte allowance is `18208/18432`, not a whole-program proof. Guard negatives and simulated one-shot/recovery tests passed. A read-only check found stable55 generation 8 idle, no diagnostic and no exclusive lock. The independent full-model hash check is in local session `21079`, and the final offline suite is in session `25981`; poll these existing sessions and require both exact model hashes and suite success. Immediately before one run, repeat driver/process/lock/staged-file preflight; the wrapper independently repeats both full model hashes and idle checks. Never rerun the one-shot marker. The physical fault test has not yet run; no inference or throughput is claimed. Continue only the rewrite, then benchmark and assistant integration.

> **Resident role fault exercise, offline only (2026-11-23):** a separately guarded controller now saves the fixed invocation buffer-address word and all 24 queue-control words, forces address zero, releases the actual resident roles, requires both failed receipts and exact role-owned queue failures, samples quiet NoC FIFOs/doorbells/outstanding slots, requests a fault stop, then requires local restore and reinstates and rechecks the saved words. Simulated controller tests cover a successful fault/cleanup, a missing receipt, a nonquiet NoC, and failed preparation or restore stores. An RV32 interpreter executed the actual linked reader and writer ticks with the rejected invocation, confirmed failure and wait/stop receipts and only role-owned L1 writes, and rejected any NoC/MMIO access. The full offline dataflow suite and the final extended controller and RV32 tests passed. This is simulated, not hardware: the experimental fault code explicitly refuses kernel builds, and no no-reboot fault recovery or inference is established. The normal idle-service qualification cross-build still passes (driver SHA-256 `acb5caf14225180a700e7a4f857f99f808f7de2507253cab09c58ee4b97364b7`) but differs from the physically tested stage-u binary; the original hardware evidence applies only to its pinned binary. The stable default control remains `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`. A separately tagged physical candidate, full build guards, safe staged recovery and fresh idle/27B/model-hash preflight remain necessary. Continue only the resident rewrite, then benchmark and integrate the assistant. No hardware access or throughput measurement occurred.

> **Resident partial-start cancellation, offline (2026-11-23):** the copy-aware recovery ledger now records a captured worker as cancelled only after stackless cancel/park and held-reset evidence, independent retained-source and intact-RAM proof, and either no stack-release attempt or proof that its attempted write was drained and absent. A possibly outstanding release remains retained. Retirement distinguishes cancellation from an actual stop-to-restore, and the resident binding carries each result through fleet cleanup. Focused ASan/UBSan simulations cover mixed restored/cancelled fleets, unarmed and armed cancellations, posted releases, wrong pointers, changed RAM, missing scope proof and incorrect retirement type. The guarded Haiku snapshot build passed and rejects loading (driver SHA-256 `6e37ae2764102dfb2ea25e2cb048c8a7ab9d57bb3ebc71f0af4e3ad90852b489`, normal control `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`); selected kernel stack estimate `17888/18432` is not a whole-program proof. The real scope still denies the necessary firmware, writer, source and reset-continuity guarantees, and incomplete shim startup and physical fault recovery remain unqualified; auto-release stays disabled. The full offline dataflow suite passed; the final ledger test extension also passed independently. No hardware access or throughput measurement occurred. Continue only the rewrite, then benchmark and integrate the assistant; repeat idle-driver and 27B preflight before card use.

> **Resident auto-restore ledger, offline (2026-11-23):** the snapshot binding now selects a separate one-shot ledger before a role stack release, records a restore only after the expected stop/restore park and held reset, and routes cleanup through a parent-handoff-aware retirement path. The previous separately released restore-shim path cannot be used for this mode. Before any release ledger starts, the binding can restore staged images through the held-state path. Focused ASan/UBSan simulations cover stale admission, duplicate arming, wrong retained pointers, missing restore, partial fleets, failed image-retirement writes, proof loss and retry; the Haiku snapshot witness cross-build passed and rejects loading (driver SHA-256 `4aeb975cdd7c75d26c837c381aa48d205897c42af04650d9e1b71fb1a456e89c`, normal control `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`). This path still explicitly denies service release: the real scope has no lifetime proof for firmware, source, handoff and writer exclusion; cancelled or uncertain starts and physical fault recovery remain unresolved. The full offline dataflow suite passed; the final early-cleanup change also passed the focused recovery-owner and binding tests. No hardware access or throughput measurement occurred. Continue only the rewrite, then benchmark and integrate the assistant; repeat idle-driver and 27B preflight before card use.

> **Snapshot auto-restore binding, offline work in progress (2026-11-23):** the snapshot rewrite can explicitly select the physically qualified resident service plus stop-to-restore images; its running-PC range excludes the restore tail, and the stop observer checks stable role and restore receipts and exact restore park PCs. The normal image remains the default. The new path cannot release roles: its retained-RAM lifecycle ledger and scope proof of source, writer and firmware exclusion are not yet connected. Focused simulated checks pass, and the snapshot Haiku build rejects loading (driver SHA-256 `b5b05a69fdb18356df89d62b1ef6a414a4846f809a87889883412767240abaae`). No card access, fault-recovery qualification or throughput measurement occurred. Continue only the resident rewrite; connect and test the complete lifecycle before any hardware use, then benchmark and integrate the assistant. Repeat the idle-driver and 27B preflight immediately before card access.

> **Resident reader/writer idle-stop passed on the Pioneer (2026-11-23):** restricted tag12 captured all 120 held workers, staged worker zero's actual resident role code, and with TRISCs held started both B/NC service loops, observed their startup receipts and running PCs, requested idle stop, then observed stop receipts and exact stackless local-RAM restore receipts and parked PCs. The original mailbox, handoff, reset-preservation setting and fleet images were restored; mappings closed and the bank session ended cleanly. The wrapper hot-reloaded stable55 without reboot. Independent postflight found generation 8 idle, no diagnostic or exclusive lock, expected stable-driver hash and model metadata; see [stage-u evidence](benchmarks/qwen35-role-service-20261123-u/RUN.md). Both full model hashes matched before and after the run; see [postflight hashes](benchmarks/qwen35-role-service-20261123-u/post-model-hashes.txt). This verifies a one-worker idle lifecycle only; projection work, the two-read pipeline, TRISC release, firmware exclusion, fault recovery and inference remain unqualified. Continue only the rewrite, then benchmark and assistant integration; no new decode speed was measured.

> **Resident role idle-stop/restore qualification prepared (2026-11-23):** separately guarded tag `2026112300030012` stages the exact shared reader/writer service code on worker zero, captures both private-RAM copies, releases B/NC with TRISCs held, waits for their actual startup receipts, requests an idle mailbox stop, and has each stopped role jump to the exact previously qualified local-RAM restore code. It verifies restore receipts and parked PCs, restores the original mailbox, handoff, reset-preservation word and fleet images, then closes the bank and hot-restores stable55. The actual controller passed simulated startup, stop, cleanup and retained-stop-failure tests; the linked shims and exact restore instructions passed RV32 interpretation with simulated role ticks, and normal role-code bytes match the current resident images. The full dataflow suite, six build-guard negative tests, wrapper/recovery tests and reproducible Haiku build passed. Candidate SHA-256 `eb035341f505dc5c406e0e45946fd87b379b857ba58b5ba6fb3b2b1e503fb08a`; client `30b493d051eda2cb72a3a80ad937c2d4ad02109c8e1e8a2dbedfdc844e617944`; selected kernel frame sum plus 4096-byte allowance `18016/18432`, not a whole-program proof. These, the one-shot wrapper (SHA-256 `ad4c9d8da8ccb5e69e7219a1cb489ebbc955533bd55ad7743c385b2e63b6ea51`) and pre-staged recovery (SHA-256 `eeb93bd48a6cabfe8f546a88009b38f8c3a417a4985f0342fc090fb61b3e9429`) are hash-verified at `/boot/home/pioneer-q35-role-service-20261123-u`. Fresh preflight showed stable55 generation 6 idle, no 27B diagnostic or lock; the independent model hashes are running in session `96165` with output `/private/tmp/q35-role-service-preflight-models.txt`. Before one card run, require both exact hashes and fresh driver/process/lock/staged-file preflight; the wrapper repeats both full model hashes. Never rerun its one-shot marker. This is not graph inference, a firmware-exclusion proof, no-reboot fault recovery or throughput evidence. Continue only the rewrite, then benchmark and assistant integration.

> **Two in-flight weight reads, offline rewrite (2026-11-23):** the shared reader now keeps up to two assigned weight pages in flight using NoC transaction IDs 0 and 1, while preserving ordered page publication and overlapping activation packing and matrix work. A one-shot pipeline verifies initiator acceptance before publication, waits for FIFO drainage and the corresponding outstanding-ID count, checks exact accumulated counters and a quiet NoC at completion, and retains faults without reusing uncertain pages. The protocol requires exclusive NIU/ID ownership and stable routing for the whole projection; the real hardware scope still does not establish that guarantee. Simulations cover two in-flight requests, reversed completion, 512 requests with repeated ID reuse, FIFO and counter faults, dropped doorbells, uncertain issues, counter wrap, ordered page contents and reader/compute/writer coordination. The final non-loadable Haiku exclusive-takeover build passed (driver SHA-256 `464f120a96dafc5c60dcb180117e3ea26121812b93923a3d5acda8cb114bd918`, client SHA-256 `67b0e73d0d2bb4a86ff8537179ef598e87e741d8038e70e4a60fe4fc2d547c02`); reader code is 19,936/20,480 bytes and the selected frame sum is 5,952/8,192 bytes, not a whole-program proof. The full dataflow suite passed in session `98251`; the additional retained-in-flight fault test also passed under ASan/UBSan. This path has not run on hardware, no new throughput has been measured, and firmware exclusion and fault recovery remain unqualified. Continue only the rewrite, then benchmark and assistant integration.

> **Retry pre-reset preparation, offline (2026-11-23 local):** a failure while disabling PCI bus mastering or obtaining two matching PCIe payload snapshots no longer automatically ends the non-loadable recovery attempt before an ASIC reset is issued. The recovery owner retains the same guard ticket and unopened post-reset mapping object, repeats the full identity/layout and idempotent master-disable checks, and validates the ticket again before issuing the reset. The client recognizes a specific preparation-wait detail and retries with its existing bound. Simulations cover dropped card/root command writes, repeated preparation failures, changing payload reads, stale tickets and exactly one eventual reset; an unrecognized error is not retried by the client. Failure after an uncertain reset trigger remains a separate path. The reload/recovery suite, complete offline target suite and non-loadable Haiku cross-build passed; driver SHA-256 `17dc586dee137d876def13c2d8c9b09b5a95fc517dbfb31a69832fdd3dfb6604`, client SHA-256 `4a79a42f954a1686621c853ab836dd8f813fbd40a335458efe1e855dadd47d4f`, default control unchanged at `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`. Physical reset, firmware reinitialization and no-reboot recovery remain unqualified; the driver is non-loadable. No card was accessed and no throughput measured. Continue only the rewrite until ready, then benchmark and integrate the assistant; repeat the idle/27B preflight immediately before card use.

> **Observed dedicated clean stop, offline (2026-11-23 local):** the non-loadable rewrite now preserves a clean-stop marker only when closing an idle ready service; failed startup or a retained fault does not acquire it, and retries retain it. The dedicated role binding can use that scoped observation to run its existing ordered clean-stop path. Before each role stop it verifies a stable idle compute mailbox and empty queues; it then checks role receipts and park PCs, requests compute stop, checks stopped receipts and PCs, holds workers and audits the held fleet. The actual binding and per-tile simulated MMIO exercised the complete path for all 120 workers and confirmed a retry does not duplicate stop or hold writes. Targeted tests also reject occupied queues and non-ready workers. This is not a physical NoC/firmware exclusion proof, physical shutdown or no-reboot recovery: partial or uncertain releases and faulted graphs remain separate, and the takeover remains retained until reset/reinitialization. The full offline target suite and non-loadable Haiku cross-build passed; driver SHA-256 `05adc3b7d65f37ed186d8b8641784087d8460ac430bf390005a4954f35229b16`, client SHA-256 `1a11bbbcc578ef29a2211809292428508a0c26a7e1545a308020cd0507423019`, default control unchanged at `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`. No card was accessed and no throughput measured. Continue only the rewrite until ready, then benchmark and integrate the assistant; repeat the idle/27B preflight immediately before card use.

> **Observed dedicated runtime, offline (2026-11-23 local):** the non-loadable dedicated binding can now perform its bounded runtime checks during installation, ready and graph-running phases under its real session/firmware admission; the older snapshot-path proof remains denied. The real binding and per-tile simulated BAR storage now carry 120 workers through staged-image verification, release and startup-readiness checks using simulated worker receipts and PCs. A simulated warm state with advanced independent worker/role sequences also passes; missing role startup, an occupied queue, a parked role, mismatched firmware and an invalid service phase are rejected. These are observations only: they do not establish physical image or firmware write exclusion, graph continuity, NoC quiescence or recoverability. Fault auditing, shutdown, reset/reinitialization and physical launch remain unqualified, and the driver is non-loadable. The full offline target suite and non-loadable Haiku cross-build passed; driver SHA-256 `21e5ad71b2867d11e8f8568057d1624b20141a57446a891863b0a1a413eb4dc8`, client SHA-256 `1a11bbbcc578ef29a2211809292428508a0c26a7e1545a308020cd0507423019`, default control unchanged at `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`. No card access or throughput measurement occurred; simulated readiness is not model inference. Continue only the rewrite until ready, then benchmark and integrate the assistant; repeat the idle/27B preflight immediately before card use.

> **Observed dedicated release, offline (2026-11-23 local):** the non-loadable dedicated prototype now reaches release through its own scoped admission after stage observations, a complete image re-verification and the one-shot release ledger; the earlier test-only fabricated authorization has been removed. The actual binding, per-tile BAR simulation and adapter exercise all 120 releases, reject an unexpected firmware identity and out-of-order starts, and retain an uncertain partial release without retry. A close attempt revokes release admission and authorization before stop, preventing the remaining fleet from being started after cleanup begins. This admission is based on point-in-time observations: it is not proof of firmware write exclusion, physical release safety or recoverability. Stop, runtime, release-write drainage and ASIC reset/reinitialization are not physically qualified, and the driver remains non-loadable. The complete offline target suite and non-loadable Haiku cross-build passed; driver SHA-256 `f4bfd3abc796bde71975fefa85c295380806ae638a09e3ea2efe00ba59bbabb7`, client SHA-256 `1a11bbbcc578ef29a2211809292428508a0c26a7e1545a308020cd0507423019`, default control unchanged at `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`. The Pioneer answered SSH in Haiku and its process list showed no Qwen/Pioneer diagnostic; stable-driver idle was not checked in this turn. No card was accessed or throughput measured. Continue only the rewrite until ready, then benchmark and integrate the assistant; repeat the full idle/27B preflight immediately before card use.

> **Dedicated release binding and held-stop audit, offline (2026-11-23 local):** the actual Haiku shared-role binding and PCI adapter now exercise all 120 per-tile releases in simulation under an explicitly injected test-only authorization; a second run drops one release and checks the retained, non-retryable partial ledger. The test-only authorization is rejected when compiling for RISC-V, and the real release proof still returns false. This exercise exposed a shutdown bug: the final held-worker audit previously reused staging admission, which is revoked on first release. Dedicated stop now uses a separate read-only check of owner, firmware, route, clock, reset and local NoC status, including for held cores that were never released. The real binding simulation confirms that check works after capture ends and rejects running tiles and firmware mismatch; samples do not prove that another agent cannot write or establish a completed physical shutdown. Actual stop authorization, release-write drainage, runtime ownership and device reset/reinitialization remain unqualified. The complete offline target suite and non-loadable Haiku cross-build passed; driver SHA-256 `7a213170b36b2176f01d51bec77f577138e3f667a189a22100e52092b33e02cb`, client SHA-256 `1a11bbbcc578ef29a2211809292428508a0c26a7e1545a308020cd0507423019`, default control unchanged at `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`. No card was accessed and no throughput was measured. Continue only the rewrite until ready, then benchmark and integrate the assistant; repeat the idle/27B preflight immediately before card use.

> **Per-tile staging and release clock checks, offline (2026-11-23 local):** the actual Haiku binding and mapper now have a simulation with distinct backing memory for all 120 workers. It verifies both boot entries and the generated reader/writer shims and code on every tile. A one-write BAR misroute with normal-looking route status is detected before any boot entry is written; the partial takeover remains reset-required and release is denied. The dedicated release adapter also rechecks the selected clock before reset access and after an attempted release; simulated clock loss before the write leaves it unattempted, while clock loss after the write retains the uncertain attempt without reading a disabled tile. Clock samples cannot exclude concurrent external changes. The full offline target suite and non-loadable Haiku cross-build passed; driver SHA-256 `660a4736f66db4d2ab0e5c021d8fde9c949143d32e26406fd37685cee4ce2623`, client SHA-256 `1a11bbbcc578ef29a2211809292428508a0c26a7e1545a308020cd0507423019`, default control unchanged at `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`. Physical ownership, release, startup and no-reboot recovery remain unqualified; the driver is non-loadable. No card was accessed and no throughput was measured. Continue only the rewrite until ready, then benchmark and integrate the assistant; repeat the idle/27B preflight immediately before card use.

> **Dedicated observed staging, offline (2026-11-22 local):** the compile-only dedicated binding now reaches role-image staging after exclusive-owner and firmware-identity checks and sampled L2CPU reset, held fleet/NoC and host-DMA checks, with per-tile route, clock and held-reset checks. The actual binding and adapters traversed 120 workers in simulation using shared mock backing storage; separate fleet tests exercise per-core storage. Disabled-clock, firmware, range and route rejection and a simulated reset change after partial staging were exercised; partial state remains reset-required and retry is denied. These point-in-time observations do not prove firmware or other NoC agents cannot write. Release is still denied, the driver is non-loadable, and physical startup and no-reboot recovery remain unqualified. The offline target suite and non-loadable Haiku build passed; driver SHA-256 `6e291be15375bbcd1a870e9a07670d06fa250fa98354766c51d61dd29170e770`, client SHA-256 `1a11bbbcc578ef29a2211809292428508a0c26a7e1545a308020cd0507423019`, default control unchanged at `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`. No card access or throughput measurement occurred. Continue only the rewrite until ready, then benchmark and integrate the assistant; repeat the idle/27B preflight immediately before card use.

> **Adjacent projection activation reuse, offline (2026-11-22 local):** the shared reader keeps packed activations across adjacent graph steps when their epoch, input rows, base and packing format match, and the preceding projection's complete output bank ranges do not overlap the input. Q4_K/Q5_K/Q6_K share the Q8_K packing; Q8_0 remains separate. A run resets the cache, non-adjacent steps cannot hit, and inactive or failed readers do not publish a reusable result. Concurrent simulated role tests compare packed values across all formats; range overlap, changed input, skipped steps, epochs and quantization changes are covered. A synthetic model-layout recording across 120 workers and 548 steps had `7280/23320` active projections eligible to skip a reload and repack; this is a simulation count, not a measured speedup. The complete offline target suite and non-loadable Haiku build passed, and exported reader and graph images were checked for the metadata address. Driver SHA-256 `8654feaa3366c91c099bb780c4b60d852ab6c2111fdcc23c18fa80f366c499c1`, client SHA-256 `1a11bbbcc578ef29a2211809292428508a0c26a7e1545a308020cd0507423019`, default control unchanged at `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`. Physical role ownership, L1 visibility, launch and no-reboot recovery remain unqualified; no card was accessed and no throughput was measured. Continue only the rewrite until ready, then benchmark and integrate the assistant; repeat the idle/27B preflight immediately before card use.

> **Shared-role compact barrier, offline (2026-11-22 local):** successful tree-barrier messages now send one repeated epoch/step marker and reconstruct the known success payload at the receiver. Failures use a distinct marker and retain ordered payload-plus-marker writes. Simulations exercise 120 workers for 548 steps, 65,536 old/new marker word mixtures, graph execution and failure propagation, partial sends and malformed data. Successful simulated decode-barrier traffic changes from `260848` writes to `130424`, a message count rather than measured throughput. The target suite and non-loadable Haiku cross-build passed; driver SHA-256 `22598f10341fd214d58ed71273eac3d712323fb6068d6893cd1c9f389d6ef0ae`, client SHA-256 `1a11bbbcc578ef29a2211809292428508a0c26a7e1545a308020cd0507423019`. The shared graph worker stack envelope is `6256/8192`, not a whole-program proof. Physical launch, L1/NoC visibility and no-reboot recovery remain unqualified; no card was accessed and no throughput measured. Continue only the rewrite until ready, then benchmark and integrate the assistant.

> **Post-reset mapping retry, offline (2026-11-22 local):** a failed post-reset mapping allocation returning `B_NO_MEMORY` can now be cleaned up and retried under the same reset ticket, without another reset or PCI restore. The path requires successful unmapping and a stable PCI route; other errors and uncertain cleanup are not retried. The client recognizes the explicit pending detail. Simulations cover all seven allocation points, repeated waits, cleanup/route failures, invalid tickets and a subsequent firmware wait. The recovery suite and non-loadable Haiku cross-build passed; driver SHA-256 `69354c372acf3d4bf2cfdccc01f975982fb90668974522dc914a1dc64b797a04`, client SHA-256 `1a11bbbcc578ef29a2211809292428508a0c26a7e1545a308020cd0507423019`, default control unchanged at `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`. Real reset, firmware reinitialization, launch and full no-reboot recovery remain unqualified; no card was accessed and no throughput measured. Continue only the rewrite until ready, then benchmark and integrate the assistant.

> **Shared writer output coalescing, offline (2026-11-22 local):** the resident writer combines two already-ready adjacent 64-byte output pages into one NoC write when they are contiguous in the queue and belong to the same destination segment. It never waits for a second page, and it retains both queue pages on a failed transfer. Simulations cover paired writes, immediate singles, ring wrap, destination segment boundaries, transfer failure and concurrent role execution. The full offline target suite and non-loadable Haiku cross-build pass; driver SHA-256 `2609220bff2feed96de192c870e73922f6fa45adae2343ebdc93b1bd3b1a17e0`, client SHA-256 `486fb45075c7a661f0f2b2260d15bf9cdcc568a6183f250745720410e50e3675`; default control remains `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`. One NoC write replaces two only when both pages are ready; occurrence and performance are unmeasured. Physical launch and no-reboot recovery remain unqualified. No card was accessed. Continue only the rewrite until ready, then benchmark and integrate the assistant.

> **Resident stream lengths, offline (2026-11-22 local):** each shared-role compute worker now caches the validated length of each graph record in a bounded L1 region after its first complete fetch. Later graph executions read only the 64-byte-aligned prefix containing the full typed invocation; header, checksum, fetched padding and copied command are still validated. The cache is invalidated on worker startup, bound to the sealed stream identity, and rejects inconsistent lengths. An actual-record simulation across all 548 decode steps and 120 cores reduced total per-graph record bytes from `134676480` on the first pass to `58913280` on a warm pass, a transfer count, not measured speed. Tests cover changed token/position, all invocation families, wrong lengths, corrupt payload/padding and stale cache identity. The shared graph and its stream/barrier objects now compile with the shared-role flags; the independent image test verifies the exported graph actually calls the cache binding and samples FIFO status. Full offline target and recovery suites and the non-loadable Haiku build passed. Driver SHA-256 `102684dd6a3e6d7be2e79e2b1f94ce504b196c859e93caf475f2eacbc2fb5cb1`, client SHA-256 `486fb45075c7a661f0f2b2260d15bf9cdcc568a6183f250745720410e50e3675`; default control remains `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85`. No card was accessed, no new tokens/sec measured, and physical launch and no-reboot recovery remain unqualified. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Resumable post-reset firmware wait, offline (2026-11-22 local):** when an ASIC reset has completed but firmware has not yet booted after a bounded poll, the dedicated rewrite now retains the same guard ticket and fresh mapping and allows the client to resume that poll without issuing a second reset or remapping PCI. This is allowed only after each scratch probe restored its TLB, the saved TLB still matches, and PCI remains stable; interruption, drift, telemetry failure and uncertain restoration remain non-retryable. An explicit response detail distinguishes the resumable wait from other firmware failures. Simulations cover repeated waits followed by success, stale tickets, TLB drift, client retry limits and retained failures. The recovery suite and non-loadable Haiku driver/client cross-build passed: driver SHA-256 `4fd0e6b7b46b8efc3f5eba17f75dc6f639b0af9dd12a237b02017dde65b9d018`, client SHA-256 `486fb45075c7a661f0f2b2260d15bf9cdcc568a6183f250745720410e50e3675`. Physical reset/reinitialization and no-reboot recovery remain unqualified; no model run or accelerator command was issued. At 2026-11-23 05:35 UTC strict Haiku SSH showed no 27B/4B diagnostic, installed stable driver SHA-256 `041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403`, and CPU-only guard status driver55 generation1 `quiesced0 cold0 active_owner0 open_count1 guard1`. Repeat the preflight immediately before any future card access. Continue only the rewrite until ready, then benchmark and integrate the assistant.

> **Audited resident descriptors, offline (2026-11-22):** the dedicated rewrite can reuse a one-shot previous descriptor receipt after the existing full completion audit, compare the eight mutable words, and write/read back only changed words. A composed test of the actual ledger and PCI transport across 120 simulated cores reduced warm-update old-record reads from 15,360 to 960 per graph, excluding unchanged write-readbacks; the completion audit still reads the full descriptors. Tests cover missing/stale/reused receipts, mutable drift, static corruption detected by device descriptor validation, and partial writes. This source-level count is not a throughput measurement. The optimized path remains disabled: the real scope explicitly denies the necessary lifetime-spanning descriptor ownership proof across audits, ioctls and publication, so the driver retains the full-read path. Full offline rewrite and recovery suites, focused fleet and guard tests, and the non-loadable Haiku cross-build passed. Driver SHA-256 `57530e2a50b1e6fcf999887debcb1e22d5da393659fefc8ac6b839c8b7d16546`; default-control SHA-256 `07b59c292cd38926e29d45ef7c9bde655ad1db20237f8fdf30ce8e81975f2f85` matches the previous build. Physical ownership, launch, reset/reinitialization and stable-driver/27B restoration remain unqualified; no card was accessed and no throughput was measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Route-only peer leases, offline (2026-11-22):** the exclusive rewrite now configures each held peer’s NoC without reading or allocating an 8 KiB L1 backup. Before service installation, cleanup audits the held fleet, refuses an unexpected dirty L1 or command, and restores the recorded NoC and TLB state; uncertain operations retain/quarantine the owner. The removed old peer-lease loop read 2,048 words on each of 119 peers (243,712 32-bit reads), a source-level setup count, not a measured throughput change. Actual lease and composed 120-worker simulations cover success, dropped configuration, unexpected commands/dirty state and failed TLB restoration; the full offline rewrite and recovery suites and non-loadable Haiku cross-build passed. Driver SHA-256 `90abfc3ae9c4a84ba8acb315fb8f1fe86d86e918abdd19311315c21f8b67a493`; the default control driver is unchanged. Physical ownership, release, reset/reinitialization and stable-driver/27B restoration are unqualified. No card was accessed and no throughput measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Dedicated held-fleet preparation, offline (2026-11-22):** model binding for the exclusive rewrite now prepares the 120 held workers’ NoC/store leases without staging or booting the older Q38 dispatcher. Preflight and route checks accept the dedicated held state, and the panel mapping no longer allocates old per-core panel snapshots. If the service never starts, a fleet-wide held audit precedes reverse-order peer lease restoration; uncertain preparation or close retains/quarantines the owner. The client can request guarded reset after a failed pre-service BANK_END. Simulations exercise the actual extracted preparation entry point, rejection of an old booted owner, partial preparation, pre-service close failure and recovery; the full offline rewrite and recovery suites and non-loadable Haiku cross-build passed. Driver SHA-256 `8e54212202bbe54bb7165f223ad88e8b7e6a3bb7ae90bd2c3b30f613fc2ec8ee`, client SHA-256 `2b000245f136195748cee4f87f364e9b446c19773f4e1041d7a487158b9b778c`. The separate default control driver hash remains unchanged. Physical ownership, release, reset/reinitialization and stable-driver/27B restoration remain unqualified; no card was accessed and no throughput measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Dedicated compute takeover without legacy snapshots, offline (2026-11-22):** the exclusive resident service no longer captures prior compute L1/boot state or initializes the snapshot-handoff mailbox. It marks reset retirement necessary before its first hold attempt, so a failed hold, role stage, partial launch or clean stop cannot fall through to old-state restore/resume. The dedicated build requires the shared role owner and reset-retirement path. In a successful prior install, the removed snapshot loop performed two reads of 264 KiB per core: 16,220,160 32-bit reads over 120 cores, a source-level setup count rather than a decode-speed measurement. Dedicated simulations cover all 120 workers, a failed hold that took effect, failed role staging, partial release and reset-required close; the complete offline rewrite and recovery suites and the non-loadable Haiku cross-build passed. Driver SHA-256 `69c1f955b1e669c10c32ddbf3ad7a71ba9dad98903edab75c55c691bc2ff1895`. Real ownership/release, ASIC reset/reinitialization and restoration of stable-driver/27B usability remain unqualified. No card was accessed and no throughput measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Dedicated release contract, offline (2026-11-22):** the exclusive takeover no longer calls the older snapshot-path release proof. Its independent gate must still establish exclusive ownership, partial-fleet stop/hold and reset/reinitialization, and is still hard-denied for hardware. The simulated PCI adapter test requires dedicated release to work with the snapshot proof disabled, while refusing an unavailable dedicated proof and an uncertain retry; it rejects the preceding implementation. The full offline rewrite suite and takeover Haiku cross-build pass, and the driver hash stays `b5ea23fa0e40db9a6b4cd6b3965b1c8a99e286e0a2472f3eb0f57e5e1529444d` because the physical dedicated gate remains false. The [Tenstorrent tt-lab reference](https://github.com/tenstorrent/tt-lab/blob/f601ace5228d2db166fedd9c9c2db7dae51c89e2/src/tt_backend.cpp) takes exclusive ownership and holds, stages and releases workers; that reference does not qualify this Haiku implementation. Physical launch and reboot-free recovery remain unverified. No card was accessed and no throughput measured. Continue only the rewrite until ready, then benchmark and integrate the assistant.

> **Post-reset MMIO mapping attributes, offline (2026-11-22):** the fresh post-reset audit, ARC queue, notification and DBI mappings now pass `B_UNCACHED_MEMORY` in Haiku’s address-specification argument rather than the protection argument. The actual mapping adapter test checks both arguments for every simulated window; it rejects the preceding code and passes with the fix. The recovery suite and non-loadable takeover Haiku cross-build passed; driver SHA-256 `b5ea23fa0e40db9a6b4cd6b3965b1c8a99e286e0a2472f3eb0f57e5e1529444d`. RISC-V currently defaults an unspecified physical mapping to uncached, so the old placement does not by itself prove the old mapping was cached. Physical ownership, launch and reset/reinitialization remain unqualified, no card was accessed, and no throughput was measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Cold/graph fault recovery admission, offline (2026-11-22):** the explicit live-owner reset now admits a retained graph fault even if the service still reports ready, and a cold exclusive session even if service installation did not finish. If close cannot return a result, the client can request the same fault-gated reset; the driver still verifies exclusive ownership, team, generation and detached role scope before quarantine or reset. Simulations cover both new driver states, a rejected healthy session, and close-transport failure with successful and failed recovery. The reload suite and non-loadable Haiku cross-build passed; driver SHA-256 `4ca4d6cc4f7b9deaebf4640aafd4ab8edc4e7e842072a4f74ec70cf0f9d895f2`, client SHA-256 `984202a0a3b72857c9b90f8788d5bbcb763e128b9f7572a269a32eeb95a9acea`. Physical ownership/release, ASIC reinitialization, no-reboot recovery, and stable-driver/27B restoration are unqualified; no card was accessed and no throughput measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

> **Live fault reset handoff, offline (2026-11-22):** the dedicated rewrite prototype now offers an explicit request to abandon and quarantine a faulted exclusive session when normal stop/recovery fails. It checks caller, sole ownership, build/generation, detached role scope and fault state before using the existing ticketed reset and PCI/firmware/fleet/DMA audits. A timeout or owner-cleanup retry resumes the same transaction without issuing another reset. The client requests the fault path only after an indicated shutdown fault and verifies idle state and the new generation. Simulated tests cover refusals, timeout/cleanup retry, lost replies and failed unload; the reload suite and non-loadable Haiku cross-build passed. Driver SHA-256 `41cf4bbf43fe72f19f56f73d3553ec19cd13d31bfc294b381face081a06170f4`, client SHA-256 `acf8565d40b9220d32d1dbdb38fc65f5cdfb98e7996c279e6d65edff2f18c9e5`. This remains compile-only: the physical ownership and release gates are closed and real reset/reinitialization and stable-driver/27B restoration are unqualified. No card was accessed and no throughput measured. Continue only the Tenstorrent-style rewrite until ready, then benchmark and integrate the assistant.

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

> **Role recovery-to-PCI adapter, non-loadable witness:** `PioneerQwen35RoleFleetPciIO` translates the actual role owner's exact boot, shim, extra-code, B/NC local-RAM and reset-disable operations into fixed Haiku BAR views, selecting and rechecking the pinned tile route. It samples the held reset state and both NoC IDs/command and outstanding counters twice, and checks stable default TRISC0/NCRISC boot routing; those local checks do not prove the fleet or incoming traffic is quiet. A compile witness instantiates the adapter with the actual `PioneerSessionIO`, borrowed scope, Haiku mapper and role owner; host tests exercise all 120 simulated routes, busy NoCs, boot overrides, valid/invalid ranges and denied proofs. Real held/quiet, default boot, RAM-stability, snapshot and write proofs still return false; a later guarded install caller remains fail-closed. The full non-loadable driver SHA-256 is `c5a1c6349e5fc3e84fc31e4c34799283b667cadf1f274efa6aa807a66e5c2f22`; its conservative `18016/18432` stack estimate does not cover a real role callback path. No hardware access occurred.

> **Borrowed install lock, non-loadable witness:** `Qwen35DataflowControl` now creates a one-shot role scope only after its existing session lock is acquired, checks owner/team/thread/generation and detaches it before unlock. A failed detach or semaphore release latches recovery. The scope is available during installation without requiring the resident service to be Ready, but it denies every role-memory and control write. A later exact-run fault scope admits the retained graph for audit and promotes close only after the existing graph audit settles. Simulated tests cover thread/team/generation changes, retained graph denial, repeated attach and unlock failure. The earlier non-loadable driver compiled with SHA-256 `32824d7e35355b3884160df227094003b9b647db3f8e975e61f791e1a2f4ac5f`, graph-install stack envelope `16448/18432` with a 2048-byte allowance (not whole-program proof). No role mapper/reset is invoked by this scope.

> **Writable role views, non-loadable Haiku witness:** scoped writable mappings cover only fixed role boot words, low shims, extra code, private B/NC RAM and `DISABLE_RESET`; register writes may change only bits 6–9 and soft-reset/NoC/PC writes are excluded. Access checks the pinned BAR and route before and after each word and requires caller-provided held/snapshot proofs; failed writes or unmaps remain the owner's responsibility. Real Haiku mapper, session and route-guard types now compile together, but the real scope deliberately denies **all** role memory and control writes. Earlier recovery+dispatch+role-write driver witness SHA-256 `310a324b301d0637edeb08c38d08825adfddb360f9e9932bc031d3845455a7fc` rejects load in `init_driver`; no writable-view ioctl existed in that earlier witness. Simulated tests cover ranges, masks, stale routes, partial writes and map cleanup. A later guarded install binding remains fail-closed; no card was accessed.

> **Reproducible shared-role firmware, offline:** the recovery-only `build_qwen35_shared_role_images.sh` exports fixed-address reader/writer code and startup shims into `PioneerQwen35SharedRoleImages.h`, checking resolved symbols, entry/park/tick addresses, supported instructions, placement limits and stack estimates. A guarded `PIONEER_QWEN35_SHARED_ROLE_GRAPH_COMPILE_ONLY=1` path in `build_qwen35_dataflow_service_image.sh` exports the actual graph/coordinator image and rejects the old single-controller projection. A simulated adapter stages and verifies the actual exported reader/writer images and boot words on all 120 tiles through the service and recovery owners, then restores snapshots. Both builds require dispatch and recovery compile-only flags. The latest shared graph text SHA-256 is `342e7e3561f85db731bb5eb3636c76468ddf51be06bf322bab5175d1b7584ec8`, dispatch SHA-256 `40ab598f452724689f5fdf217a4f460b53adfe671a75c8190860d80a48d62d55`, and reader/writer header SHA-256 `61973e43dd1a3a5979bd26d75daecab205e1ad1b5016fd276c34af10d9a52c2f`. These are non-loadable witnesses; no on-card correctness or throughput result exists.

> **Shared-role service lifecycle, guarded witness:** the existing `DataflowServiceOwner` now has a recovery-only compile path for an already-locked role lifecycle. It asks IO to capture/stage all role state while held, initializes and reads back each role mailbox, consumes the role release ledger, rechecks all 120 resident images/boot/queues/mailboxes/barrier slots, then uses role-aware startup/readiness callbacks. Close requires a role-aware stop/hold and role restore before resident restoration, then rechecks both role and resident snapshots before resume. Fault-injection tests cover stage/arm/start/stop/restore failures, corrupted mailboxes and cross-tile staging. A separate simulated adapter joins the actual service owner to the actual role recovery owner and covers partial start, partial reset-control writes and resident-restore corruption with successful retries. The Haiku SDK compile with fake IO reports audit/install frames of 624/752 bytes; this is not a complete kernel stack proof. Only the guarded PCI L1 range was extended for held role-mailbox access; running writes remain denied. **A later fail-closed Haiku build binds the role owner and callbacks; no role-aware reset is performed and no new service was loaded.**

> **Role reset preservation, simulated:** per [Blackhole soft-reset documentation](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/TensixTile/SoftReset.md), `RISCV_DEBUG_REG_DISABLE_RESET` bits 6–9 preserve BRISC/NCRISC local RAM and debug `DR` registers across reset exit. The role fleet owner snapshots each complete word, sets only those four bits before consuming the one-shot release ledger, double-checks all 120, and restores and rechecks the original words during teardown. Simulated partial writes, cross-tile corruption and later resident-restore corruption retain the owner for retry. The owner also rechecks all role images/RAM/control after resident restore. None of this proves physical release, PC/NoC quiet, timing or complete recoverability: IO/control writes remain unbound and no hardware was accessed.

> **Lifecycle integration constraint:** the read-only Haiku scope described below is a separate lock-acquiring witness for a `Ready` service. Actual installation already holds the session lock and changes the service to `PhaseInstalling` before holding the fleet, so that witness cannot be reused by simply calling it: doing so would reacquire the same lock and its Ready proof would fail. Additional role snapshots/staging belong inside the existing locked install, after all TRISC cores are held and before any starts; teardown must settle the additional roles before restoring or resuming the old fleet. The existing PCI memory `StartCore` and `StopAndAuditAll` only understand the single-controller `0x47800`/`0x46800` reset states and require a role-aware lifecycle before BRISC/NCRISC can run. The later guarded binding supplies the integration scaffold but not role-aware startup or fault recovery.

> **Graph-to-role bridge, compile-only:** the actual graph worker executor now conditionally dispatches projections to the shared-activation compute coordinator, using a contiguous role-mailbox sequence independent of graph steps and retaining failure rather than falling back. A concurrent host test interleaves projection and non-projection steps, checks consecutive role sequences, and rejects pending, stopped and exhausted mailboxes. This path is guarded by both the dispatch and recovery-witness flags; a build missing the recovery flag is rejected. The role graph's TRISC0 service disables and verifies L0 cache before touching shared state. The existing graph, barrier, executor and shared-role compute link together in the reserved L1 code regions, without the old single-controller projection. This is a non-loadable witness only: the B/NC images, boot/reset, fleet ownership, fault quiescence, NoC/cache visibility, stack bound and end-to-end behavior still require physical integration and qualification.

> **Shared-role cache policy, offline:** [Blackhole cfg0](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/TensixTile/BabyRISCV/CSRs.md#cfg0) resets with `DisLowCash` clear, enabling the non-coherent L0 data cache; the default in [Tenstorrent firmware](https://github.com/tenstorrent/tt-metal/blob/main/tt_metal/hw/inc/internal/tt-1xx/risc_common.h) is to disable it. The shared-role BRISC/NCRISC startup shims and TRISC0 compute coordinator now fence, set only that bit and read it back before reading shared state, parking or failing if it is clear. RV32 target disassembly is checked. This is not a running service: shared visibility, NoC completion/flush, the live TRISC0 lifecycle and restoration of prior machine state still require hardware integration and testing.

> **Selected-tile held/NoC samples, non-loadable witness:** the actual Haiku view samples reset `0x47800`, both logical tile IDs, all four initiator doorbells and 32 outstanding-request counters on each NoC twice through the guarded read-only mappings. Host tests cover all 120 tile routes, both NoCs, each busy slot and wrong reset/IDs. This samples a selected tile only: it does not establish fleet-wide quiet, prevent incoming traffic, or prove the local-RAM initialization window has elapsed, and it grants no RAM access. No ioctl or live call is registered. A full non-loadable driver compile succeeded (SHA-256 `3bc066501ce05df368915f2de16909266888eb70e5b84932665bc63d925ae62c`; kernel stack estimate `17968/18432`, not whole-program proof). Do not install it. Tenstorrent's [Blackhole guide](https://github.com/tenstorrent/tt-metal/blob/main/tech_reports/Blackhole/BlackholeBringUpProgrammingGuide.md#l1-data-cache) also requires the reader to invalidate previously read cached L1 data and explicit NoC flushes in relevant cases; this rewrite's real cache and publication policy remains unproved.

> **Actual Haiku session types, non-loadable witness:** the read-only view and selected-route guard now compile against the real driver session, same-owner lock, cookie/team, generation and stable-service types. The scope deliberately denies all boot/L1/private-RAM reads until held/NoC/RAM-stability proofs are implemented. Host mocks exercise lock, team, read-only mapping flags, partial-map cleanup and failed lock release. A full driver build with both recovery and graph compile-only flags succeeded; `init_driver` rejects that binary, and no ioctl or card operation is registered. Witness driver SHA-256 `1c1a0d39e7d73e66fc954fc72aa5f9ffd6b962c04019ca2a930fbef756c6b47d`, conservative kernel stack estimate 17968/18432 bytes, not a whole-program proof. This is not the tested tag0168 driver.

> **Selected-route guard, compile-only:** the read-only mapping guard checks the pinned Pioneer BAR profile, an injected exclusive rewrite-session proof, both current TLB translations before and after, and two matching NoC logical tile IDs. Mocks cover all 120 routes, stale TLBs, unstable IDs and missing ownership; the real exclusive-session scope is wired only into the non-loadable witness above, and this is not an idle or write-safety proof.

> **Haiku read-only role mapping, compile-only:** fixed BAR window planning covers both boot pages, low shims, extra code, private B/NC RAM, control, PCs and both NoCs. The mapping lifecycle uses read-only Haiku mappings, fences reads, validates BAR bounds, restricts reads to owned memory and known status registers, and requires caller-supplied route and memory-safety proofs; partial map/unmap failures remain tracked. Mock tests and Haiku kernel SDK compilation pass. A real route guard is connected only in the non-loadable witness above; no live ioctl, mapping call or card access is connected, and idle and write authority are not established.

> **Fleet image and private-RAM recovery, simulated:** both 8 KiB private B/NC RAMs per tile are snapshotted and marked dirty before a one-shot release ledger can be consumed. Restoration requires held roles and a proven completed RAM-initialization window; image and RAM regions are rechecked after all writes. Tests simulate reset-cleared RAM, partial/misrouted writes and cross-region corruption. The IO, reset/release, PC/NoC audits, cache setup and existing TRISC0 restoration remain unbound, so this is not yet no-reboot driver recovery.

> **Composed 120-tile image ownership, simulated:** extra code, boot words and shims are captured before staging; the complete fleet is checked after staging. Restoration puts boot entries back before code and rechecks both regions afterward. Tests inject cross-region corruption and require a retry before claiming these images restored. Local RAM, reset, NoCs, queues and physical PCI access remain outside this contract, so it is not a complete driver recovery path.

> **120-tile boot/shim recovery, simulated:** all boot/shim snapshots precede staging; the fleet is checked again after staging and after restoration, including untouched tiles. Partial and misrouted writes retain snapshots for retry. The subsequent simulated image owner composes this with the extra-code owner; neither is bound to physical hardware.

> **Boot and shim owner, simulation only:** a per-tile transaction double-reads both original boot words and the low shim range, requires default reset-PC routing and held roles, verifies both shims and untouched bytes before changing boot words, and restores the original boot words before the shim snapshot. Tests inject partial writes, bad reset routing and shim corruption. It is not connected to PCI, does not restore local RAM or reset, and does not provide fleet-level boot recovery.

> **120-tile role-code recovery contract, simulated:** capture all extra L1 snapshots before staging; verify all staged images and untouched bytes across the fleet; retain snapshots on partial writes; restore and recheck every captured tile. Tests inject partial writes, corruption of an earlier restored tile, and corruption of an otherwise untouched tile. This only covers the additional code region. It is not bound to PCI and does not establish boot, shims, local RAM, reset, NoC or full-driver recovery.

> **Shared-role startup witnesses:** two 52-byte low-L1 shims set an 8 KiB private B/NC stack, repeatedly invoke the bounded role tick, and enter a fixed park loop after stop or invalid state. Both link with their shared-activation role bodies; a conservative sum of compiler-reported frames plus a 1024-byte allowance is 5456/8192 bytes, not a recursion or whole-program proof. There is no boot image, reset control, physical owner, cache qualification or safe-release proof, so these witnesses must not be loaded.

> **Boot-address caveat:** offline RISC-V jump encoding reaches proposed low-L1 shim slots from the default BRISC/NCRISC reset PCs. These shim slots lie below `0x80000`, matching the earlier official Metal firmware-base bound, but the shims themselves, their onward branches to higher code, and safe boot on the installed firmware are unverified. The existing snapshot covers the proposed shims, while its PCI write gate does not admit them. No boot words or reset controls are touched.

> **Shared activation variant, offline:** the reader publishes one readiness ticket after packing the activation once, and the Tensix compute role reads immutable packed chunks directly while consuming weight pages. This avoids copying an activation page per weight tile in the queued-input prototype. Concurrent mocked Q4/Q5/Q6/Q8 queue-loop tests verify direct cache pointers and hold the readiness ticket through every matmul; an injected reader failure retains the command. Separately linked reader and writer witnesses fit the provisional slots. This variant is not deployed, does not yet prove cross-RISC cache/NoC visibility, and has no measured speedup.

> **Coordinated queue-loop simulation:** the real reader/compute/writer queue loops now run under the per-tile coordinator in a concurrent mock; the coordinator waits for both role receipts, checks and rebases the queues, and then acknowledges. The mock uses synthetic operations, not numerical inference or hardware, and the physical service remains unintegrated.

> **Inactive queued shards:** reader roles for cores assigned no output groups now acknowledge without fetching or packing the activation. The behavior is covered by an inactive-core mock; the existing single-worker hardware path already skipped such cores. This is not yet a hardware performance result.

> **Role coordinator, simulated:** a bounded coordinator issues one per-tile handoff, runs a supplied compute body, waits for both terminal role receipts, and acknowledges only after clean queue audit/reset. A failed compute, failed role, failed queue audit or missing receipt retains the command. Concurrent mock tests exercise clean reuse and failure paths. A compile-only adapter now binds the real queued Tensix compute body; it is not connected to the live graph, and the proofs and shutdown are not hardware-qualified.

> **Role service decision, simulated:** a bounded single-step service decision now handles idle, one command, wait-for-acknowledgement, retained fault and stop request without retrying a failed body. The later offline shim witnesses contain a loop and park point, but are not loadable processor images; the physical owner still needs to implement safe launch, bounded polling, parked-PC/NoC audit and full restoration.

> **Role command adapters, offline:** reader/writer once-only adapters now invoke their concrete queued bodies and publish a terminal mailbox receipt only after the body returns. A body failure retains a failed receipt and cannot be acknowledged as clean. With those adapters linked, the proposed reader/writer slots use 17,076 / 8,204 bytes of 20,480 / 12,288 bytes respectively when linked through the bounded service tick. The later offline shim witnesses add a separate low-L1 loop and park point; physical startup remains unimplemented. These link witnesses remain unowned and non-deployable; safe startup, stop, and recovery are not implemented.

> **Per-tile role handoff, simulated:** a 64-byte single-writer-per-field mailbox fits in the already-saved L1 view after graph traces. It uses monotonically increasing command/acknowledgement sequences and separate reader/writer receipts; failed or incomplete roles cannot be acknowledged as clean. Tests cover concurrent reader/writer handoffs, reuse, out-of-order commands, missing compute/queue proofs, failed roles and sequence limits. The protocol also models clean stop receipts and a separate fault-stop request only after both roles return and a trusted caller proves compute and queue writers settled. Fault-stop does not acknowledge the failed command. These are receipts, not proof of parked PCs or quiet NoCs: the protocol neither launches roles nor proves queue visibility, hardware recovery or safe resource release.

> **Role code snapshot contract, simulated:** a new isolated owner double-reads the proposed extra L1 region, keeps its snapshot after uncertain partial writes, requires held-role checks during staging/restoration, rechecks both staged images before a future launch, and verifies the entire restored region after all writes. Tests inject unstable reads, partial transfers, release of a role, and late corruption. This contract is not bound to the driver; it does not save or restore boot words, local RAM, reset, caches or NoC state, and is not a safe-release or no-reboot recovery proof.

> **Tag0168 hardware:** the exact phase-reduction candidate generated correct ` Paris.` at 0.247897 tokens/s for one decoded token, with 610.801086 s load and 18.771527 s prompt. The guard passed; clean unload and stable driver55 generation20 independently verified. This is approximately 3.5% faster than the previous short profile, not a sustained benchmark; 100 tokens/s remains unmet. See `benchmarks/qwen35-dataflow-tag0168-20261122/RUN.md`.

> **Offline role placement:** proposed reader `0xb0000..0xb4fff` and writer `0xb5000..0xb7fff` are disjoint and link within their size limits. The current PCI owner ends at `0xb0000` and cannot load them. The plan requires snapshots and verified restoration of the additional L1, boot and processor state before launch; no role image is installed by this change.

> **Concrete queued role bodies, offline:** the reader, Tensix-compute, and writer entry bodies compile for RV32IMF without unresolved imports or A-extension instructions. Separate link-size witnesses at a shared hypothetical base measure 15,228 / 18,069 / 6,864 bytes respectively; they are not placed together, not service images, and have no boot or owner. Their stack budgets and safe shared-L1 allocation still need an end-to-end audit before any hardware use. Tenstorrent documents an 8 KiB private local data RAM for each BRISC/NCRISC, a potential stack location; their reset clears local RAM, so save/restore and startup ordering must be proven, and a single-frame stack measurement is not a call-chain bound. The hypothetical code base lies outside the current owner’s saved L1 view. See the [BabyRISCV memory map](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/TensixTile/BabyRISCV/README.md#local-data-ram).

> **Processor identity correction:** the current image enters through the TRISC0 boot word at `0x6000`; the driver releases TRISC0 with reset value `0x46800`. Earlier notes calling this resident worker BRISC were incorrect. Each tile still uses its Tensix matrix engine for projections. The BRISC/NoC reference describes an upstream inspiration, not the processor running this image. See `PioneerQwen35DataflowPciMemory.h` and `TENSTORRENT_PORT.md` for the reset contract.

> **Separate-role rewrite, offline only:** the earlier generic reader and writer did not bind the concrete NoC operations, and the concrete Tensix compute adapter accepted only the single-worker activation cache. Reader/writer now bind their shard, with rejection propagated through role failure, and an isolated compile-only compute variant checks that the weight and activation queue pages have the same ticket-owned index. Concurrent mock role tests and target checks cover the contracts. This is not yet a device program: separate RISC images, safe launch/stop, L1 ownership and fault recovery still need implementation and hardware qualification. The currently running tag0168 binary predates these changes.

The next device lifecycle must retain and audit each additional RISC's boot, reset and code state, give its stack and shared queues disjoint saved L1 ownership, and prove all roles stopped before restoring any image. Queue publication must account for Blackhole's optional L1 data cache and its NoC completion/flush rules; ordinary host simulation does not prove cross-RISC visibility. The current owner starts TRISC0 only and must not be treated as an implementation of this lifecycle. See Tenstorrent's [Blackhole bring-up guide](https://github.com/tenstorrent/tt-metal/blob/main/tech_reports/Blackhole/BlackholeBringUpProgrammingGuide.md) and [reader/compute/writer lab](https://github.com/tenstorrent/tt-metal/blob/main/docs/source/tt-metalium/tt_metal/labs/matmul/lab1/lab1.rst).

> **Tag0168 candidate:** within the rewrite, Q4_K/Q5_K panels skip two matrix fidelity phases that are zero for their operand range; Q6_K/Q8_0 retain four. Exact arithmetic and target image tests pass. The subsequent hardware diagnostic and clean unload are recorded above.

> **Tag0167 profile completed:** one correct ` Paris.` diagnostic at 0.239486 decode tokens/s; load 610.592493 s, prompt 19.426467 s, decode 4.175606 s. Core0 projection accounted for 57.975% and sync/other for 36.774% of recorded cycles, including movement and instrumentation respectively. Clean unload; stable driver55 generation18 independently idle. See `benchmarks/qwen35-dataflow-tag0167-20261122/RUN.md`.

> **Bandwidth planning, not a measurement:** Tenstorrent lists the [P100A](https://docs.tenstorrent.com/aibs/blackhole/index.html) with 120 Tensix cores, 28 GB GDDR6 and 448 GB/s peak memory bandwidth. The authenticated upload in tag0166 was about 2.74 GB. As a rough thought experiment, reading that many bytes once per token at 100 tokens/s would require about 274 GB/s, or 61% of the listed peak, before other traffic. The actual bytes read per token have not been measured and need not equal uploaded bytes; do not treat this as a measured bandwidth or a performance guarantee.

> **Performance design reference:** Tenstorrent’s [LLM bring-up guide](https://github.com/tenstorrent/tt-metal/blob/main/tech_reports/LLMs/llms.md) describes decode matmuls with activations/results width-sharded in L1 and weights sharded across DRAM, using multicast and reusable matmul programs. The current rewrite retains a resident graph and 120-worker sharding, but its projection worker still reads and packs an activation on each core and serially drives many small matrix panels. The tag0167 timing experiment measured projection operations and synchronization/other on one decode; core0 timings are not a whole-fleet utilization measurement. Preserve Q4_K_M numerical validation and no CPU fallback when adapting the design.

> **Tag0166 two warm runs verified:** both produced ` Paris.`; model load 614.806798 s, prompts 19.469486/19.464716 s, decode 0.239027/0.238991 tokens/s. Guard passed and stable driver55 generation16 was verified idle after clean unload. The 100 tokens/s goal remains unmet; the subsequent tag0167 profile completed cleanly.

> **Tag0167 timing candidate:** device-only core0 cycle bins for a completed decode, including projections, recurrent operators, record fetch, authorization and synchronization/other. It is for diagnosis, not a fleet-utilization proof, and adds measurement overhead. Full target suite/builds pass; the profile completed with correct output and a clean unload, as recorded above. See `QWEN35_4B_NEXT_SESSION.md` for hashes.

> **Tag0166 first correct output:** verified ` Paris.` (`11751,13`); load 617.160613 s, prompt 19.503739 s, one decode 4.190600 s (0.238629 tokens/s). Client exit0 and clean unload; wrapper restored stable driver but its final awk validator had a syntax error. A corrected validator passed self-test on Haiku and the two-warm-run guard subsequently passed. Performance remains far below 100 tokens/s. See `benchmarks/qwen35-dataflow-tag0166-20261122/RUN.md`.

> **Tag0165 hardware:** graph deadline at step8, no output; the graph recovered with same-owner close and clean unload without reboot. Stable driver55 generation12 was verified idle. The build still had incorrect FP32 readback, fixed in tag0166; see `benchmarks/qwen35-dataflow-tag0165-20261122/RUN.md`.

> **Tag0166 readback correction prepared:** the rewrite left config3 in signed-integer conversion mode while exporting SFPU FP32 values. Tag0164 Dst zero readouts were therefore inconclusive; tag0166 switches to raw FP32 for output and trace, then restores integer mode for dot checks. The prior metadata-selector and alias corrections remain included. The full target suite and trial/normal/non-loadable witness builds pass; hardware impact is unverified.

> **Tag0165 selector fix:** tag0164 observed zero Dst readback values, but later review showed readback was in integer-conversion mode, so those FP32 samples are inconclusive. The rewrite now restores the fixed-format Dst metadata selector (config73 `0x13`) before the SFPU accumulator metadata load; ordinary vector unpack had reset it to `3`. This is in addition to the affine alias fix. Full target suite/builds pass; hardware effect remains unverified. See `QWEN35_4B_NEXT_SESSION.md` for exact current artifact hashes.

> **Tag0165 prepared fix:** the earlier affine staging copy overwrote the source minima with coefficients because `PioneerQ38Panel` has a 512-byte operand prefix, not the offline Q5 tile's 576-byte prefix. The rewrite now places the panel so its fields directly match the SFPU source addresses, with compile-time layout checks and a distinct-values test. Full target suite and builds pass; hardware impact is unverified. Tag0164 unloaded cleanly and stable driver55 generation10 was verified idle.

> **Tag0164 trace:** Dst samples read zero with integer conversion enabled and are inconclusive about FP32 data. The run unloaded cleanly; see `benchmarks/qwen35-dataflow-tag0164-20261122/RUN.md`.

> **Tag0163 candidate:** restored qualified Tensix source-window initialization and destination clear, and added bounded core0 first-projection samples for packed activation, scale, dot, and coefficient. Tag0163 hardware showed matching nonzero matrix dots but zero projection output; see `benchmarks/qwen35-dataflow-tag0163-20261122/RUN.md`.

> **Tag0162 hardware trace:** token0 embedding samples match the independently decoded GGUF; layer0 and final RMS samples are nonzero, while sampled first QKV, gate, and final output projection pages are all zero. Output remains incorrect; unload was clean and stable driver55 generation6 is idle. See `benchmarks/qwen35-dataflow-tag0162-20261122/RUN.md`. Continue only the rewrite and investigate the projection path.

# Qwen3.5 Tenstorrent device-program rewrite

Tag0162 adds a diagnostic-only root-worker trace at eight graph steps and
a CPU-side query for the last audited argmax score. It reads raw device bits
only after graph completion, checks two stable copies and verifies each sample's
sequence against the audited result. The trace is not an inference input or
release proof. An independent Q6_K decode of token0 from a hash-matched GGUF
prefix gives rows16/31 as `0xbd2f21c0`/`0x3cb4c800`; these apply to the trace
only when its last graph input is token0. Trace, protocol, PCI range and full
target tests pass, as do normal, trial and non-loadable witness builds. No
tag0162 hardware result existed at the time of that entry.


Tag0161 passed the former convolution stall and completed one graph run, but
generated `0,0` (`!!`) instead of the verified `11751,13` (` Paris.`). The
raw one-token decode rate was 0.242095 tokens/s after 19.186788 s prompt
processing and 612.118705 s load; because output is incorrect, this is not an
acceptance benchmark. Unload succeeded and stable driver55 was restored idle
without reboot. See the [tag0161 record](benchmarks/qwen35-dataflow-tag0161-20261122/RUN.md).
The rounding-independent SiLU follow-up was not in tag0161; model numerical
correctness remains the next rewrite problem.


The [Blackhole BabyRISCV instruction set](https://github.com/tenstorrent/tt-isa-documentation/blob/e0b765516b78f8faf34004f93d68ad49c59f4725/BlackholeA0/TensixTile/BabyRISCV/InstructionSet.md)
confirms that `fdiv.s`/`fsqrt.s` are not implemented and that float rounding
mode bits are ignored. The SiLU exponent calculation previously emitted
`fcvt.w.s ... rtz`; it now decodes the small exponent with integer operations
so its truncation does not depend on hardware rounding mode. The worker-image
audit rejects these conversions as well as unsupported divide/square root.
SiLU and convolution numerical tests, the graph service image build and the
full target suite pass. This follow-up has not yet run on hardware.


Tag0160 traced the first convolution stall to arithmetic on group0. Its exact
worker disassembly contains `fdiv.s` inside `DataflowSiluProduct`; the device's
existing qualified math uses software division. Tag0161 replaces the runtime
floating divides with that helper and adds an image build guard rejecting
floating divide and square-root instructions. SiLU, convolution and one million
independent software-division comparisons pass; the target suite and all three
Haiku builds pass. This diagnosis and fix still require hardware confirmation.
Tag0160 needed an NVMe power cycle to restore stable driver55; no token or
decode rate was obtained. See [the run record](benchmarks/qwen35-dataflow-tag0160-20261122/RUN.md).


The rewrite client now performs up to four bounded recovery attempts when a
captured graph fault specifically returns `RECOVERY_PENDING`, keeping the
original owner open while workers may settle. Every retry uses the existing
fence and all-core fault audit; all other errors stop retrying. The retry
policy and real teardown integration pass simulation and target builds. This
has not been exercised on the card and does not recover a permanently stuck
NoC or qualify the disabled ASIC-reset prototype.


Tag0160 adds correlated, read-only per-group convolution progress markers for
validation, reads, arithmetic and writes. The progress-aware wait recognizes
forward group/phase movement without treating the marker as a completion or
recovery proof. A simulated all-120-core convolution test verifies marker
order and the regular target suite now includes it. The target suite and
normal, qualification, and non-loadable reset-witness builds pass. Hardware
qualification has not run. The qualification driver SHA-256 is
`8c524d28bcb59107d6772d9a7831ff4c1dec6929978e7c2b66f24eb5c3ab4fc7`.


The guarded tag0159 run reached step7 (layer0 recurrent convolution), after
embedding, residual save, normalization, and four projections, then timed
out with worker0 executing. No token or decode throughput was produced.
In-place recovery failed for this unsettled graph; an authorized NVMe power
cycle restored stable driver55 and preserved the recorded model-file sizes.
The progress receipt is diagnostic and proves neither numerical correctness
nor safe idle. See the [tag0159 record](benchmarks/qwen35-dataflow-tag0159-20261122/RUN.md).


Tag0158 timed out after 8.258036 s with a worker still running; no token or
decode rate was produced. Its unsettled graph could not be recovered in place.
An authorized NVMe power cycle restored stable driver55 idle and preserved both
model files at their recorded sizes. Tag0159 adds read-only, non-authoritative
worker step/phase observations in a separate saved L1 word, and a bounded
progress-aware host wait (30 s inactivity, 600 s total). Tests cover the
progress encoding, idle and total deadlines, terminal and invalid states, and
separation from host-running writes. The target suite and all three target
builds pass; tag0159 has not yet run on the card. The ASIC-reset recovery
prototype remains unqualified. See the [tag0158 record](benchmarks/qwen35-dataflow-tag0158-20261122/RUN.md).


## Scope

Rewrite the Qwen3.5 Work execution path around one resident TRISC0 worker per
Tensix core. Each worker owns one immutable shard, performs bounded NoC 0
reads, issues projections to the Tensix matrix engine, and sends results with
NoC 1. Keep two alternating L1 pages for weight and activation staging, sharded
intermediate activations, and no host inference path. This follows
Tenstorrent's per-worker BRISC programming pattern; it is not a claim that the
Haiku image runs TT-Metalium or its separate NCRISC/TRISC kernels. Keep the
checkpoint history on `codex/qwen35-4b-p100a`.

Complete the device-program rewrite before benchmarking; integrate with the
assistant only after the hardware generation and throughput gates pass.

## Current status (2026-11-22)

The active work remains the Tenstorrent-style device-program rewrite. It uses
120 resident TRISC0 workers and Tensix matrix engines, with no CPU inference
or fallback. Pioneer boots from its existing NVMe: Fedora hands off to Haiku
with kexec. The user confirmed SD cards are no longer used; do not use or
recommend an SD-card workflow.

Tag0153's first hardware qualification loaded the verified 4B model and
installed the graph, but the first prompt graph failed with a retained
session before any token. The no-reboot recovery reached restored-fleet resume
and failed; a second close attempted the old stop audit after restoration.
An authorized NVMe cold recovery restored stable driver55, idle and without a
recovery lock. See [the run record](benchmarks/qwen35-dataflow-tag0153-20261122/RUN.md).

Tag0154 adds graph phase/core failure codes, preserves an exact
settled-graph proof across close retries, and retries only the final fleet
audit after all restored workers have started. A partial start remains
retained and is not replayed. Recovery responses now expose the service-close
failure details. Host simulations and the RV32IMF target suite pass; this
change was qualified in the diagnostic described below. The exact cause of
tag0153's graph failure remains unknown.
Before another card operation, recheck that the 27B diagnostic has exited and
the stable driver is idle.

Tag0154's diagnostic reached worker 0's graph-completion check without a
token. Its recovery failure identified stale service-image trust after the
original dispatcher image was restored, because both images have the same
boot word. Tag0155 invalidates the cached proof before held-core writes and
restoration, and records a device-graph fault receipt for a CPU-only host
query. Partial restoration and timeout cases retain ownership. The 0155
target/simulation suite and all three builds pass; hardware qualification
remains open. The card is back on stable driver55 with no lock after NVMe
cold recovery. See [the tag0154 run record](benchmarks/qwen35-dataflow-tag0154-20261122/RUN.md).

Tag0155 narrowed the graph fault to worker 0's first graph-record fetch and
successfully recovered that settled fault, unloaded the bank and restored the
stable driver without rebooting. No token was generated. Tag0156 adds the
fetch/NoC stage diagnostics needed to localize the remaining error; simulated
tests and all builds pass, but tag0156 has not run on hardware. See
[the tag0155 run record](benchmarks/qwen35-dataflow-tag0155-20261122/RUN.md).

Tag0156 traced the first graph fetch failure to the driver's exact-match check
on NoC counter `0x20c`. Tenstorrent documents that counter as unsupported to
read on Blackhole; the other recorded counters matched and the graph fault
again recovered without reboot. The next rewrite omits that register while
retaining all supported checks; it needs hardware qualification. See
[the tag0156 run record](benchmarks/qwen35-dataflow-tag0156-20261122/RUN.md).

Tag0157 passed the initial graph fetch and two steps, then worker 0 failed in
the first RMS step. The card again recovered and unloaded without reboot. The
next rewrite stages RMS scale records in the NoC-accessible input page rather
than on the worker stack, and adds a receipt for the RMS failure stage; that
fix requires hardware qualification. See
[the tag0157 run record](benchmarks/qwen35-dataflow-tag0157-20261122/RUN.md).

Tag0154's qualification driver SHA-256 is
`152edda1d9214b869eb74299010d1e973da91c6070e7f8f191a4cbb6f597c223`
and its standalone client SHA-256 is
`21e63fed6cdcc05bc3b168d022042245ba3e6eae0c153aaaeb49ad9a23c2f564`.
The normal and non-loadable graph/recovery witness builds also passed. The
worker stack envelope is `7632/8192` and the graph ioctl envelope is
`13856/18432` including the helper allowance; neither is a whole-program proof.

Tag0147's offline fault-recovery work added a bounded graph settlement audit
and a service transition that permits closing but never resumes inference on
the faulted model. A successful audit requires a host-write fence, stable
receipts and backend proofs for both NoCs, queues, worker image, routes, and
graph peer/fetch traffic for all 120 cores before acknowledgements. The
fixture tests failure retention and partial acknowledgements in simulation.
At tag0147 the backend, driver ioctl integration, restored-fleet release and
hardware qualification remained open. The target suite and both Haiku/RISC-V builds pass; the normal
tag0147 driver SHA-256 is
`48ba20fbd4f3118cf1d8a099f37bad8d505a61e3e45eadf3d2dfb5bed337e7ba`,
and the non-loadable graph witness is
`24eca47a2b71b115147bc4263a486960619b20e81827b915e36d67f1c909fa31`.

Tag0148 updates the real non-loadable graph staging adapter to avoid
rewriting unchanged descriptor words on warm runs. It validates the entire
old record and its completed sequence, rejects changes outside the allowed
dynamic words, verifies each changed write, and retains the graph on any
uncertainty. The publisher still reads and compares the complete new record
before any trigger. In a simulated prompt-to-decode transition this reduces
descriptor writes from 128 to seven per core; multiple full-record reads and
the host's per-step metadata replay remain. The target suite and both Haiku
builds pass. The normal driver SHA-256 is
`96c8f9b2d749c54cce873be27d2b6fc184dd3bc4827a1e02c9f48c8d3d83646b`,
and the non-loadable graph witness is
`343a690d5ab7e6a189e1bfbee13df75f7ed53b6988e47494f23464d114f4490e`.
No hardware throughput is implied.

Tag0149 connects that audit to the graph owner's RECOVER request in the
non-loadable build. It checks the owned stream and surrounding context, fences
the last host writes with a same-device read, audits both NoCs and each
worker's stable mailbox and queues, and acknowledges terminal workers only
after the entire fleet passes. A settled session can close but cannot run a
new graph. The service restores and resumes saved worker state before the
retained graph run and candidate are released. Failed or partial audits keep
the owner; later attempts start with another fence and re-audit all workers.
Simulated integration covers audit refusal, retry, all-core acknowledgement,
close and release. The implementation remains non-loadable and has not run
on Pioneer; uncertain outstanding traffic can still prevent recovery. The
target and reload suites and both Haiku/RISC-V builds pass. The recovery
call-path envelope is `6608/18432`, including a 2,048-byte helper allowance;
it is not a whole-program proof. Normal driver SHA-256:
`fdab1921ab9ed3f439ed78d8bef826dc1c881709ed709aae3e8caca6a12b30f6`;
non-loadable graph witness:
`61063443a267b8e7b10e15a57107429e53bac6ee29b92ce4c91e74ea52e615bf`.

Tag0150 builds a distinct graph qualification driver without enabling the
ASIC-reset prototype. Its dedicated tag is `2026112100020150`; a build that
combines the trial and reset-witness modes is rejected. The actual graph
service sections match the non-loadable graph witness byte for byte. Offline
target/reload tests and normal, witness and trial cross-builds pass; no trial
has been installed or run. Trial driver SHA-256:
`aaf92ec23d14be2b62edadf230f1c3fc317e4d2bee67f76647a55cdb814a15f6`;
normal and non-loadable witness drivers:
`385db06798142123d7efe56bbe8f671217f8a9525d1faab455901df8603681f1` and
`2e7c2b32598dfa51ff026fba5750782bfe4a44f764c0cf69aba4b9935ee3a05f`.

An important tag0150 performance limit was its barrier: 548 decode
steps times 119 peers times two blocking release writes makes 130,424
sequential coordinator writes per token. Ten milliseconds per token would
allow roughly 77 ns per write if every other operation cost zero. This is a
source-level count, not a measurement. Continue the rewrite toward the
Tenstorrent-style multicast/semaphore pattern, with explicit proof of the
actual router masks, NoC1 coordinate mapping, receiver set, acknowledgement
accounting and data-before-commit ordering. Do not run a benchmark until the
rewrite is complete.

Tag0151 replaces the serial root release loop with a fixed four-way unicast
tree. The coordinator still gathers all 120 receipts; every receiver checks
its committed release before forwarding to its fixed children, and arbitrary
peer release edges are rejected. All 120 cores remain compute workers. The
NoC1 audit now checks translation for all used worker coordinates at each
sender. Root release sends fall from 130,424 to 4,384 64-byte transactions per
decode token; total unicast releases across the fleet are unchanged. Simulation
covers out-of-order scheduling, partial relay failure, malformed and torn
releases, route restrictions and drift. The target/reload suites and normal,
non-loadable witness and qualification Haiku cross-builds pass, but this is
not a hardware result or a throughput measurement. Qualification driver tag
`2026112100020151` SHA-256:
`d3580922016123209304a033ac25ca56a25b3630688673a26714125d1cdf090a`;
normal and witness drivers:
`ac7945b2b377c2da2ffc0f4eef2b0a48b5dc59ffafe7220c3fe9acb15bcb2bb0` and
`1fa285843b4615fffb638e451df5c1353beaf2f05d1dd19219dc18c00f0beb6d`.

Tag0152 extends the tree to collect arrivals. Every node checks the committed
receipts from its own children, aggregates the earliest failing worker and
sends one receipt to its parent. Worker 0 therefore polls four child receipts
instead of 119 direct receipts at each graph step. Arrival slots are cleared
and checked on every relevant held core, and the transport admits only a
child-to-parent arrival or a parent-to-child release. Failure identities
outside the reporting subtree and releases that erase known failures are
rejected. The target/reload suites, all-core simulated failure and scheduling
tests, and normal/witness/qualification cross-builds pass. These checks do not
measure hardware speed; fleet-wide unicast traffic and host audits remain.
The worker stack envelope is `7632/8192` including a 1,024-byte helper
allowance, not a whole-program proof. Qualification driver SHA-256:
`1d527077af67276ca9024f8011f91f56e9fde8b7f053fe4268000cea041da82d`;
normal and witness driver SHA-256:
`dc31e3e98d3f0fd84e2f44baecb06534d7360f87a8439aaa6e6c2ce2d4dabb70` and
`7d0b349751e8324769fb71400eca5d23a24cd08b846201e5260c8c01fe64c121`.

Tag0153 removes two redundant pre-command full descriptor reads per worker.
The physical staging path already proves the initial descriptor word by word
or compares the whole old descriptor and verifies every changed warm word. A
new graph-session ledger records the exact descriptor only after those checks
and the marker write succeed, requires matching fleet-wide ticket/token and
all 120 staged cores, and permits each command once in order. The session lock,
sealed stream and restricted peer write ranges protect staged records until
publication; the post-run full descriptor audit is retained. By source count
this eliminates 30,720 32-bit MMIO reads per graph, while the ledger adds 61,440
bytes to the aligned, heap-owned graph object. Simulated tests cover stale,
forged, partial, duplicate and ordered publication across two runs. Target
and reload suites and all three Haiku cross-builds pass; there is no hardware
measurement. Qualification driver SHA-256:
`7d2319caa8c40a57913f567a13e1639b8a10f54ab2aa5ceac2bfd8793601f6b5`;
normal and witness driver SHA-256:
`f8a88cffab42bacab19c07c6aa0cf7084e0a2b85a771719dbe956fdd965c6dc0` and
`23b00c9419c017301e7eca6dea9a11218b553c9eb3bf266194a0bcac1992d146`.

After the tag0120 diagnostic exited cold, a single authorized power-off/on
cycle and the verified NVMe Fedora-to-Haiku handoff restored a fresh Haiku
boot. The saved hash-verified recovery script restored stable driver55:
tag `2026102100010012`, generation2, `quiesced0 cold0 active_owner0
open_count1 guard1`. The recovery lock is absent and no inference/dataflow
client is running. The 4B file remains `2740937888` bytes and the preserved
27B file remains `18973870528` bytes. No model diagnostic has run after this
recovery. Details are in
[`tag0124 recovery`](benchmarks/qwen35-dataflow-tag0124-20261122/RECOVERY.md).

Tag0120 failed the first core0 embedding read: NoC0 response counter index36
observed `0x00029066`, expected `0x00029067`; completion mask `0xfe`, busy
initiator mask `0x0`, and outstanding transaction-ID mask `0x00000001`. The
request was address `0x04185380`, route `0x312`, size `3392` bytes. It
produced no token; decode timing is unavailable.

Candidate tag0122 fixes a concrete resident-worker routing defect found by
comparing the Blackhole register definitions with Tenstorrent's NoC code.
Tag0120 used command-buffer `NOC_RET_ADDR_HI` at offset `0x14` as the local
coordinate. That register is mutable command state, not the core identity;
it could route a read response to another core. The worker and host audit now
use `NOC_CFG(NOC_ID_LOGICAL)` at `0x148`, and the NoC commands use marked,
static-VC1 read and nonposted write control fields. See Tenstorrent's
[Blackhole register definitions](https://github.com/tenstorrent/tt-metal/blob/main/tt_metal/hw/inc/internal/tt-1xx/blackhole/noc/noc_parameters.h)
and [NoC initialization and transfers](https://github.com/tenstorrent/tt-metal/blob/main/tt_metal/hw/inc/internal/tt-1xx/blackhole/noc_nonblocking_api.h).
This is a plausible cause of tag0120's missing response, not a confirmed
hardware diagnosis. A host fixture checks the stale-command-register case and
the read/write command fields; the RV32IMF role build and Haiku/RISC-V target
build pass. Tag0122 is not installed or hardware-tested. Its default driver
SHA-256 is `6a4c03b152b01ec623b078755992b8fc342a558af41567e2d8775a1e9b2c9957`;
the service text hash is
`6af17124c1859b34166617577dce72bbbafb06e094146a796546a2650db51089`.

Tag0123 strengthens that fix: the logical ID's upper 20 bits belong to
firmware and are ignored when decoding its coordinates; the NoC0 coordinate
must also match the selected worker before launch. Completion checks ignore
only documented hardware-mutated reserved bits in `NOC_CTRL`, while still
checking the request type, VC and all other software-controlled fields.
Without this, a successful transfer could be rejected after hardware changes
those bits. Tests cover all 120 worker-coordinate mappings, nonzero upper ID
bits, changed control fields, and allowed reserved-bit changes. The authoritative
[Blackhole NoC memory map](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/MemoryMap.md)
documents both behaviors. Tag0123 has not run on the card; in particular,
the NoC1 logical-route and destination-translation assumptions remain to be
verified. Its normal driver SHA-256 is
`5163d79689ce225100913bbc26b4bc8329e9de22642d5641808c805de839130f`.

Tag0124 adds a read-only NoC1 fleet audit before starting the dataflow service.
The previous route proof copied the NoC0 bank routes to NoC1 without checking
the second NIU's translation. The new audit samples each worker twice, checks
its NoC1 identity and idle state, and verifies all seven bank routes against
the independently established NoC0 geometry and the reversed 17-by-12 NoC
coordinate system. It refuses unsupported DDR translation modes and FIFO
state. The NoC1 aperture is read-only and is present only in the dedicated
dataflow build. Simulated tests cover all 120 workers, altered bank routes,
changing registers, busy traffic and invalid identities. The Haiku/RISC-V
target builds pass, but this audit has not been exercised on the Pioneer.
See Tenstorrent's [Blackhole coordinate documentation](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/Coordinates.md).
The tag0124 normal driver SHA-256 is
`3336b78873687f2c491314751e39e0f2942a0bcf0dfccd2d3f9bcdf4f4cc79ea`.

Tag0125 connects the disabled recovery prototype to a bounded ARC state0
exchange on firmware queue 0 after reset and telemetry checks. The transport
uses fresh mappings, validates queue ownership, and retains the mappings and
cold latch if publication or completion is uncertain. Simulated tests cover
pointer wrap, every dropped queue write, timeout, bad replies and partial
mapping; the existing 27B application-queue tests still pass. The state0
operation follows Tenstorrent's [Blackhole initialization](https://github.com/tenstorrent/tt-kmd/blob/main/blackhole.c)
and [firmware queue protocol](https://github.com/tenstorrent/tt-kmd/blob/main/msgqueue.c).
This is only one part of firmware reinitialization and has not run on the
card. Full PCI/DBI restoration, remaining firmware setup, resumable failure
handling and physical qualification remain open. The normal driver SHA-256 is
`fc736ae48530f37785f11ca2be56626077e549cae05e3a321db841ecd9caa366`;
the enabled prototype remains non-loadable and is used only to compile-check.

Tag0126 connects the DBI payload snapshot and restore to the non-loadable
recovery witness. The healthy-session adapter checks the fixed PCIe identity,
BARs, NoC route and status before mapping the control register read-only. The
post-reset path audits both NoCs across the held fleet and the pinned firmware
HDMA, then uses a fresh writable mapping and restores its fresh TLB snapshot.
The owner and mapping are retained on uncertainty. Fake-device tests cover
the real session adapter, mapping and recovery orchestration, including lost
writes, route changes, busy DMA and failed TLB restore. Normal and non-loadable
Haiku/RISC-V builds pass; the dataflow stack bound remains `15712/18432`.
The normal driver SHA-256 is
`51b8ccacdb2efaa1a7d65588040aa9e8a835fe265463d0eedb48c331660faad3`;
the non-loadable witness is
`43bd02e3763c49866f24ebbd9fbef9236ea0edb649d82c914f7981ec0975979f`.
This remains untested on Pioneer, is not complete PCI/firmware recovery, and
does not establish a new decode rate.

Tag0127 removes a repeated per-block 576-byte activation copy in the resident
projection worker. It passes the exact packed L1 cache chunk directly to the
existing Tensix code, with bounds tied to the format, input width and block.
ASan/UBSan tests check all bytes and chunk reuse for Q8_0, Q4_K, Q5_K and Q6_K;
the RV32IMF worker compile and normal and non-loadable Haiku builds pass. The
normal driver SHA-256 is
`f6c6ef24e5e41bbcd97a960cf85e49f458776655e1718af7eb1a4aedbeeee4e9` and
service text SHA-256 is
`529c47979f45426c2114241e7063b2e74fc03403a296cb0afd4a19fbe8492ff9`.
There is no physical correctness or throughput evidence for this change.

The host still performs a separate all-core dispatch lifecycle for each of
548 resident operations per generated token. This source-level count is not
a timing measurement. Moving the checked graph and synchronization onto the
device is the major outstanding architectural work toward 100 tokens/s.

Tag0128 starts that work with a device-side 120-worker barrier and restricted
NoC1 peer transport. Worker 0 continues to compute, then collects one receipt
per peer and distributes a release. Two ordered transfers separate each
payload from its commit marker; stale/torn markers, timeouts and partial sends
fail closed. A partial release can permit earlier peers to advance, so the
entire graph must be retained on uncertainty. The barrier's fixed L1 slots are
within each core's existing saved snapshot; clear/readback is included only in
the non-loadable device-dispatch witness. Simulated tests exercise all workers,
failure selection, stale/torn records, send failures, held-core setup and L1
restore. RV32IMF compilation of the real transport has no atomic instructions
or runtime helpers. Normal and non-loadable Haiku builds pass; their driver
SHA-256 values are `6ae463bb56390e8e1693c57bbdf9b11684b54742e74b379e598d322e1af8b4b7`
and `dfa3472ad3714c1276bc88e072185c9a3c8574278eb16dce41e466c89f6de1e1`.
The worker service does not yet invoke this barrier; queued descriptor fetch,
epoch ownership, timeout/fault integration and hardware qualification remain.

The queued rewrite now includes a 2 KiB per-core descriptor record, a
bounded NoC0 fetch into the idle second weight page, and a planner for 120
non-overlapping streams in the packed model workspace. Fetch checks record
identity, sequence, core, size and an accidental-corruption checksum; it
invalidates the old command before reading and publishes the new magic last.
Synthetic tests cover actual embedding/projection records, all stream leases,
workspace overlap/capacity, malformed records and read failure. The concrete
fetch compiles for RV32IMF with no runtime helper or A-extension instruction.
The checksum is not authorization: trusted kernel lease validation, typed
model-bound command authorization, immutable storage, dynamic token/position
handling, worker execution, and hardware tests remain to be implemented. A
full 548-record stream for all cores occupies 134,676,480 bytes; rebuilding it
for each token would create substantial transfer overhead, so fixed records
must be reused and only dynamic fields updated.

The primary worker has only 2,892 bytes of unused code space. A separate
offline linker witness places the existing 54,452-byte primary image at
`0x72000` and the 7,308-byte stream/barrier compile probes at `0xa0400`,
before the invocation slot at `0xaa000`; both fit and have no unresolved
imports. The entire range is covered by the existing L1 snapshot. Tag0129's
non-loadable witness now writes and reads back both segments while the fleet
is held, includes the second segment in image verification and accepted
program-counter ranges, and prohibits writes to it while cores are running.
Simulated tests exercise dropped writes, failed and corrupt readbacks and the
held/running access boundary. The second segment still contains compile probes,
not the actual queued service, and no tag0129 image has run on Pioneer. The
normal driver SHA-256 is
`8f7a25dc4b2cbddeaf8b6d691a9342386f7e8e5e4c9d565f7b3e766f4ca5eb31`;
the non-loadable witness SHA-256 is
`22f4d211d27e3ae0a49c6040a6a3b02871b55e94c98d2fc1997f85f8f6248072`.

Tag0130 extracts execution of one local command from the service loop for
reuse by the future queued worker. The existing service still supplies the
command and retains its publication, completion and acknowledgment protocol;
the new executor does not itself authorize queued commands or coordinate the
fleet. The RV32IMF image is 54,536 bytes, the synthetic role tests pass, and
both normal and non-loadable Haiku builds pass. Their driver SHA-256 values are
`66fc673b05a400cf090de5d621840c1ef73a42c68b1222b1d7c21c24d766d75d`
and `c9b51667a271c350701729e240af5f90af06d50ddcb7e0a7b8249635d15a27e8`.
This has not run on the card and does not establish a decode rate.

Tag0131 adds a per-worker control loop for stream fetch, exact-command
authority, execution and fleet barrier. Bounded polling and any uncertain
step retain the whole graph; local completion is not fleet completion. A
120-worker simulation uses real encoded embedding records, the stream fetch
and the barrier, with fake execution and authority. It covers successful
two-step execution, rejected authority, failed reads, uncertain execution,
reported worker failure, partial release, queue failure and deadlines. A
non-loadable RV32IMF witness links the loop with real NoC transport and the
worker executor, but its authorization hook rejects every command and the
service never calls it. Its primary and dispatch sections use 54,652 and
8,164 bytes respectively. Normal and witness builds pass; their driver
SHA-256 values are
`9046b4e81acb393102d79c6c4c1b533819cde9f1b0b2e579393fff7caa5e947c`
and `8d40af3f685cb64b584da5841cc8faa86cfb7c6f46b28d4b677f6b596e19d362`.
The kernel must still authorize and protect the exact stream, bind fresh
epochs, update dynamic fields efficiently, audit fleet completion and report
faults. Nothing here establishes correctness or speed on Pioneer.

Tag0132 adds a separate metadata-only stream recorder. It clones the checked
model authority into an isolated planning candidate, runs the existing full
decoder sequence, binds each program to all 120 cores, and emits encoded
records to a write/readback/seal interface. The candidate is speculative and
must not be promoted to live state without a full physical audit. A synthetic
full-shape model test produces 548 steps for two positions and covers all nine
invocation kinds, forged worker IDs, failed writes and failed begin/seal. It
uses fake storage and performs no inference. The test caught a real stream
format bug: RMS stores its core field at a different offset; validation now
uses each invocation's declared offset. A separate RV64 kernel-shaped planner
compile reports 3,488 bytes as its largest single frame, not a call-chain
bound. Storage ownership, sealing, dynamic updates, full stack audit and the
device authorization hook remain open; the recorder is not linked to either
driver. Normal and non-loadable tag0132 drivers build successfully, with
SHA-256 values
`b082707b8dafdfc3e2e76bd2b99349d7d02a6bd4a7180114ee54ff71448aa1b4`
and `3ffaf3a86e85d45aaa3a067f4d9f9ffc2a0d8e410c89b1e9ca2a65c9db60c313`.
No new hardware measurement exists.

Tag0133 adds a non-loadable private bank-page sink. It reconstructs workspace
bounds from the already-validated live manifest, validates the stream plan,
claims 120 disjoint uninitialized ranges in the bank owner, and buffers two
records per page. The existing page transfer writes and reads back each full
page; an odd final record is zero-padded. Generic bank access cannot enter a
claimed range, the owner cannot write it after sealing, and an uncertain
transfer quarantines the bank owner. Tests exercise the actual bank resolver
and transfer using simulated hardware, including partial/failed transfer,
overlap, occupied pages, incorrect ownership and sealing. The normal seven-bank
control tests and both Haiku builds pass. The adapter is compile-only and no
recorder is connected to it. Other mutation paths, ownership across the
entire graph, fresh epochs, efficient token updates, completion audit and
hardware qualification still need work. Normal and witness driver SHA-256:
`761d8c9458d7f8d80a2ccfbde53987129edbb76ff203f0d9ebc659cf08152108`
and `b717dfff0fee41c63f7fc6861e483a47ff0edadccd85e4c1fe157e832aabf1a4`.

Tag0134 composes the real model-bound recorder, workspace validation and
private page-transfer sink in the non-loadable driver witness. A synthetic
548-step/120-core test reaches the real bank/store implementation against
simulated hardware and checks first and last records on every core; prompt-only
planning produces 545 steps. Public bank/span and legacy uploads and old
Qwen35 dataflow actions are gated once the claim exists. The actual bank
control test covers denied outside-lease uploads. Other diagnostic and cleanup
paths still require an ownership audit. The combined build's conservative
recorder stack envelope is `17408/18432` bytes including a 2,048-byte
allowance, without a future ioctl caller; the witness's normal dataflow envelope
is `18160/18432`. There is no public graph entry, retained session lifecycle,
device authority, final fleet audit or physical qualification. The current
32,880 page writes per token also make fixed-record reuse essential. Normal
and non-loadable driver SHA-256 values are
`8966cc4906c61d36dfc8c76fec02c1917f034a190bbaffba0426d41b93717dc5`
and `a41ba66116553b520bc08ca37d152faad379e5687f9142b422eb582b6aae8514`.

Tag0135 prototypes fixed-record reuse. The worker fetches and checks a sealed
template, patches token/embedding, position and recurrent ping-pong fields in
local staging, then publishes the command. Synthetic full-shape tests compare
every core and operation against freshly generated authorized records for
positions 1 and 2 and an embedding-shard change; typed-builder comparisons
cover the last context position and last vocabulary token. Failed patches do
not publish command magic. The non-loadable RV32IMF worker witness links the
dynamic fetch but its graph authorization hook denies execution. Its primary
and dispatch code use 54,652 and 9,996 bytes.

The template epoch is now separate from per-run barrier epochs and command
sequence ranges. RMS and argmax receipts also require fresh sequences; an
unbound ticket allocator prototype issues non-overlapping ranges and rejects
exhaustion. Neither tickets nor the trusted dynamic run context are connected
to an owned live session, and fleet completion, failure retention, clean
release, mutation-path audit and hardware qualification remain incomplete.
The normal and non-loadable tag0135 driver SHA-256 values are
`90ef29e142c91a321b32d48dd2a99ae464345969ccb5c1bc3f2dce9dbe752d67`
and `93fd0da9854f76674f12980c0d26288458e1d8701dea0057daf3257e724ffe27`.

Tag0136 adds a 512-byte graph-run descriptor in each worker's saved L1. A
non-loadable resident worker recognizes the graph marker and enters the
graph loop with a checked local descriptor copy. The host publisher checks
all workers, stages and reads back all 120 descriptors and markers, then
issues triggers; partial publication is retained as a graph fault. Simulated
tests cover malformed descriptors, failed readback, partial triggers and
idle-only descriptor writes. The graph authorization hook still denies all
commands and no live session invokes the publisher. The worker's conservative
stack envelope is `7824/8192` bytes including a 1,024-byte helper allowance;
it is not a whole-program proof. The recorder envelope is `17424/18432` with
a 2,048-byte allowance, excluding a real ioctl caller. Run ownership, trusted
ticket/context binding, completion/failure audits, cleanup, the remaining
mutation paths and hardware qualification remain open. Normal and non-loadable
tag0136 driver SHA-256 values are
`7e89661ffae9792945a07b4583a2de876e588e0653ec084d2e1666d3e15e2951`
and `7984c3bd895bf9341289537ed2d1f6128a73a1ad479e55b74df860fca64fcf03`.

Tag0137 fixes a repeated-run readiness check: a fully acknowledged worker may
have a nonzero command sequence, so requiring the startup mailbox's zero
sequences rejected a ready worker after its first command. The new check
requires an idle mailbox matching a stable ready snapshot, and rejects stale
snapshots, shutdown and unexpected reserved fields. This is a code-level fix;
it does not establish the cause of tag0120's embedding failure.

A non-loadable graph completion prototype now waits for all 120 terminal
workers, audits their descriptors, final commands, runtime and NoC state, and
checks a stable argmax for decode before acknowledging any worker. It retains
the graph on incomplete, inconsistent or partially acknowledged results.
Simulated decode and prompt tests cover success and fault cases. The actual
driver's ownership gate still rejects every graph, and completion does not
commit model state. A real session must establish immutable model and ticket
ownership, connect authorization and completion, commit model state exactly
once, and implement safe cleanup before hardware use. The normal and
non-loadable Haiku/RISC-V builds and the resident-bank regressions pass. Their
driver SHA-256 values are
`d11e3806d24913a1b248d612b0b06dd78413b3f324009a342464c1b83f31c9ea`
and `4d5bddfc3b5f5e576c00ee585a24c31acfbd8af67da0af184271d34cdca52dbc`.
The non-loadable build's reported conservative kernel stack is `18160/18432`;
the existing recorder and worker envelopes still exclude a real caller and
whole-program proof respectively. This work has not run on the card and
provides no throughput measurement.

The next offline component replays the graph's authority transitions into a
private candidate without regenerating the 120 per-core records. Simulated
prompt and decode tests compare the candidate with the full recorder for two
successive positions and verify that the original authority remains unchanged.
The standalone Haiku/RISC-V kernel compile passes; its largest individual
frame is 3,536 bytes, not a call-chain stack bound. This component is not
connected to the driver or allowed to promote state, and does not prove device
execution or measure decode speed.

Tag0138 makes one sealed 548-record decode template usable for prompt tokens:
the descriptor carries a 545-step prompt ticket or a 548-step decode ticket,
and each device worker executes exactly that many records. This avoids a
second protected stream claim and upload. Full-shape simulated recording
compares all 120 cores' first 545 records against independently recorded
prompt programs at positions zero and one, including a dynamic token patch;
the fleet simulator also checks prefix termination and count bounds. Before
staging any worker, publication now requires the ownership callback to bind
the sealed stream, model, token, ticket and fixed activation slot. The real
callback still denies all publication; recording that fixed slot in the live
session and establishing the immutable ownership proof remain necessary.

The target test suite and normal and non-loadable Haiku/RISC-V builds pass.
The normal and non-loadable driver SHA-256 values are
`5996e2b386eea9c8df5660830186458cc2ab98e17e57ed6f76468126b03640c6`
and `f56bd0561a08005dd5e80a77bb98fc0ffe3dcc812879f4af6c316aa431f71ae6`.
The non-loadable kernel stack estimate is `18160/18432`, and the worker
envelope remains `7824/8192` with a 1,024-byte allowance, not a whole-program
proof. This change is not hardware-qualified and gives no throughput result.

Tag0139 adds a read-only sealed-stream identity proof. The recording witness
retains its template slot and audited routes; the sink compares the complete
plan against its recorded copy, and the bank ledger checks the exact owner and
120 claimed ranges, sealing, current baseline and quarantine state. Simulated
tests reject altered ranges, changed stream bindings, a different owner, a
drifting baseline and quarantine. This proves only stream identity: the
future session must separately prove service safety, model identity, fresh
ticket ownership and model-state commit. In particular, the existing general
`Unsafe()` check treats an installed service as unsafe and cannot by itself
authorize a resident graph. The publication callback still denies all runs.
The target suite and 27B shared-bank regression pass, as do normal and
non-loadable Haiku/RISC-V builds. Driver SHA-256 values:
`305bed311286cb667932385e755129f7dfe08be450779d08a11d20fc71d21236`
and `55d17dccbe446ac1adea65ff7738933b5dc241463266552a3ddb7df385db841c`.
No card run or speed measurement has been made.

Tag0140 adds a graph phase to the resident service in the non-loadable
dispatch build. It reserves the next worker-mailbox trigger, binds one
publisher, blocks normal work and close while the graph is in progress, and
adopts the new service sequence only after that publisher's completion has
been audited and the model-state commit callback succeeds. An inert preflight
may cancel with the same publisher; staged or uncertain graphs retain a fault
and cannot use the old projection-recovery or close paths. The simulated test
exercises consecutive prompt/decode graphs, duplicate commit, wrong-publisher
cancellation, an unaudited result, partial staging and commit failure. The
real driver commit callback remains deny-all: there is no live candidate
promotion or complete graph-fault recovery yet. The service owner must retain
the publisher and all graph resources through the fault.

The graph lifecycle test, full target test suite and normal/non-loadable
Haiku/RISC-V builds pass. Normal and non-loadable driver SHA-256 values:
`e90017ffe37b80e4e8f3ffa20318aaa5c7a929cf114010958b069f7eabb305cc`
and `df9177d08b62e3989ba3b23edf41db799e3b8a4fbf92e9cb15802cc54b635788`.
The graph worker and recorder stack envelopes retain the previously documented
limitations. This is simulation and compilation only, with no decode result.

Tag0141 adds a session-lock-scoped model transaction for the queued graph.
It builds a private candidate from the current model, binds the exact token,
ticket, activation slot and publisher, and captures compact snapshots of all
mutable metadata for both original and candidate. Promotion swaps the live
authority pointer exactly once only after the matching publisher is audited
and both snapshots still match. The retired authority stays owned by the
session until references are ruled out. Static layout and routes are immutable
after binding and still require the separate sealed-stream/session proof.
The simulated service test connects this transaction through a prompt and a
decode step; other tests reject forged token layout, changed live or candidate
state, wrong publisher, missing audit and repeated commit. No tensor work is
performed by this host metadata transaction.

The real driver's publication and commit callbacks still deny execution: the
transaction, its candidate, ticket issuer and sealed recording are not yet
owned by an actual driver session, and device authorization and graph-fault
cleanup remain open. The target suite and normal/non-loadable Haiku builds
pass. Normal and non-loadable driver SHA-256 values:
`9207eb940020793ef91e0510f7fdb8051cde16aa5854003d3f2ca3d6ddd06eb1`
and `a2abbd831a7841ed3325e4af7f289d29e845c1ff020314c59e353a7d0cd6656c`.
The new model-planning stack envelope is `9472/18432`, including a 2,048-byte
helper allowance but excluding a real ioctl caller. No hardware or speed
result exists for this change.

Tag0142 composes the sealed stream, fresh ticket issuer, resident service,
publisher, all-core completion audit and model transaction into a synchronous
graph-session coordinator. One recording can be claimed by only one session,
preventing a second issuer from reusing its barrier epochs. The coordinator
returns output only after the state commit; uncertain publication or audit
retains the run, candidate and service fault. Simulated tests exercise two
successive prompt/decode runs, duplicate stream claims, failed preflight and
an audit fault. A non-loadable driver witness now connects this coordinator to
the real recording's bank proof and the PCI transport. Its controlled context
safety check permits only this ready/running dataflow service while preserving
the other `Unsafe()` checks; it also verifies the runtime owner, cold/reload
state and seven-bank model session. This code has not run on the card.

There is still no actual session allocation, public graph request or retained
object teardown, and the device's per-command authorization hook denies all
commands. Those must be implemented before a hardware trial. The target suite,
27B shared-bank regression and both Haiku builds pass. Normal and non-loadable
driver SHA-256 values:
`60f47279614f341405f471e35dcdf578d7cf0d16124676ab2eebc919447378cd`
and `d5d1959ceadfa18e2ca0577747334dd34ea2fd8eeae04e5c7057a962e4d9ca66`.
The coordinator stack envelope is `13552/18432`, including a 2,048-byte helper
allowance but excluding the future ioctl caller. No decode rate is available.

Tag0143 replaces the non-loadable worker's unconditional graph-command denial
with a local fetched-command consistency check. After the descriptor has been
validated and the sealed record fetched and patched, it checks the record's
core, step, structure and checksum, then compares the staged invocation with
the published L1 program before calling the typed Tensix worker. The check is
not model authorization: that depends on the host's validated recording,
disjoint sealed bank claim, private descriptor and session ownership. There is
still no live session allocation, public graph request, or fault-safe graph
teardown, and no card run has established the memory/route assumptions. The
dispatcher has moved from the test directory into the rewrite sources.

Simulated tests cover corrupt headers and payloads, altered program words,
wrong core/step, prompt bounds and all nine invocation families from a
dynamically patched recording. The target suite and both Haiku builds pass.
Normal and non-loadable driver SHA-256 values:
`ae683b12983e8fd759a0231f6216b6f85342fe72220dd76ede07352ea243f3d4`
and `48e65c1b0c8a52e92be259c8c997b3b85f01069174fe78d7d08a9f8737b43eb5`.
The worker witness has a `7792/8192` stack envelope including a 1,024-byte
helper allowance; it is not a whole-program proof. These checks add work per
command and have not been benchmarked.

Tag0144 adds a context-owned graph lifetime in the non-loadable driver. It
retains the recording and session, allocates each run and candidate, transfers
the candidate into the live model only on success, and retains uncertain runs
instead of freeing their resources. The context's ordinary unsafe check now
also catches that retained graph. Normal successful runs free their run object
and retired authority under the session lock. A simulated allocator/runner
test covers allocation failure, two promotions, retained fault and release
only after a simulated restoration proof. The real graph-restoration proof
still denies release, and graph-fault recovery without reboot is not complete.

The kernel lacks C++17 aligned delete, so 64-byte-aligned graph objects now use
a checked placement allocation with matching destruction; the ordinary model
authority keeps its existing new/delete convention. The allocator test, target
suite, shared 27B bank regression and Haiku builds pass. Normal and non-loadable
driver SHA-256 values:
`d35020cfd9200901d585f9f163e44a1d4ea3536273f301f931178819a966d2d6`
and `f56e29a6a6ddb5e29b580a247a2c95fec5f343f7207d4d5ec201f5e320eafa00`.
The recording and graph-run stack envelopes are `17552/18432` and
`13744/18432`, respectively, including 2,048-byte helper allowances and
excluding a real ioctl caller. Recording, binding and running this owner are
still compile witnesses, not a public generation path or hardware result.

Tag0145 connects the context-owned graph to the existing install and generation
requests in the non-loadable dispatch build. Install records a sealed decode
template for activation slot 0, starts the resident service and binds the
graph session. Generation uses only the graph runner: prompt tokens use the
545-step prefix, the last prompt token and decode tokens use 548 steps, and
there is no host tensor inference fallback. Prompt time includes the first
selected token; decode time covers the remaining output tokens, matching the
client's `(output_count-1)/decode_seconds` calculation. Recording is included
in model-load time. Requests for another activation slot are rejected.

Simulation covers token ordering, EOS, invalid output, separate timing, two
warm requests through the owned runner and retained audit failure. The target
suite and normal/non-loadable Haiku builds pass. Normal and non-loadable driver
SHA-256 values:
`813d44299bb77faeb02bc3d45f805afbe4d499fdd9456f87aeec5bfa22557582`
and `6d4d5c72f94fc90187df7889b4cf6022ef2dcc7b0632af991e42e746e1c149d7`.
The measured install and generation call-path envelopes are `16400/18432`
and `15872/18432`, respectively, each including a 2,048-byte helper allowance;
neither is a whole-program stack proof. The driver remains non-loadable pending
fault recovery, teardown review and physical qualification. No card run or
decode throughput exists for this path.

Tag0146 connects retained graph heap objects to the existing non-loadable
ASIC-reset recovery prototype. Their release is allowed only after the same
locked recovery ticket has proved reset, firmware readiness, all 120 cores and
NoCs quiet, host DMA idle, the temporary mapping closed and the old owner's
mapping disposal completed. The release is CPU-only and occurs before PCI
unreserve; a failed proof or release keeps the cold latch. Mapping disposal
now records completion separately from an attempted or partial disposal.
Simulated recovery tests exercise the graph-release ordering and refusal, and
the existing failure-injection suite keeps the owner/cold latch on earlier
failures. This is not yet an in-place recovery guarantee: the ASIC-reset and
firmware sequence is non-loadable and unqualified on Pioneer, ordinary graph
faults still lack a bounded no-reset teardown, and no hardware trial occurred.

The recovery tests, target suite and both Haiku builds pass. Normal and
non-loadable driver SHA-256 values:
`44817035ee8ff1728fbf5052c1711c288858727cbbd2aee9dbc70058fd8db93f`
and `48ed831cec7bf16e01ac42d9965d1539683e81c7d678307b8b8ee7f6aec0b3d3`.
The graph generation call-path envelope is `15920/18432` with a 2,048-byte
helper allowance, not a whole-program proof. No decode rate is available.

Offline recovery-guard work now defines a 48-byte proof requiring ASIC reset,
post-reset firmware readiness, restored PCI state and mappings, all 120 cores
held, idle NoC and host DMA, and released retained owners. The resident guard
requires a ticket tied to the fault epoch, so a concurrent fault denies
recovery. The extracted tests pass, as does the full Haiku/RISC-V target
build (`15712/18432` kernel-stack audit); recovery guard hash after its atomic
fault-state and module-path fixes:
`8b29c57ac8b47f1c6218dcdf31733cd736f63727ad0527daae3cd12e4fc906aa`.
An isolated Blackhole reset helper now encodes the exact three PCI config
writes used by Tenstorrent's KMD, rejects bus mastering or a stale reset marker,
and waits a bounded number of times for the marker to clear. Its fake-PCI
ASan/UBSan test passes with the reload-guard fixture. A prototype ioctl
connects it to the recovery sequence but is disabled by default. Complete
PCIe capability restoration, remaining firmware initialization, physical recovery,
and silicon validation remain open. A compiled PCI restoration helper
reestablishes the fixed active BAR and bridge layout after the reset marker
clears, while refusing bus mastering, an uncleared marker, unsafe interrupt
state, and unexpected identities. Simulated PCI tests exercise the real BAR
plan, dropped writes, and decoder ordering. It does not by itself restore all
PCI capabilities or establish firmware readiness. The currently loaded legacy guard
is `B_KEEP_LOADED` and cannot be upgraded in place. The completed clean boot
still loaded that legacy guard; after staging the new guard, another fresh
Haiku kernel boot through the NVMe/kexec path will be required to activate it.

The post-reset mapping now polls ARC scratch readiness with a bounded wait,
reads the fresh firmware telemetry table, and requires the pinned firmware
bundle and harvesting values before the fleet audit. Simulated tests cover a
delayed boot, timeout, ARC error, wrong firmware, interrupted wait, and TLB
restore failure. This confirms only the selected scratch and telemetry
contract; the queue state0 exchange is now wired to the disabled prototype,
while the complete recovery lifecycle still needs hardware qualification.

The post-reset audit checks both NoC banks on all 120 cores twice, with
explicit read failures and busy/outstanding transaction checks. A dedicated
mapping implementation validates PCI BARs, captures the TLB only after reset,
checks the fresh topology and each selected core's identity, and restores the
post-reset TLB value before releasing mappings. Simulated-MMIO fixtures cover
both networks, PCI/BAR rejection, partial map failure, wrong core identity,
and retained mappings on failed cleanup. The implementation is wired into the
default-disabled prototype but has not been exercised on the card. A single atomic
word publishes the guard's fault epoch and cold latch together; an injected
fault during latch clearing leaves the previous generation recoverable. These
checks do not qualify a physical reset.

The post-reset mapping backend now also checks both PCIe endpoints for pending
transactions and errors, and checks the pinned firmware's read/write HDMA
channel 0 enables and states in three stable samples. It requires the fleet
audit first and restores a TLB value captured after reset. Simulated MMIO tests
cover busy DMA, a changing DMA state, pending transactions at either PCIe end,
and restoration. This evidence is limited to the pinned channel-0 firmware
contract; the prototype connects it to owner release but remains disabled by
default and has not been tested on silicon.

The failed owner retains its cookie, PCI reservation, snapshots and
quarantined host buffers after the client exits. The recovery operation must
distinguish a final-freed owner from a still-live file descriptor and revoke
old generation access before retiring those resources. Host buffers must stay
wired until the reset, PCI pending-transaction and host-DMA idle proofs pass.
The PCI, mapping, DMA audit, guard, firmware and ownership steps now have a
single prototype recovery operation. The default driver refuses that
operation before any card access: complete PCIe capability restoration and
remaining firmware initialization still need implementation and the sequence
has not been hardware-qualified. An extracted test runs the real
operation body against simulated hardware and injects a failure at every stage;
a separate test verifies the default gate performs no hardware action. A
separate compile-only build includes the prototype but cannot load as a driver.
Failed recovery attempts remain cold and are not yet resumable.

The prototype now captures the endpoint PCIe maximum payload and read-request
settings twice before reset and restores those configuration-space fields
afterward with bus mastering disabled. Simulated tests cover malformed or
cyclic capability lists, invalid sizes, dropped writes, and recovery refusal.
This does not yet restore the device-internal DBI payload shadow or the rest
of PCI capability state. Tenstorrent's Blackhole driver saves and restores its
internal payload value and reinitializes hardware after reset; see its
[Blackhole implementation](https://github.com/tenstorrent/tt-kmd/blob/main/blackhole.c)
and [reset path](https://github.com/tenstorrent/tt-kmd/blob/main/chardev.c).
The new path is included only in the disabled prototype and is not qualified
on the card.

Dedicated candidate tag0121 now records the retained owner's final file
descriptor free while holding the driver lock. A read-only status ioctl
reports the owner-lifetime prerequisites only when the exact driver tag and
generation match; it refuses the lifetime condition with extra open
descriptors, a live owner, or a quiesced driver. Extracted real close/status
fixtures and the target build pass. This status does not initiate a reset,
release any owner or host buffer, or grant hardware recovery. Tag0121 is not
installed; its historical default driver SHA-256 is
`35b04d1f2b2f4654e615a6b343bda8dcbb443c4455eb6178f42c6e73cb29906a`
and its measured stack bound is `15712/18432` bytes. Stable55 is now loaded
and idle after the authorized cold recovery.

The recovery guard export now uses a name beneath the existing reload-guard
image path. Haiku's module resolver stops at the first regular file, so the
previous distinct path would not have found the shared pinned image. The
module fixture verifies both names resolve through the same image path.

A post-audit PCI teardown restores the original fixed idle BAR and bridge
state without calling the pre-reset mapping cleanup, which could restore stale
TLB contents. Simulated PCI tests exercise invalid snapshots and dropped
writes. The teardown is connected to the default-disabled recovery prototype
and is not qualified on the board.

Tag0121 also contains an in-kernel retirement method for the dedicated
dataflow session's mapped areas and host buffers. It requires the reset,
firmware, PCI, core, NoC and DMA evidence with exactly one retained owner,
rejects duplicate area IDs, and releases host buffers only after other mapped
areas. On a deletion failure it retains the remaining areas and rejects a
repeat attempt. Simulated OS tests cover invalid proof, duplicate IDs, release
order and partial failure. The prototype ioctl calls it only after its other
checks; the ioctl is disabled by default and has not run on the Pioneer. No
retained owner or buffer has yet been freed by it.

No valid output, two warm runs, clean unload, or decode-rate measurement has
been established; 100 tokens/s remains unmet. The two-warm benchmark and
assistant integration stay gated until the rewrite and physical recovery path
are complete. See
[`tag0117 RUN`](benchmarks/qwen35-dataflow-tag0117-20261121/RUN.md),
[`tag0118 BUILD`](benchmarks/qwen35-dataflow-tag0118-20261121/BUILD.md),
[`tag0118 RUN`](benchmarks/qwen35-dataflow-tag0118-20261121/RUN.md),
[`tag0119 BUILD`](benchmarks/qwen35-dataflow-tag0119-20261121/BUILD.md),
[`tag0119 RUN`](benchmarks/qwen35-dataflow-tag0119-20261121/RUN.md), and
[`tag0120 BUILD`](benchmarks/qwen35-dataflow-tag0120-20261121/BUILD.md) /
[`RUN`](benchmarks/qwen35-dataflow-tag0120-20261121/RUN.md).
Tag0109 passed guarded preflight and uploaded all
`2,740,498,432` resident bytes. Authentication took `100.467732` s, prompt
tokenization `13.243073` s, and model load/upload `334.616263` s. Generation
ioctl `0xAF56` returned `EINVAL`; the client diagnostic printed the request
fields, but no prompt processing, valid output, decode timing, or tokens/s was
produced. Offline inspection found the cause: `ProbeControl` routed Qwen35
operations only through `RESIDUAL`, omitting the FFN-through-generation ioctl
range. The generation command never reached its validation handler. Unload
also failed at stage9/core0, resume phase3, with resume detail2
(`DataflowServiceResumeIdleProofFailed`). The client exited; the guarded
wrapper retained the exclusive lock and stable-driver backup while the
candidate reported generation3 `cold1 active_owner1`. The guarded recovery
helper restored stable driver55 at generation2; the lock is absent, and
postflight confirmed both model sizes, 64 CPUs, and full RAM. Run details and
raw evidence are in
[`benchmarks/qwen35-dataflow-tag0109-20261121/RUN.md`](benchmarks/qwen35-dataflow-tag0109-20261121/RUN.md).

Tag0110 routes the complete ioctl range, adds compile-time range checks,
reports returned request fields and precise restore subchecks, skips the
running-idle proof for the fully restored reset-held root, and retries
recoverable close failures while the owner retains its snapshots. Its full
offline variant-60 build passes; owner and PCI-memory ASan/UBSan fixtures and
the RV32IMF compile pass. The guarded hardware run uploaded the full model,
then failed at the first prompt token in the embedding operation
(`position0`, `DataflowDecoderEmbeddingRunFailed`). No valid output or decode
rate was produced; close failed the core0 stop audit. Recovery restored stable
driver55 generation2 on Haiku, cleared the lock, and confirmed both model
sizes intact. Continue the rewrite offline before another benchmark. Run and
recovery evidence are in
[`benchmarks/qwen35-dataflow-tag0110-20261121/RUN.md`](benchmarks/qwen35-dataflow-tag0110-20261121/RUN.md).
Tag0110 build details are in
[`benchmarks/qwen35-dataflow-tag0110-20261121/BUILD.md`](benchmarks/qwen35-dataflow-tag0110-20261121/BUILD.md).

Tag0111 added worker-level embedding failure receipts to QUERY/RECOVER,
abortable pending embeddings after the all-core audit, and same-owner recovery
before service close. It built and passed its focused offline tests, but its
guarded hardware trial panicked in generation. Offline stack-usage evidence
showed a 194800-byte FFN frame and a 75808-byte projection-builder frame,
among several others larger than the whole 20 KiB kernel stack. The card was
recovered to stable55 generation2 idle and the 4B/27B model files remain
intact. The trial had no valid output or decode-rate result; see
[`benchmarks/qwen35-dataflow-tag0111-20261121/RUN.md`](benchmarks/qwen35-dataflow-tag0111-20261121/RUN.md).

Tag0112 reuses a heap-owned projection descriptor across the layer sequences,
keeps the 120-core schedule in the heap-owned authority, and builds directly
into the supplied descriptor without a large temporary copy. The offline
variant-60 driver/client/service build passes. The builder checks the measured
kernel stack chain and rejects any Qwen35 frame over 8192 bytes; the
conservative chain is 15696 bytes against an 18432-byte budget. Seven focused
ASan/UBSan tests pass, and the RV32IMF compile has no A atomics or unresolved
runtime helpers. Tag0112's hardware run uploaded the model, then failed during
the first prompt embedding: core0 reported worker state4, reader failure1,
phase5. The lower-level cause was not exposed by that receipt. QUERY/RECOVER
returned pending and service close failed its core0 stop audit; a cold recovery
restored stable55 generation2 and preserved both model files. There is no valid
output or decode rate. Build details are in
[`benchmarks/qwen35-dataflow-tag0112-20261121/BUILD.md`](benchmarks/qwen35-dataflow-tag0112-20261121/BUILD.md); run details are in
[`benchmarks/qwen35-dataflow-tag0112-20261121/RUN.md`](benchmarks/qwen35-dataflow-tag0112-20261121/RUN.md).

Tag0113 added worker-level embedding failure codes and recovery audit
stage/core reporting while preserving the fixed worker mailbox and ioctl
request sizes. It reproduced the first prompt embedding failure on core0:
failure7 (`EmbeddingWeightRead`), phase5. The all-core runtime audit failed at
stage6/core0, QUERY/RECOVER remained pending, and CLOSE failed at the core0
stop audit. The guarded wrapper retained the lock and backup when the driver
reported `cold1 active_owner1`. Its run record has the distinct authentication,
vocabulary, model-load, and failed-prompt timings; decode was not reached.
See [`tag0113 RUN`](benchmarks/qwen35-dataflow-tag0113-20261121/RUN.md).

Tag0114 is the hardware-tested recovery-diagnostic checkpoint. It preserved
the fixed 64-byte mailbox and ioctl request size, and added the subcheck
detail. Its failed card run and timings are in
[`benchmarks/qwen35-dataflow-tag0114-20261121/RUN.md`](benchmarks/qwen35-dataflow-tag0114-20261121/RUN.md).
Tag0115 adds the bounded read-only NoC drain wait for failure7 before
repeating the strict audit. The full variant-60 build, focused recovery
ASan/UBSan test, RV32IMF compile, and `15456/18432` stack bound pass. Its
hardware result identified a persistently busy NoC0 queue0 status; see
[`tag0115 RUN`](benchmarks/qwen35-dataflow-tag0115-20261121/RUN.md). Driver SHA-256 is
`52a4a16ceb170767089edac960925aac0fa549160f47cfd949ba657f25e21885`; client
SHA-256 is `80d28f4d75a48612773b07339010308ced09e8c8debf8e7708c35b85f7908da5`.
The card is currently stable55 generation2 after the guarded post-cycle
restore. See [`benchmarks/qwen35-dataflow-tag0115-20261121/BUILD.md`](benchmarks/qwen35-dataflow-tag0115-20261121/BUILD.md).

Tag0116 records the exact NoC transfer stage, failing index, and raw status
value for an embedding read failure using two existing mailbox reserved words.
It keeps mailbox and ioctl sizes unchanged and retains the strict recovery
audit. The full build, RV32IMF compile, and ASan/UBSan fixtures pass; the
conservative stack bound is `15712/18432`. Driver SHA-256 is
`2acf08ecf6578adcd3529c65259489eb92b4ceca5ffdcb78f6e75f1f0034e710` and
client SHA-256 is `52097a6efb88e89181b96a7476a291fb8afb1d7c828cc9bbf03c1c401c857106`.
It has not been installed on the card. Details are in
[`benchmarks/qwen35-dataflow-tag0116-20261121/BUILD.md`](benchmarks/qwen35-dataflow-tag0116-20261121/BUILD.md).

Tag0117 adds a guarded one-run client option that requests two tokens and
skips the second generation. This lets the rewrite validate a full prompt and
one decode step without running the throughput test. The variant-60 build,
target compile, sanitizer fixtures, and wrapper syntax checks pass. Driver
SHA-256 is `e5387ede717a52354f64fe8cb41ca73bf9a9cc1bc5ab86de0155dca6ec59283c`;
client SHA-256 is `7faaabe33f14399c717b91efc1f690c7defbb7a1d3b74f981d91ced47510276f`.
The one-run card diagnostic failed in the first prompt embedding. It preserved
the expected completion counter (`0x00029067`) beside the observed value
(`0x00029066`) and was recovered to stable55 generation2. See
[`benchmarks/qwen35-dataflow-tag0117-20261121/BUILD.md`](benchmarks/qwen35-dataflow-tag0117-20261121/BUILD.md).

## Progress

- 2026-11-21 (tag0115, bounded read recovery): Added a wait that reads only
  the four resident NoC initiator command registers and 32 queue-status
  registers, and only for worker failure7 (`EmbeddingWeightRead`). A stuck
  status returns a register-index detail without modifying hardware state; a
  quiet status still must pass the existing runtime audit. The fixed mailbox
  and request sizes are unchanged. The recovery ASan/UBSan test, RV32IMF
  compile, complete build, and `15456/18432` conservative stack audit pass.
  Driver SHA-256 is
  `52a4a16ceb170767089edac960925aac0fa549160f47cfd949ba657f25e21885`.
  Hardware trial timed out on NoC0 queue0 and required guarded post-cycle
  restoration to stable55 generation2. Details are in
  `benchmarks/qwen35-dataflow-tag0115-20261121/BUILD.md`.

- 2026-11-21 (tag0115, hardware result): Authentication took `99.897654` s,
  vocabulary/tokenization `13.042067` s, model load `330.400460` s, and the
  failed prompt operation `0.611537` s. Core0 reported embedding weight read
  failure7; the bounded recovery scan timed out on NoC0 queue0 (index4/detail
  `260`), with no output or decode time. The wrapper retained the cold
  candidate, stable backup, and lock. The guarded post-cycle helper restored
  stable55 generation2 and cleared the lock. See
  `benchmarks/qwen35-dataflow-tag0115-20261121/RUN.md`.

- 2026-11-21 (tag0116, failure-detail rewrite): Added exact NoC transfer
  stages and captured register values to the existing failed-embedding receipt.
  The host packs stage/index into its existing recovery detail and reports the
  raw value from the existing request result word. The mailbox remains 64
  bytes, the ioctl request size is unchanged, and the recovery path still only
  waits and audits; it does not clear commands or reset cores. Full build,
  target role compile, and recovery/embedding/owner/service-owner ASan/UBSan
  fixtures pass. Stack use is `15712/18432`. The candidate remains offline;
  Haiku is stable55 generation2 and idle. See
  `benchmarks/qwen35-dataflow-tag0116-20261121/BUILD.md`.

- 2026-11-21 (tag0117, single-run diagnostic): Added a client mode that
  validates one full prompt and a two-token output, skipping the second run.
  It loaded the full model and failed the first embedding NoC read at
  counter0/index36 (`observed=0x00029066`). No token or decode rate was
  produced. Recovery restored stable55 generation2 after the accidental
  power reset; the lock was cleared and both model files remain intact. See
  `benchmarks/qwen35-dataflow-tag0117-20261121/RUN.md`.

- 2026-11-21 (tag0118, expected-counter diagnostic): Captures the expected
  completion counter beside the observed value, carrying it through mailbox
  reserved word3 and a read-only `QUERY_EXPECTED` action. Fixed mailbox and
  ioctl sizes remain unchanged; dataflow ABI is 4. Full build, focused
  sanitizer tests, RV32IMF role compile, and `15712/18432` stack audit pass.
  The guarded diagnostic failed with observed counter0 `0x00029066` versus
  expected `0x00029067`; recovery restored stable55 generation2 and cleared
  the lock. See `benchmarks/qwen35-dataflow-tag0118-20261121/BUILD.md` and
  `benchmarks/qwen35-dataflow-tag0118-20261121/RUN.md`.

- 2026-11-21 (tag0118, hardware result and recovery): Authentication took
  `68.186682` s, prompt tokenization `13.766015` s, full model load
  `334.004328` s, and failed prompt processing `0.615329` s. The first core0
  embedding NoC read timed out with counter0/index36 observed at `0x00029066`
  and expected `0x00029067`; output and decode time are zero. QUERY/RECOVER
  reported audit stage6/core0/detail1508 and CLOSE failed the core0 stop
  audit. The cold-latched candidate was quiesced after a fresh Haiku boot;
  guarded recovery restored stable55 generation2, removed the lock, and
  verified both model files. No valid output exists. Details and raw evidence
  are in `benchmarks/qwen35-dataflow-tag0118-20261121/RUN.md`.

- 2026-11-21 (tag0119, read-only NoC failure receipt): Captures an 8-slot
  completion-counter match mask, four-initiator busy mask, and 32-slot
  outstanding transaction-ID mask in the fixed 64-byte mailbox; a read-only
  `QUERY_NOC` returns it through the existing 36-byte ioctl request. Also corrects receipt cleanup to clear
  only the six reserved words, preserving `logicalCore`. Full variant-60
  driver/service/client build passes at `15712/18432` stack bytes. Service
  owner, owner, embedding-worker ASan/UBSan, and runtime-recovery tests pass.
  The one-run diagnostic is recorded in its run file: the response counter
  timed out and the candidate cold-latched. Guarded recovery remains pending
  because the Pioneer has not answered SSH since the reboot. See
  `benchmarks/qwen35-dataflow-tag0119-20261121/BUILD.md`.

- 2026-11-21 (tag0114, hardware result): Authentication took `99.340053` s,
  vocabulary load and prompt tokenization `13.168584` s, full model load
  `329.758232` s, and the failed prompt operation `0.609034` s. Core0 failed
  embedding weight read (failure7, phase5); no valid output or decode rate was
  produced. QUERY/RECOVER stopped at stage6/detail11 (resident NoC idle), and
  CLOSE failed the core0 stop audit. The accidental power reset left the
  Haiku candidate at generation3 `cold1 active_owner1`. The authorized host
  reboot reached pinned Linux for the SD-free recovery handoff. See
  `benchmarks/qwen35-dataflow-tag0114-20261121/RUN.md`.

- 2026-11-21 (tag0114, recovery subcheck diagnostic): Split the all-core
  runtime-stability predicate into named failure details and returned the
  failing subcheck in unused low bits of the existing owner diagnostic. The
  fixed 64-byte worker mailbox and ioctl request size did not change, and no
  safety predicate was relaxed. Focused ASan/UBSan owner and service-owner
  tests pass, as does the RV32IMF compile. The full variant-60 driver/client/
  service build passes at 15456/18432 conservative stack bytes. Driver
  SHA-256 is `ea4fa51d250c143bd5144156aa6aba71f43e11c9f1c89ece41280ba7a08c3964`;
  client SHA-256 is `80d28f4d75a48612773b07339010308ced09e8c8debf8e7708c35b85f7908da5`.
  It is not installed or hardware-tested yet. Stable55 is idle at generation2
  after the guarded tag0113 post-boot restore; the lock is cleared and the
  stable backup matches. Details are in
  `benchmarks/qwen35-dataflow-tag0114-20261121/BUILD.md`.

- 2026-11-21 (tag0113, first hardware failure): Authentication took
  `98.782252` s, vocabulary and prompt tokenization `14.065504` s, and model
  load `330.909273` s for 2740498432 resident bytes. Prompt embedding failed
  after `0.612365` s on core0 with `EmbeddingWeightRead` failure7, phase5.
  No token or decode time was produced. QUERY/RECOVER failed at runtime audit
  stage6/core0; close failed the core0 stop audit. The wrapper retained tag0113,
  its lock, and the stable55 backup at `cold1 active_owner1`. The recovery
  details and hashes are in
  `benchmarks/qwen35-dataflow-tag0113-20261121/RUN.md`.

- 2026-11-21 (tag0113, failure and recovery diagnostics): Added distinct
  embedding worker failure codes and recovery-audit stage/core receipts while
  preserving the fixed worker mailbox and request sizes. The full variant-60
  driver/client/service build, RV32IMF compile, and focused ASan/UBSan owner,
  service-owner, and embedding-worker tests pass. The conservative stack bound
  is 15456/18432 bytes, with the generation sequence inlined into the measured
  ioctl frame. Driver SHA-256 is
  `e2f7d4bb865cab020c6e52cb411eb99b15ec204b1faa7b1c683496493a0d42cf`; client
  SHA-256 is
  `152c5136d707b899e139ed674d3cfb43c85f74f9058c8773795ed6becbbf94e3`. No
  tag0113 hardware result yet. See
  `benchmarks/qwen35-dataflow-tag0113-20261121/BUILD.md`.

- 2026-11-21 (tag0112, offline stack rewrite): Moved the reusable projection
  descriptor to session-owned heap storage, placed the core schedule scratch
  in the heap-owned projection authority, and made the builder fill the
  supplied descriptor instead of stacking and copying a second 64 KiB object.
  Seven focused ASan/UBSan tests pass, as does the RV32IMF compile without A
  atomics or runtime helpers. The full variant-60 driver/client/service build
  passes; its stack audit reports a conservative 15696-byte chain against the
  18432-byte budget, with no Qwen35 frame above 8192 bytes. Tag0112 driver
  SHA-256 is `b2f3a6be660a07a5c76b363b7ab116ba81b68095126e221c5a741f972a37f664`;
  the client remains `a043d2c1ba96ed28133bbf46ced63e6e1b9e70811d0ce5fedac764c3011419ae`.
  The guarded hardware run then loaded 2740498432 packed bytes in
  `328.264191` s after `98.363993` s authentication and `13.123478` s
  tokenization. The first prompt embedding failed: result31/stage4, worker
  core0 state4/reader1/phase5, partial failed-prompt time `0.612929` s,
  output0, decode time0. QUERY/RECOVER returned pending; close failed the
  core0 stop audit and the wrapper retained its lock and stable backup. The
  authorized cold recovery restored stable55 generation2, cleared the lock,
  and verified the 4B/27B files and 64-CPU/full-RAM profile. No valid output,
  completed prompt time, decode time, or tokens/s exists; the 100 tokens/s
  target remains unmet. See `benchmarks/qwen35-dataflow-tag0112-20261121/RUN.md`.
- 2026-11-21 (tag0111, hardware failure): The candidate installed after a
  clean idle preflight, then Haiku panicked during the generation ioctl. The
  compiler stack report exposed frames of 194800 bytes (FFN) and 75808 bytes
  (projection builder), exceeding Haiku's 20 KiB kernel stack. After reboot
  and the pinned SD-free handoff, recovery restored stable driver55 generation2
  idle and verified the 4B/27B file sizes. Authentication was `99.830468` s,
  tokenization `13.752066` s; an upload-progress sample reached 2657021952 of
  2740498432 payload bytes. Damaged logs prevent a completed model-load time;
  no valid output or decode rate exists. See
  `benchmarks/qwen35-dataflow-tag0111-20261121/RUN.md`.
- 2026-11-21 (tag0111, offline rewrite): Added worker-level embedding failure
  receipts to QUERY/RECOVER, made a pending embedding safely abortable after
  the all-core recovery audit, and moved recovery into the same open service
  ownership window before CLOSE. The complete offline build and focused
  ASan/UBSan tests passed. Driver SHA-256 was
  `34b8680b70b9664176492832be457a26bc82263efaf2be39a233f3e1bad29bd4`; the
  client SHA-256 remains
  `a043d2c1ba96ed28133bbf46ced63e6e1b9e70811d0ce5fedac764c3011419ae`.
- 2026-11-21 (tag0110): The tag0109 `EINVAL` was traced to a missing outer
  ioctl route for operations after `RESIDUAL`. `ProbeControl` now routes the
  contiguous rewrite range through generation. Added compile-time range
  assertions, returned request diagnostics, detailed restored-core idle
  subchecks, held-root restore proof handling, and bounded close retry after a
  resume failure. The complete offline driver/client build passes; owner and
  PCI-memory ASan/UBSan tests and the RV32IMF compile pass. Driver SHA-256 is
  `a9b79420fd351bfb7cbcdda2e70565455973a80a154ca54ba985b1d714f3bd5d`, and
  client SHA-256 is
  `b90f94175aa8a847f360b9ac2a118596d1cd092d37e475ea7043f0281a9d43fe`.
  Tag0110 uploaded the full model in `326.685735` s after `99.320494` s
  authentication and `12.779983` s tokenization. Generation failed on prompt
  position0 during embedding (`stage=4`); `0.609649` s is partial failed
  prompt time. No output, completed prompt timing, decode time, or tokens/s
  exists. Close failed the core0 stop audit. The recovery helper restored
  stable55 generation2 after the authorized power cycle and pinned SD-free
  handoff; postflight confirmed 64 CPUs, full RAM, cleared lock, and both
  model sizes. Continue offline rewrite before another hardware run. See
  `benchmarks/qwen35-dataflow-tag0110-20261121/RUN.md`.
- 2026-11-21 (tag0109): Added structured generation-ioctl rejection details
  and split fleet-resume diagnostics, plus a bounded 10,000-spin settle window
  before restored-core idle checks. The all-core Tensix rewrite still has no
  CPU inference path. The full offline variant-60 driver/client build passes;
  owner and PCI-memory ASan/UBSan fixtures pass; the RV32IMF role compile and
  both guarded-script syntax checks pass. Driver SHA-256 is
  `b87123aab5ff32e7080f14addf7e8e8a945c100e6380032eb52745e6a0387f84`, and
  client SHA-256 is
  `d55bed03a653529c78383c04d19584def654fda83d874257e06ab48e406be4a0`.
  Tag0109 silicon evidence is now recorded. It still has not passed generation
  or clean unload. See
  `benchmarks/qwen35-dataflow-tag0109-20261121/BUILD.md`.
- 2026-11-21: Added the standalone resident-dataflow benchmark client and
  builder. It authenticates the pinned GGUF, uploads the packed model, binds
  its manifest and graph workspace, installs the 120-core resident service,
  requests two repeatable greedy generations, reports model-load, prompt, and
  decode timings separately, checks the known `11751,13` output prefix, and
  explicitly closes the service and seven-bank session. Variant 60 now enables
  the required span uploader under its fixed build tag without enabling the
  legacy Work path. The Haiku RV64 client and variant-60 driver build offline;
  driver SHA-256 is
  `0f9e9b04ebcd66509f05da5053f47bd21c323122fd9e41db9ce5934e532cae7a`, client
  SHA-256 is
  `b2f45646e69e4ae7c83bff3f0572ed60c4ea97d822bebfda158f4456f1a49c08`, and
  service text SHA-256 remains
  `2e0e144040f9c29f9679d5300ff923847c75216b3bf0e577366be4e929e5f8c8`.
  Read-only Haiku preflight found no 27B diagnostic process and reported stable
  driver55 tag `2026102100010012`, generation14, `quiesced0 cold0
  active_owner0 open_count1 guard1`. No model inference has run yet. Build and
  preflight evidence is in
  `benchmarks/qwen35-dataflow-tag0102-20261121/BUILD.md`.
- 2026-11-21: Added the request-level greedy generation ioctl at `0x8847`.
  It accepts prompt token IDs, skips unused vocabulary projection for prompt
  positions before the last, returns generated token IDs, stops at an optional
  EOS token, and enforces the model context bound. The model ABI is now 5. The
  response separates prompt time (through selection of the first output
  token) from decode time (subsequent generated tokens), so decode throughput
  is computed from `output_count - 1` over `decode_time_us`. ASan/UBSan checks
  cover prompt head skipping, bounded positions, EOS, and timing fields; the
  RV32IMF target compile and offline variant-60/default-control build pass at
  tag `2026112100010102`. Driver SHA-256 is
  `fea103ea9e5290da1742c2efe3585564eddf5dd627a95b54f904a7ab53582b87`; service
  text remains 52,900 bytes with SHA-256
  `2e0e144040f9c29f9679d5300ff923847c75216b3bf0e577366be4e929e5f8c8`.
  This code is not yet validated on the Pioneer.
- 2026-11-21: Added device-side greedy token selection after the tied Q6_K
  vocabulary projection. The resident TRISC0 worker scans the on-card FP32
  logits and writes a checked token/score/sequence record to core 0's mailbox;
  all 120 workers still receive, complete, and acknowledge the command. The
  host owner verifies each invocation readback, the stable result record, all
  worker receipts, runtime state, and drained queues before committing the
  selected token. The decoder ioctl now returns `output_token` in its existing
  40-byte ABI record. ASan/UBSan checks pass for stable greedy ties, NaN
  rejection, all-core owner choreography, vocabulary authorization, and the
  full mixed decoder sequence. The Haiku RV32IMF target compile passes with no
  A-extension instructions or runtime helpers. An offline variant-60 build at
  tag `2026112100010101` passes: driver SHA-256
  `1cacb820faa496a9d72eabad3d551fbd166ef7ecc9aa5a40b92bb3977550bedf`,
  resident service text SHA-256
  `2e0e144040f9c29f9679d5300ff923847c75216b3bf0e577366be4e929e5f8c8`
  (52,900 bytes of the 57,344-byte limit), reload guard SHA-256
  `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229`, and
  default-control SHA-256
  `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d`.
  This was offline evidence only; the subsequent entry adds the request loop.
  Pioneer correctness and decode rate remain unmeasured.
- 2026-11-20: Added `PioneerQwen35DataflowTensixOps.h` as the concrete
  compute-role adapter. It binds one immutable core invocation, selects the
  right 16-value activation panel and scale, issues the Q8_0/Q4_K/Q5_K/Q6_K
  unpack and four Tensix matrix phases, validates bounded integer dot
  readbacks, runs the SFPU affine accumulator, and publishes a 16-row FP32
  page. Q5 affine scale/minimum extraction is isolated in the rewrite's
  metadata helper; packed Q5 operand expansion and all projection dots remain
  on the Tensix path. The MMIO emitter latches config/drain failures and
  returns through the queue failure channel instead of parking. The compute
  contract now requires explicit `Bind`. ASan/UBSan host role checks pass,
  and the Haiku RV32IMF probe instantiates the concrete Q5 and full compute
  paths without A-extension instructions or runtime helpers. This is compile
  and instruction-model evidence only: separate runnable device images,
  resident all-core ownership, and silicon validation are still missing. No
  Pioneer access or benchmark was performed.
- 2026-11-20: Added a rewrite-only Q5_K Tensix unpack path. The new panel
  span selects the original `qs` and `qh` byte-major windows from either
  double-buffered weight page; its SFPU instructions combine each low nibble
  with the selected high bit and write the existing lossless integer overlay
  into Dst for the matrix source path. The offline vector-layout machine
  matches the scalar reference across 2,048 panels and 524,288 operands, and
  the Haiku RV32IMF compile probe instantiates the Q5 path. These checks cover
	operand decoding and compile lowering only at that checkpoint: no native
	image or silicon qualification existed then. No Pioneer access or benchmark
	was performed.
- 2026-11-20 status review: the rewrite is at the pushed NoC-role checkpoint
  `eb6518f8a7`; no Tensix compute backend, runnable role images, or resident
  all-core owner exists yet, so this rewrite has no valid tokens/sec result.
  At the time of that review, the immediate model blocker was Q5_K: the
  rewrite vector unpacker
  accepts Q8_0/Q4_K/Q6_K, while the only Q5_K panel packers are explicitly
  scalar/offline references. Since Qwen3.5-4B Q4_K_M uses Q5_K tensors, those
  helpers cannot be put on the Work path. No Pioneer access, benchmark, or
  assistant integration was performed during that review.
- 2026-11-20: Added version-2 per-core descriptors with separate bank route
  tables for NoC 0 and NoC 1. Each table is checked for valid, unique XY
  routes and copied into the immutable core invocation; the future device
  owner must provide independently audited routes for both fabrics. Added
  `PioneerQwen35DataflowBlackholeIO.h`: its reader operations issue bounded,
  counter-proved NoC 0 reads for the core's activation fragments and assigned
  weight tiles, and perform exact Q8 activation packing; its writer operations
  issue bounded, counter-proved NoC 1 writes for only the core's assigned
  output rows. This follows the Blackhole BRISC/NoC 0 and NCRISC/NoC 1 data
  movement split described in the [TT-Metalium watcher guide](https://github.com/tenstorrent/tt-metal/blob/main/docs/source/tt-metalium/tools/watcher.rst)
  and [Blackhole NIU memory map](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/MemoryMap.md).
  The host role test passes under ASan/UBSan, and the Haiku target compile
  check now instantiates these actual NoC transfer paths. Its matching route
  fixtures are offline geometry only, not measured NoC 1 routes. Reader and
  writer operations are not yet installed as separate device images; the
  Tensix compute backend and resident all-core program owner remain to be
  implemented. No card work or benchmark was performed.
- 2026-11-20: Replaced the queue's compare-exchange first-error word with
  per-role first-error slots. Each Reader, Compute, or Writer image is the
  sole writer to its role slot, and queue polling checks all slots. The L1
  queue-state reservation now uses 32-byte aligned records. RV32 builds use
  explicit `fence` instructions and aligned volatile 32-bit loads/stores, so
  the queue protocol does not require the RISC-V A extension or a libatomic
  helper. Host checks retain compiler atomics for concurrent test coverage.
  The returned queue error uses fixed Reader/Compute/Writer priority when
  multiple roles fail; each role's first error remains preserved in its slot.
  On RV32, the four failure slots are scanned with one acquire fence after the
  reads, keeping the normal polling path from issuing four fences per check.
  The concurrent role exercise and Q5 panel check pass under ASan/UBSan. A
  compile-only adapter now instantiates the three role templates using the
  Haiku cross-toolchain with the Work image's `rv32imf_zicsr` flags; the
  repeatable check is `tests/run_pioneer_qwen35_dataflow_target_compile_test.sh`.
  The ELF32 object has no unresolved helpers or A-extension instructions, and
  its acquire/release fences are present in disassembly. This checks the role
  contracts' target lowering, not a device backend or runnable image.
- 2026-11-20: User confirmed the work order: finish the Tenstorrent-style code
  rewrite first; only then proceed to hardware benchmarking and assistant
  integration. Keep this file updated as the rewrite's checkpoint log. No
  benchmark or assistant-integration work belongs in the current phase.
- 2026-11-20: Moved the immutable per-core invocation from `0xa0000` to
  `0xaa000`. The prior address overlaps the existing Tensix destination-init
  source window (`0xa0000..0xa03ff`); the new 4 KiB invocation region ends at
  `0xab000`, before root scratch at `0xb0000`. The circular-buffer area still
  ends at `0xa0000`.
- 2026-11-20: Changed queue state publication to acquire/release atomics. A
  producer publishes page writes before advancing `produced`; a consumer
  publishes completion before the producer can reuse the page. Failure
  propagation atomically preserves the first role error. The offline test
  checks the first-error rule and verifies the queue refuses further work
  after failure. A separate ASan/UBSan SPSC stress check exchanged 50,000
  ordered pages between concurrent producer and consumer threads without loss
  or reordering.
- 2026-11-20: Added Q5_K (`type=13`) to the offline dataflow-program and
  activation-packing checks. Its projection tiles use the same Q8_K activation
  packing as Q4_K/Q6_K; Q8_0 remains a separate activation format. This closes
  the Q5 descriptor/activation-plan gap, while its Tensix matrix operation is
  still an unimplemented backend gate. Expanded the descriptor check across
  all eight Qwen3.5-4B quantization/width combinations and rebound every shape
  to all 120 core invocations.
- 2026-11-20: Added an offline-only Q5_K panel-layout reference that preserves
  exact integer weights and FP32 coefficient/minimum metadata in the Tensix Q8
  panel-zero layout. A compile guard keeps this scalar reference out of Work
  code; the eventual backend must produce the layout with device Tensix
  operations. The offline test decodes the synthetic tile through the Tensix
  panel decoder and checks every operand plus metadata.
- 2026-11-20: Reworked the per-core L1 map after finding that two adjacent
  maximum Q6_K weight pages would overlap the existing affine metadata at
  `0x91200..0x91280`. Weight pages now ping-pong between `0x90000` and
  `0x98000` with a checked `0x8000` stride, matching the Qwen35 tile-prefetch
  plan; activation cache and queue state occupy `0x80000..0x89030`, and
  input/output queues start at `0x92000` and `0x92800`. Both the full program
  and copied core invocation reject any page
  layout that overlaps affine metadata, queue state, activation cache, or
  another buffer. Added a rewrite-only vector-unpack entry that accepts the
  current page address and validates both ping-pong slots; its offline check
  verifies every Q8/Q4/Q6 panel source address. Reserved `0x90fc0..0x91280`
  for the Q5 repacker's Q8 panel-zero tile plus affine metadata; compile-time
  offset checks bind its coefficient/minimum fields to `0x91200`/`0x91240`,
  and only Q8 panel zero may unpack from that staging address. The compute Ops
  adapter still does not call these helpers.
  The dataflow and Q5 panel tests both pass under AddressSanitizer and
  UndefinedBehaviorSanitizer after the L1 layout change. A negative compile
  check also confirms the scalar Q5 reference is rejected unless its
  test-only macro is set.
- 2026-11-20: Added `PioneerQwen35DataflowKernels.h` with separate reader,
  Tensix-compute, and writer role contracts over two-page single-producer,
  single-consumer local queues. The compute template requires an explicit
  Tensix-matrix-engine capability, and each role validates its core's shard
  descriptor before touching device data. Role setup errors publish a queue
  failure so a neighboring role can leave its bounded wait.
- 2026-11-20: Added a host-only concurrent reader/compute/writer exercise over
  a complete Q5_K shard using the real queue and role loops. It checks packed
  activation chunks, weight reads, Tensix-call counts, output-page counts, and
  queue drain. This exposed a reader address bug: groups of 16 rows must
  advance by `16 * rowBytes`; the reader now bounds each group and block with
  that stride. The mock Ops validate choreography only and do not perform
  inference. The exercise passes with AddressSanitizer and
  UndefinedBehaviorSanitizer.
- The reader packs each 256-value activation chunk once in local memory and
  reuses that packed chunk across its assigned output groups. Q8_0 weight
  blocks map eight-at-a-time to the same 256-value activation chunk.
- Added `DataflowCoreInvocation`: the full 120-core schedule remains host-side,
  while each core gets only its own immutable shard assignment in the bounded
  4 KiB local region at `0xaa000`. The per-core object is size-checked and
  revalidates its local buffers, queue layout, and bank ranges.
- Added a device-side local-L1 binder for the invocation, queue controls,
  circular buffers, and activation cache at their fixed addresses. It is
  compile-checked only; no Blackhole role image consumes it yet.
- Added reader, compute, and writer entry wrappers that bind that local
  context and publish a role failure to shared queue state if binding fails.
- Expanded the offline program test to compile all three role interfaces and
  check 9,216-row output shards and a 9,216-value Q4_K input across all 120
  per-core materializations. It also checks Q6_K and Q8_0 plans, shard
  coverage, queue address binding, queue publish/consume, ticket wraparound,
  in-place activation packing, Q8 block mapping, and malformed-descriptor
  rejection. The host test passes with AddressSanitizer and
  UndefinedBehaviorSanitizer.
- The existing Work dispatcher and ROOT gather path are retained as the
  checkpoint; no changes to that path are part of this rewrite phase.
- Added `PioneerQwen35DataflowProgram.h`: a 120-core program descriptor and
  builder that maps each projection shard to its weight bank, sharded
  activation input/output ranges, and double-buffered reader/compute/writer
  circular-buffer layout.
- The lower-level concurrent reader/compute/writer loops remain offline
  contracts. The current device implementation is the resident TRISC0 service
  image and per-core projection worker described below; it overlaps the next
  NoC 0 weight prefetch with Tensix work. The host PCI image loader and physical
  mailbox/recovery adapter are not implemented.

## Rewrite gates

1. Complete offline descriptor and per-core worker checks.
2. Build the concrete resident worker image with its NoC and Tensix operations
   for the Haiku RV32 image, with no CPU inference or fallback.
3. Wire the new host owner to the physical PCI transport so it loads the image
   across all 120 cores, materializes per-core descriptors, drives the mailbox,
   and preserves sharded activations across dependent operations.
4. Complete offline target and full-graph correctness gates. Only then begin
   the hardware benchmark and assistant integration phases.

## Latest rewrite work

- 2026-11-20: Added `PioneerQwen35DataflowBlackholeWorker.h`, a concrete
  one-TRISC0-worker-per-core projection path. Each core reads its input and
  weight shard over NoC 0, packs Q8 activation chunks locally, alternates its
  two L1 weight/input pages, issues each output group through the Tensix
  matrix engine, and writes the sharded result over NoC 1. It validates the
  fixed per-core queue state before use and publishes bounded Reader, Compute,
  or Writer failure slots before returning. No host dot-product or inference
  fallback exists. The target compile probe now instantiates this worker, and
  the Haiku RV32IMF cross-compile passes without A-extension instructions or
  runtime helpers. At this initial worker checkpoint, the service loop and
  standalone image builder had not yet been added. No Pioneer access,
  benchmark, or assistant integration was performed.
- 2026-11-20: Added a 64-byte per-core mailbox ABI and a resident TRISC0 service
  loop. The service verifies its local XY route against the immutable core
  assignment, executes one projection invocation, publishes completion or a
  role failure, then accepts another only after host acknowledgement. On that
  acknowledgement it verifies the local queues are drained and clears their
  error slots. Two weight pages now overlap a NoC 0 prefetch with Tensix work.
  The standalone image builder passes and emits a 25,024-byte RV32 image with no
  unresolved imports; the target compile probe still passes without A-extension
  instructions or runtime helpers. The offline mailbox/route check also passes
  under AddressSanitizer and UndefinedBehaviorSanitizer. This is offline image
  evidence only: the physical host launch/acknowledgement owner is missing,
  and the image has not been loaded or run on the card. No benchmark or
  assistant integration was performed.
- 2026-11-20: Added `PioneerQwen35DataflowOwner.h`, a host-side projection
  coordinator contract. It checks all 120 workers, materializes and reads back
  every immutable invocation before publishing any command, waits for terminal
  receipts from the whole shard set, and only acknowledges after the backend
  audit succeeds. Ambiguous partial publication moves the owner into a faulted
  state; `Recover()` requires an all-core backend recovery audit before it
  acknowledges completed workers and resumes with a higher mailbox sequence.
  ASan/UBSan mock coverage passes normal two-projection execution and partial
  launch recovery, including workers resynchronizing across skipped sequence
  values. This is the orchestration contract and mock only: its physical PCI
  transport, image load, recovery proof, and all-core graph are still missing.
  No Pioneer access, benchmark, or assistant integration was performed.
- 2026-11-20: Added `PioneerQwen35DataflowServiceOwner.h` to own the resident
  image lifecycle around that projection coordinator. It requires an idle
  session and a checked 120-route map, captures all core state before writes,
  holds every core, stages the worker image in bounded 256-byte chunks, and
  reads back code, queue controls, mailbox, and boot word before starting any
  worker. It waits for every worker's ready receipt, then delegates projection
  commands. Close stops and audits the fleet, restores and verifies all
  captured L1 and boot state while cores remain held, then resumes each core's
  captured reset state only after the whole fleet matches. The ASan/UBSan mock
  passes full-fleet startup, preflight refusal, partial-start close, stop-audit
  retry, projection, restoration, and original running/held-state recovery. A
  Haiku RV32IMF compile of the service owner also passes. This is still an IO
  contract and mock: the Haiku PCI/L1/reset
  adapter, generated-image packaging, full model graph, and physical recovery
  proof remain unimplemented. No Pioneer access, benchmark, or assistant
  integration was performed.

The pre-rewrite Work baseline is `1b2b57c561`; the branch tip immediately
before adding the resident worker was `830ec1011e`. Rewrite checkpoints are pushed
incrementally to `codex/qwen35-4b-p100a`; use that branch's current tip for the
latest checkpoint.

The Haiku RV32 cross-compiler is available at
`/Volumes/HaikuBuildLocal/generated.riscv64`; use
`tests/run_pioneer_qwen35_dataflow_target_compile_test.sh` for the checked
worker compile. The local Apple Clang 17 build has no RISC-V target.

The lower-level queue role templates remain offline contracts. The concrete
per-core path is `PioneerQwen35DataflowBlackholeWorkerEntry`, which binds
TRISC0-side NoC operations and Tensix matrix operations into one bounded worker.
It prefetches the next weight tile over NoC 0 while the matrix engine processes
the current tile. Tenstorrent's open `tt-lab` worker pattern is the closest fit
for this freestanding Haiku image ([project](https://github.com/tenstorrent/tt-lab),
[BRISC firmware](https://github.com/tenstorrent/tt-lab/blob/main/src/brisc.cpp)).
The resident all-120-worker owner, driver command path, offline generated-image
validation, full decoder sequence, and request-level greedy generation are now
implemented. Silicon correctness and performance evidence remain open.

## Current checkpoint: 2026-11-21

Checkpoint `55df940409` is pushed with device-side greedy token selection and
the full decoder sequence. The current local rewrite adds the bounded
request-level generation ioctl: token IDs go in, generated token IDs come out,
and prompt and decode time are returned separately. Prompt timing includes the
first output token; decode timing covers later output tokens, with throughput
reported as `(output_count - 1) / decode_time`. Tied projections use all 120
Tensix matrix engines. The TRISC0 argmax worker scans logits; all 120 resident
workers take part in each command barrier and receipt protocol. The driver
uses build tag `2026112100010102`.

Offline ASan/UBSan tests cover deterministic tie selection, non-finite-logit
rejection, argmax authorization and owner barriers, full decoder sequencing,
prompt generation, EOS, context bounds, and phase timing. The RV32IMF target
compile and variant-60 driver, resident service image, reload-guard, and
default-control builds pass. The resident service text is 52,900 bytes and
fits the 57,344-byte region. Tag0105 was the first hardware install attempt for
this request-level client. It uploaded and bound the model but failed before
worker readiness, so no silicon output, prompt time, or decode throughput was
produced. The verified failure and post-cycle recovery state are recorded in
`benchmarks/qwen35-dataflow-tag0105-20261121/`.

The offline request/generation loop and standalone benchmark client are
implemented. The initial preflight verified that the prior 27B diagnostic
had exited and stable driver55 was idle. The first tag0105 hardware trial
reached service startup and reported worker-readiness failure. Stable driver55
has been restored at generation2 with the recovery lock removed. Next, add
per-worker readiness diagnostics and bounded startup rollback before another
guarded hardware trial.

The dense-attention baseline `e533c033ef` includes
Tensix matrix projections, device-side Q6_K token embedding and hidden RMS
normalization, manifest-bound graph projections, SiLU×Up, residual save/add,
feed-forward sequencing, recurrent convolution, Alpha/Beta projections,
delta-state update, fused per-head RMS normalization plus SiLU gating, and
dense attention/KV. RecurrentOut uses the Tensix projection path with an
explicit input offset into the retained QKV V channels. Recurrent and dense
layer sequences are implemented. Mixed dense/recurrent full-model
orchestration, vocabulary readout, and token selection remain incomplete. No
hardware test or decode rate is available.

Checkpoint `6e0f12b4ff` adds exact authorization and readiness
tracking for the dense QueryAndGate, Key, and Value Tensix projections. The
projection test now checks the required QG -> K -> V order, workspace role and
layer ownership, and all 120 active Tensix assignments for QueryAndGate. The
focused host test passes with ASan/UBSan, and the Haiku RV32IMF compile check
passes without A atomics or runtime helpers. Dense RoPE, K/V cache packing and
writes, attention, AttentionOut, mixed-layer sequencing, vocabulary readout,
and token generation remain open; there is still no model output or decode-rate
measurement.

Checkpoint `e533c033ef` adds the dense-attention path. A manifest-bound
descriptor reserves the model's rotary table, per-layer KV cache, Q/K norms,
QKV output, gate input, and attention-output scratch. TRISC0 workers perform
Q/K RMS normalization, RoPE, half-precision K/V cache packing, and grouped
query attention with tiled NoC reads and online softmax; the AttentionOut
matrix projection remains on the Tensix path. Every command materializes and
reads back all 120 worker invocations and audits receipts before committing
authority state. Four workers own the K/V heads and sixteen own query heads;
the all-core barrier keeps the resident service synchronized.

`DataflowDenseLayerSequence` now composes residual save, attention norm,
QueryAndGate/Key/Value projections, K/V cache writes, attention, AttentionOut,
residual add, and the existing FFN sequence. Driver commands `0x8844` and
`0x8845` expose the dense-attention primitives and full dense-layer sequence.
ASan/UBSan checks pass for the dense math, worker, program/authority/owner, and
sequence fixtures. The math fixture matches the pinned operators exactly for
RMS, RoPE, F16 conversion, and online softmax. The RV32IMF target compile
passes without A atomics or runtime helpers. Offline resident-service-image,
reload-guard, and default-control builds pass; service text SHA-256 is
`1166102743dc8017be249c1d6a83a7a55642bc425ebbfa3b8eda74e4dbc98a59`,
reload guard SHA-256 is
`d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229`, and
default-control SHA-256 is
`5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d`.
These are offline checks only. Mixed recurrent/dense model orchestration,
vocabulary readout and token selection remain before full-model output.
No Pioneer run or benchmark has been performed.

The graph workspace participates in the projection path. The request can
select the Up, recurrent QKV, or recurrent gate sidecar; the authority maps
each selector to its exact model role and rejects recurrent targets on dense
layers. It tracks readiness per scratch tensor, independently from activation
buffers and from other scratch tensors. ASan/UBSan projection and authority
checks, the Haiku RV32IMF role compile, and driver60, reload-guard, and
default-control builds pass. The driver60 SHA-256 is
`35c2c1e0f5ae691f5943ec56aa18a21a90e3a5bb69c3c691cb919696bf08b90a`.
These selectors make QKV and recurrent-gate projections available through
the existing 120-core Tensix path. No legacy Work changes are part of this
step.

Checkpoint `00b34e48eb` adds the recurrent output projection. Its descriptor
keeps the whole manifest-bound QKV RowSpace and selects the 4096-row V slice
through `inputBase`; the offset is copied to every core invocation and the
reader maps it across source fragments. Only the exact same-layer RecurrentOut
tensor and a successfully completed delta step are accepted. The authority
invalidates the destination before work and consumes QKV after a successful
commit or audited abort. ASan/UBSan delta-authority tests cover the exact V
offset, rejection of a shifted offset, all 120 matrix-worker assignments, and
one-time completion. Existing projection/authority checks pass. The RV32IMF
role compile passes without A atomics or runtime helpers. The resident service
image is 44,224 bytes with no unresolved imports and fits the fixed 57,344-byte
region. The offline driver60 build at tag `2026112000010099` passes; its SHA-256
is `e39be4fb33c5d614cc8959d7dd8a5c60510fe33c4ce6960d73c759c1c0af378d`.
Service text SHA-256 is
`7eea8370b070336d98ffd28897e2ce44fec519249f533b1e83506f80da58f950`; reload
guard and default-control builds pass. These remain offline checks; there is no
Pioneer output or performance measurement.

Checkpoint `cd6c25e985` adds a rewrite-only recurrent layer sequence and ioctl
at `0x8843`, using new driver build tag `2026112100010100`. It composes the
attention norm, four exact graph projections, recurrent convolution and delta
step, V-slice RecurrentOut, attention residual add, and the existing FFN
sequence. The sequence rejects dense layers and invalid positions, preserves
the first failing stage, and performs no tensor math itself. ASan/UBSan tests
cover operation ordering, invalid dense-layer requests, and stopping after a
delta failure. The Haiku RV32IMF role compile passes without A atomics or
runtime helpers. The offline driver60 build passes with SHA-256
`057b487b6543f4103934e8c24bc4f05a0af939b35d2013f4fe74b4e53c0772bc`; reload
guard and default-control builds pass. This is still offline-only: dense
attention/KV and final token generation remain before end-to-end model runs.

The device-side SiLU×Up step is implemented and pushed in `8ed176883c`. Its
manifest-bound schedule assigns all 576 sixteen-row panels over the 120
resident workers (96 workers handle five panels and 24 handle four). Each
worker reads its gate and graph Up panels over NoC, computes SiLU(gate)×Up on
the device, and writes the gate activation in place. The authority requires a
completed exact Up projection and initialized gate activation, invalidates
both inputs before execution, and commits the gate only after the owner audits
all worker receipts, invocation readbacks, and queues. A root-only `SILU`
control opcode exposes this primitive; it does not add generation or a CPU
fallback.

ASan/UBSan checks pass for the SiLU approximation, 120-core descriptor
coverage, exactly-once worker simulation, authority state transitions, and
owner orchestration. The Haiku RV32IMF compile passes with no A atomics or
runtime helpers. The offline driver60, reload-guard, and default-control
builds pass; driver60 SHA-256 is
`d8435142c513f9b94290a706a17d440449fea8b8dd957c27ece2bf128a9873f2`, reload
guard is `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229`,
and default-control is
`5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d`.
These are offline checks only.

The residual save/add operation is implemented and pushed in `019b7fb29e`. The
manifest-bound schedule assigns all 160 hidden-width panels across the 120
workers (40 workers handle two panels and 80 handle one). `SAVE` copies an
initialized Hidden-row activation into residual scratch; `ADD` adds that saved
residual into the activation in place. The authority invalidates residual
readiness before save and consumes it before add, then commits only after
all-core invocation readback, terminal receipts, runtime/queue audit, and
acknowledgement. The root-only `RESIDUAL` opcode exposes the two explicit
modes without host math or fallback.

ASan/UBSan checks pass for exactly-once coverage and save/add arithmetic on
all 160 panels, authority save/add transitions, and owner orchestration across
both modes. The RV32IMF compile passes without A atomics or runtime helpers;
the offline driver60, reload-guard, and default-control builds pass. The new
driver60 SHA-256 is
`5b1aff677a77cc86b75e725e0bd16c2e2a2c34aeb3b277cabbe5164be1c0961a`; the
service text SHA-256 is
`279cac1dddbc27f0c6621fec07b248d267714073355fb0cf83b812d6cc3cf9fe`.

The rewrite step in `2a2405752a` adds `DataflowFfnSequence`, which composes
those device operations into a complete feed-forward sublayer: residual save,
post-attention RMS, Gate and Up projections, SiLU×Up, Down projection, and
residual add. A rewrite-only root opcode exposes this sequence and reports the
failing stage. The coordinator only builds authorized programs and submits
resident worker commands; it performs no tensor arithmetic. Host ASan/UBSan
checks cover complete ordering, invalid requests, and stop-on-first-failure.
The RV32IMF sequence compile check passes without A atomics or runtime helpers.
The offline driver60, reload-guard, and default-control builds pass; the
driver60 SHA-256 is
`5b59a99ea3346af23399ec273bd810c3f9730bb4f5aa8c8cce0c131264045ed9`.
This is an offline-checked, pushed rewrite checkpoint; recurrent/dense
attention, state updates, final vocabulary readout, and token generation
remain unimplemented. No Pioneer access or benchmark was performed.

Checkpoint `ef1279b585` extends the validated graph-workspace sidecar to six
FP32 tensors: Hidden residual, FFN Up, recurrent QKV (`ConvChannels`),
recurrent gate (`DeltaInner`), and 32-row Alpha and Beta vectors. Manifest
validation requires all six regions in allocator order; the model-bind ABI is
now version 3. This gives the recurrent projection and state operators
reserved, bounded output storage. Checkpoint `cd35b0db45` adds the first
recurrent projection targets (QKV and gate); the projection state machine now
commits or invalidates each target independently. ASan/UBSan projection and
authority checks pass, as does the RV32IMF role compile. The offline driver60,
reload-guard, and default-control builds pass. The updated driver60 SHA-256 is
`35c2c1e0f5ae691f5943ec56aa18a21a90e3a5bb69c3c691cb919696bf08b90a`.
Recurrent convolution, delta-state math, dense attention, KV/state IO, final
vocabulary readout, and token generation remain unimplemented. The GGUF
fixture test was not run because no local pinned-model path is available. No
Pioneer access or benchmark was performed.

2026-11-20 rewrite progress: added the manifest-bound recurrent convolution
operator and its resident worker command in checkpoint `a769eea54c`. It reads
the completed recurrent QKV sidecar, the model's four-tap convolution weights,
and the selected
ping-pong history over NoC 0; each of the 120 cores owns four or five of the
512 channel panels and writes updated history plus the SiLU result over NoC 1.
The projection feeding this operation remains the rewrite's Tensix matrix
path; this convolution step is channelwise firmware math, not a matrix
projection. The new authority transition invalidates QKV while state is
updated and commits it only after all-core completion, invocation readback,
queue/runtime audit, acknowledgement, and ready-state checks. A rewrite-only
driver ioctl exposes the command. The offline fixture checks exact position-0
and warm-position arithmetic, ping-pong history, complete 120-core coverage,
authority transitions, and owner sequencing. ASan/UBSan worker/owner and
existing dataflow tests pass, the Haiku RV32IMF compile passes without A
atomics/runtime helpers, and the offline driver60, reload-guard, and
default-control builds pass. Driver60 SHA-256:
`15e433e1b5c15fa1c2bba19255d9268c8b0e2f91e1e6b4826f518ed5383b18c2`.
This is offline-only evidence. No Pioneer access, silicon validation,
generated output, benchmark, or assistant integration was done. Delta-state
math and Alpha/Beta projections, dense attention, state/KV IO, vocabulary
readout, and token generation remain to be implemented before benchmarking.

Checkpoint `281ca54aab` connects the reserved recurrent Alpha and Beta
workspaces to their exact model roles in the existing Tensix projection
builder. The scheduler now permits fewer than 120 output panels: all 120
resident workers still receive, validate, and acknowledge the command, while
the two workers owning a 32-row gate projection issue matrix work and the
remaining 118 complete as bounded no-op workers. The graph authority tracks
each sidecar independently. Offline tests verify Alpha/Beta role and
workspace binding, active/inactive invocation coverage, and the all-core
owner barrier. ASan/UBSan projection, owner, and recurrent-convolution tests
pass; the RV32IMF role compile passes without A atomics or runtime helpers.
Offline driver60 SHA-256:
`a001c424f14e1c09dc8c67dba326732bcc7dc33b475941b1477b8cc971e2e59c`;
service-image text SHA-256:
`507aa854644964ec091fb7a19271d87edcad0dbf794481125ff0eda593db76b2`.
The driver, reload-guard, and default-control package builds pass. These
remain offline checks; no hardware work or performance result exists.

Checkpoint `307c9d134b` adds per-layer recurrent position tracking to the
authority. A convolution can run only at that layer's next committed position;
replay, skipping, and out-of-order requests are rejected. The new
`ResetRecurrentState()` request boundary resets position tracking and
invalidates recurrent scratch without writing the large resident state
regions. This is safe because position-zero convolution substitutes zero
history instead of reading the old ping-pong slot; the delta-state worker
likewise substitutes zero state at position zero. ASan/UBSan tests cover successful advance,
replay rejection, reset, and reprojection before a new position zero. The
RV32IMF role compile passes, and offline driver60, reload-guard, and
default-control builds pass. Driver60 SHA-256:
`a7cd35960b48c7844910e8f2f0df72b661ce19dd014d8b5ca4c1f96d4a715d15`.
No hardware validation or throughput evidence was produced.

## Delta-state worker `221022cc34` (2026-11-21)

At checkpoint `221022cc34`, the manifest-bound delta program validated QKV,
gate, Alpha/Beta, learned decay/bias vectors, and state ping-pong ranges. It
partitioned 256 sixteen-value panels across all 120 workers. The TRISC0 worker
performed Q/K normalization, decay and beta gates, state update, and readout;
projections remained on the Tensix matrix path. The following checkpoint
extends this step with per-head RMS normalization and SiLU gating.

The projection authority permits a delta command only after matching QKV,
gate, Alpha, and Beta projections plus the same-position convolution have
committed. It invalidates QKV and consumes Alpha/Beta while the command is in
flight, then advances the delta position only after all 120 invocations,
worker receipts, runtime/queue audits, and acknowledgements pass. Position
zero ignores stale state and writes the alternate slot. Driver opcode
`0x8842` exposes this operation through the resident service.

The `061aa5c8ef` checkpoint assigns each of 32 recurrent heads to one TRISC0
worker. Each owns all eight state panels plus the complete 128-value RMS
normalization group; gated values are stored back after all source panels for
that head are read. The 120-worker command/receipt/acknowledgement barrier is
unchanged, and projections still use all 120 Tensix matrix engines.

Offline ASan/UBSan fixtures pass for zero-state and warm-state recurrence,
head gates, bit-for-bit RMS-plus-SiLU comparison with the root operator,
descriptor coverage, authority transitions, and the all-core owner barrier.
The Haiku RV32IMF worker compile passes without A-extension instructions. The
size-optimized service image is 43,964 bytes, has no unresolved imports, and
fits the fixed 57,344-byte region. Offline driver60 SHA-256:
`d523169a4c46c51406a7d2c5b63847383095648e23681866b81b230ff4596578`;
service text SHA-256:
`8363fdcc559620eee9553a37c8061ab8ce9e7166595e90120dec1d277eca3d0f`;
reload guard SHA-256:
`d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229`;
default-control SHA-256:
`5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d`.
These are offline checks only. The operation has not run on the Pioneer and
has no silicon correctness or performance evidence.

The earlier checkpoints below describe implementation history. Hardware
correctness, warm-run output, measured throughput, and assistant integration
remain separate milestones. Do not report legacy Work measurements as rewrite
performance.

## Current rewrite progress (2026-11-21)

The tag0106 silicon trial authenticated, tokenized, uploaded the full 4B model,
and bound its manifest, then stopped at worker 0 startup with the local-core
route check failing. It produced no inference output or model-load, prompt, or
decode timing. The post-cycle recovery helper restored stable driver55 at
generation2 and removed the recovery lock. A read-only check confirmed the
Pioneer is idle, with both the 4B and 27B model files intact.

The next rewrite change drains startup results from all 120 workers after the
first readiness failure, retaining the earliest worker diagnostic. This gives
later workers time to reach a stable ready or startup-fatal state before the
existing all-core stop audit runs. The focused owner and PCI-memory tests pass
with AddressSanitizer and UndefinedBehaviorSanitizer, the RV32IMF role compile
passes without A atomics or runtime helpers, and the offline driver/package
build passes. The rebuilt driver SHA-256 is
`531f419a9c6d213cdf75da0afd0e27e327c6750086cda0375d59e3684d8fd420`; the
service text is unchanged. These checks do not establish silicon recovery.
At that checkpoint the route mismatch and exact cleanup failure still needed
hardware evidence; tag0107 later captured raw route `0x312` and a resume
failure, as recorded below. No warm output or decode rate exists; the 100
tokens/s goal remains unmet. Assistant integration remains deferred until the
rewrite and benchmark are complete.

### Tag0107 recovery and route diagnostics

Tag0107 now records the phase-2 worker's raw local route, waits for the full
120-core startup scan, and reports the stop-audit stage/core. Its special
recovery proof admits only that exact route-fatal state when the selected
physical core, service image, captured NoC sources, raw route, queues, and
idle NoC are stable; it does not admit the route for inference. The protocol
is ABI 2. Owner and PCI-memory ASan/UBSan tests pass, the RV32IMF service
compile passes without A atomics/runtime helpers, and the offline package
build passes. Driver SHA-256:
`176aacf58aeb972aabbe05d5c3ca6f89f5af1a1f3648ba96d7c932abea085668`; client
SHA-256: `232a98fd41c77f3e559e5de983a156febb65ff483c16da5181dcc28f268d72cd`.
The guarded tag0107 trial and post-cycle recovery helper were run on the card.
The trial authenticated, tokenized the prompt, uploaded
the complete 4B model, and bound its manifest. Startup again stopped at worker
0: phase 2 rejected raw local route `0x312` (786). No prompt execution or model
load/prompt/decode timing was produced. Cleanup stopped after the client
reported `close_failed result=18` (resident-service resume failure); the
client descriptor closed, but the guarded wrapper correctly refused to swap
drivers while the candidate reported `cold1 active_owner1`. The current card
state is tag0107 driver60, generation3, `cold1 active_owner1 open_count1`, with
the recovery lock retained. The stable driver has not yet been restored.

This run confirms the route-fatal branch is reached and the driver does not
silently fall back to CPU inference, but it does not verify the attempted
no-reboot cleanup path. The next rewrite checkpoint needs a precise
restore/resume failure stage and core, then a corrected worker identity/route
contract before another model run. No valid output, warm run, or decode rate
exists; the 100 tokens/s goal remains unmet. The 27B setup was verified intact
at preflight and was not accessed by this 4B run. Assistant integration remains
deferred until the rewrite and benchmark are complete.

### Tag0108 route and cleanup rewrite (2026-11-21)

Tag0108 no longer derives a logical worker number from the firmware's raw
local NoC register. The kernel writes the selected core ID into the startup
mailbox; each worker validates that assignment and checks invocation ownership
against it. During the all-core preflight the host now captures a stable,
unique local route for each selected core and requires that route to remain
unchanged during inference. Projections still have no CPU fallback and use
the Tensix matrix engine across the 120-core service fleet. Close now reports
the exact core and substage when restoring or resuming the fleet fails.

The new mailbox/client protocol is ABI 3. Five focused sanitizer tests pass,
the RV32IMF compile passes without A atomics or runtime helpers, and the full
offline package build passes. Tag0108 driver SHA-256 is
`62d64043cf5f3e2110b96446839efdfe2c61a945c8d87cd21d9d9a52072de1dc`; client
SHA-256 is `cbee1483b73fa8e08754fbd04d1f78376e725ae7dc6e078ad271fdc2a58a256a`.
The guarded two-warm and post-cycle recovery scripts are prepared.

The guarded tag0108 trial passed preflight from stable driver55 generation2
idle, installed candidate generation3, and uploaded all 2,740,498,432 model
bytes. Authentication took 100.027643 s, vocabulary/prompt tokenization
13.277220 s, and model load/upload 334.815789 s. The first generation ioctl
returned `EINVAL`; no generation completed, so there is no valid output,
prompt-processing timing, decode timing, warm-run rate, or tokens/s. The
client's prefix/repeatability messages are failure checks after the rejected
request, not generated text.

Cleanup returned `DataflowServiceResumeFailed` (result18), diagnostic
stage9/core0, resume phase3, resume detail2 (the final idle-core proof failed);
unload was not clean, and the wrapper correctly kept
the lock and stable backup while tag0108 reported `cold1 active_owner1`.
After verifying the client had exited, one authorized power cycle and the
pinned SD-free Linux-to-Haiku handoff allowed the tag0108 recovery helper to
quiesce the fresh generation1 candidate and restore stable driver55 at
generation2. A read-only postflight confirmed stable55 idle, the lock absent,
both model files intact, and 64 CPUs/full RAM. Raw logs and exact post-recovery
hashes are in
[`benchmarks/qwen35-dataflow-tag0108-20261121/RUN.md`](benchmarks/qwen35-dataflow-tag0108-20261121/RUN.md).

The next code-only rewrite step is to add a precise receipt for the rejected
generation request and resolve the core0 resume phase3 failure before another
hardware trial. The 100 decode tokens/s target remains unmet. Assistant
integration remains deferred until the rewrite and benchmark are complete.

### Tag0119 NoC response-counter diagnostics (2026-11-21)

Tag0119 added a read-only NoC snapshot to the existing 64-byte worker mailbox:
an eight-slot completion-counter match mask, four-initiator busy mask, and
32-slot outstanding transaction-ID mask. It also fixes worker receipt cleanup
to clear exactly the six declared reserved words, preserving `logicalCore`.
The full variant-60 driver/service/client build and focused owner,
embedding-worker, and runtime-recovery sanitizer fixtures pass. The compiled
service uses no A-extension atomics and fits its fixed region.

The one-run hardware diagnostic authenticated, tokenized, and loaded the full
4B model, then failed on prompt embedding read stage 7/index 36. The response
counter observed `0x00029066` where `0x00029067` was expected; the match mask
was `0xfe`, initiator-busy mask `0x0`, and outstanding-ID mask `0x00000001`.
The raw tag0119 client called that last field `failed_read_queue_busy_mask`;
that label was incorrect. Per the Blackhole counter map, bit 0 is outstanding
transaction ID 0. Together with the missing read-response counter and the
advanced command-accepted/request-sent counters, this shows the read request
was sent but its response was not received by the initiating NIU; it does not
yet distinguish a target-address/route issue from a missing return response.
This is a read-only observation; no counter reset, request retry, or Tensix
reset was issued. Authentication took 100.664302 s, tokenization 13.757776 s, load
333.142334 s, and the failed prompt 0.615593 s. No output token was produced,
so decode throughput is unavailable and 100 tokens/s remains unmet.

The tag0119 candidate cold-latched and clean unload failed; the trial preserved
the stable-driver backup and recovery lock. The 4B and preserved 27B model
files were verified intact, and the client process had exited. Haiku accepted
a reboot request, but both pinned SSH profiles later timed out; the existing
serial console showed shutdown-device teardown without a new boot banner.
The user's accidental power reset was reported during this recovery window.
No relay pulse was sent while the current board power state remained unclear.
Stable driver55 generation2 must be verified restored before any next
hardware trial. The two-warm throughput benchmark and assistant integration
remain deferred. Full evidence is in
[`benchmarks/qwen35-dataflow-tag0119-20261121/RUN.md`](benchmarks/qwen35-dataflow-tag0119-20261121/RUN.md).

### Tag0120 request-identity receipt (2026-11-21)

Tag0120 clarifies the tag0119 counters and extends the failed-read receipt.
The fixed worker mailbox remains 64 bytes and the control request remains 36
bytes. The six existing reserved words now preserve the failed read's remote
address, target route, byte count, stage/index, observed and expected
completion counter, counter/initiator masks, and 32 outstanding transaction-ID
slots. A new read-only `QUERY_READ_REQUEST` action returns the read address,
route, and size; it performs no NoC transaction, reset, retry, or counter
clear. The normal owner and 120-worker command barriers are unchanged.

The correctly tagged variant-60 driver/service/client build passes with
runtime tag `2026112100010120` and a `15712/18432`-byte conservative kernel
stack. The RV32IMF role compile has no A-extension atomics or unresolved
helpers. The embedding-worker and service-owner ASan/UBSan fixtures verify
request identity and receipt propagation; the runtime-recovery fixture passes.
Driver SHA-256 is
`153748972239101302603e71f4ba3a3cd665fb6e63d51a7cb4094df91c538d7c`, client
SHA-256 is `73cd7e9b47711866d67bd50eb8cb0ad358138760d2bb3d5cda54cfdf8802a60b`,
and service text SHA-256 is
`2025572629309596d727bbc3aec55b458f643930fbe0f1766633e168c20c4dcd`.

Tag0120 was installed and run after stable55 generation2 had been recovered
and verified idle. Its diagnostic reproduced the missing core0 embedding-read
response and left the candidate cold-latched at generation3 with an active
owner. The recovery lock and stable55 backup were retained. The SD-free NVMe
Linux-to-Haiku kexec handoff is verified and remains the recovery route; no
SD-card steps are required. The driver has no verified in-place ASIC reset
and post-reset initialization path yet. No token output, decode rate, warm-run
result, or clean unload has been established; 100 tokens/s remains unmet.
Assistant integration remains deferred. Build and run evidence are in
[`benchmarks/qwen35-dataflow-tag0120-20261121/BUILD.md`](benchmarks/qwen35-dataflow-tag0120-20261121/BUILD.md).

### In-place recovery guard foundation (2026-11-21)

The official `tt-kmd` Blackhole driver exposes an ASIC reset that sends the
PCIe interface timer interrupt after setting a reset marker. Its chardev
serializes the operation, invalidates the reset generation, zaps mappings,
reclaims TLB windows and iATUs, then marks hardware initialization and firmware
readiness as required. A separate POST_RESET wait/poll path gates reuse until
firmware is ready. Sources: [Blackhole reset callback](https://github.com/tenstorrent/tt-kmd/blob/6d84325ad6ba3c032c01adac2be01c9eb642fd1c/blackhole.c),
[reset ioctl lifecycle](https://github.com/tenstorrent/tt-kmd/blob/6d84325ad6ba3c032c01adac2be01c9eb642fd1c/chardev.c),
[PCIe reset marker and timer trigger](https://github.com/tenstorrent/tt-kmd/blob/6d84325ad6ba3c032c01adac2be01c9eb642fd1c/pcie.c).
This is the design reference, not yet a Haiku ASIC-reset implementation. The
new resident proof gate is compiled and unit-tested, and an isolated
fake-PCI-tested helper encodes the three-write reset trigger. Neither one is
connected to a driver operation: they do not reset the ASIC, restore PCI
state, rebuild mappings, prove firmware readiness, or clear the currently
loaded tag0120 guard. A safe port must also account for Haiku's retained Qwen35
owner, host pages, mapped PCI windows, and persistent cold latch. It must not
merely clear the latch or retry the stuck NoC request.

The tag0120 incident above was subsequently recovered. See the latest entry in
`QWEN35_4B_NEXT_SESSION.md` for the newer qualification failure and restoration.
On 2026-11-23 a restricted empty-session reset qualification failed without
recording its failure stage; it retained a cold owner. A later diagnostic
attempt reached firmware reinitialization but its detail did not identify the
failing operation. After full power cycles and verified NVMe-to-Haiku boots,
stable55 was restored, and it reports generation 2,
`cold0 active_owner0`, and no exclusive lock. The new recovery guard is loaded;
the old guard is backed up. This is not a successful no-reboot recovery and
does not establish model correctness or performance. Model-file metadata was
checked, not a fresh full-content hash. No SD cards were used.
