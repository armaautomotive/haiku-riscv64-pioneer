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
