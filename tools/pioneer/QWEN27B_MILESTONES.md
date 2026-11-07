# Qwen 27B on the Pioneer P100A: milestones

This is the short progress tracker. Do not treat a synthetic test, cached
weights, or a completed upload as full-model inference or a speed result.

## Baseline: measured, not yet usable

The November 6 full-model counting test ran its model inference, including
attention and state, on the P100A. The CPU handled loading, tokenization and
lifecycle. Its observed post-prompt rate was approximately **0.173 tokens/s**;
the first answer was ready approximately 35 minutes after Generate. This is
one limited counting test, not a broad quality assessment. The benchmark and
its precise scope are in
[`benchmarks/qwen38-fast-decode-profile-physical-20261106.txt`](benchmarks/qwen38-fast-decode-profile-physical-20261106.txt).
The target of 20 tokens/s remains unmet, and there is no demonstrated faster
full-model hybrid result.

The experimental hybrid path converts some Q4_K_M weights to BFP4/BFP8;
it is not numerically identical to the original. One matched 32-output band
had 14.3% relative L2 difference. In a separate CPU-only comparison of one
full-model token, the hybrid and original chose the same next token, but the
logits differed by 26.8% relative L2 and their top-five choices overlapped
four out of five. These are warning signs, not answer-quality scores; full
model quality is not established. See the
[physical band result](benchmarks/qwen38-matched-band-physical-20261107.txt)
and [CPU-only hybrid comparison](benchmarks/qwen38-bfp-hybrid-cpu-quality-20261106.txt).

## Matrix-engine attention: experimental, not yet measured

A separate dense-attention worker now puts QK and PV products on matrix
engines. Its actual instructions passed simulated six-head and 24-partition
tests, with a test-only global controller; it has not been integrated into
the full model or run on the card. The more accurate PV path uses four matrix
products; reuse of converted operands cut simulated worker instructions by
about 60% on one fixture, but whether it is faster on the card remains unknown. See the
[experiment and qualification limits](benchmarks/qwen38-dense-bf16-matrix-experiment-20261107.txt).

**Hardware recovery is on hold:** after reboot, several saved trial records
read back as unrelated old binary bytes. Their pre-reboot copies still match
their recorded hashes, and a check-only filesystem scan did not report
missing or multiply allocated blocks, but did report freeable blocks. The
driver-restoration guard refused and the unqualified tag125 driver remains
installed. No recovery guard has been bypassed and no new card test has run.
See the [file-integrity investigation](benchmarks/qwen38-hybrid125-postboot-file-integrity-20261107.txt).

## Milestone 1: upload the prepared model

**Passed for upload and release.** On November 7 the one-shot test verified
all 1,344 converted fragments, uploaded 21.14 GB to the P100A, and released
the span and bank owners cleanly. The client exited 0; its full transcript,
driver state, 64 CPUs and full RAM were independently checked. The measured
session took 6,277.803 seconds (about 104.6 minutes), excluding the earlier
model/cache preflight. That is end-to-end loading and release time, not PCIe
bandwidth or inference time. The test released the weights and ran no model.

The tested binary predates the separate batched-read change, which has passed
simulated tests and cross-compilation but has not been timed on the Pioneer.
A subsequent source-plan check now fetches the plan three times instead of
once per fragment while still comparing every fragment and rechecking source
identity. It too is offline-tested and has not been timed on the Pioneer. See
[`benchmarks/qwen38-hybrid-upload-session-20261107.txt`](benchmarks/qwen38-hybrid-upload-session-20261107.txt)
for its stage, safety constraints and observation details.

This pass does not establish a held-weight inference session or a token rate.

## Milestone 2: hold weights and run the model

**Not yet passed.** The experimental upload session holds the bank session
after upload and derives the coordinator's address table from that same
layout. It is now connected to a separately built native inference candidate;
its end-to-end lifecycle has passed synthetic tests, but its first real
full-model startup failed. It is not an Assistant or Work-mode implementation.
Model layout, firmware, ownership and safe cleanup still need physical qualification.

**Why the tracked integration was necessary:** An offline upload-layout test recorded every
accepted synthetic page and applied the native startup page-presence check.
Both the hybrid layout and the original Q4 layout passed, even though their
weight formats differ. Page presence alone therefore cannot select a safe
worker. The new native owner must bind the uploaded format and layout to the
matching firmware throughout the same session. The test did not use the card
and does not show that the installed Q4 Assistant is using converted weights.
The loader's layout metadata now distinguishes original, BFP4 and BFP8
fragments explicitly; it previously labeled BFP4 as original because their
allocations have the same length. Upload and manifest checks reject a changed
format label. This is host-side metadata, not yet a kernel binding or proof
of the bytes on the card.

The kernel's token-owner bind and memory owner now also reject an explicitly
hybrid layout with an original worker, or an original layout with the QGW5
hybrid worker, before staging. An earlier QGW5 journal fixture used the original
layout; it has been corrected. This check cannot identify the contents of a
presence-only upload ledger. The tracked receipt described below adds a
format and session binding; its actual on-card behavior remains unqualified.

An opt-in kernel receipt now tracks the fixed hybrid upload order, the same
owner/FD/team/generation, the span lease and the existing page-hash ledger;
it can derive the matching hybrid layout after a complete, closed upload.
The receipt is now connected to the actual span-control and native-bind code
in a **non-loadable test configuration**. Simulated control tests cover
same-FD/team admission, an upload, incomplete-seal refusal and reply-failure
quarantine; the actual native context also cross-compiles for Haiku RISC-V.
The verified-model loader now sends the matching BEGIN and SEAL handshake;
a complete synthetic upload passed through that loader and the kernel receipt.
An uncertain FD close now retains the tracked owner instead of implicitly
ending it. The private native context now selects the matching pinned QGW5
programs, stages the hybrid address table and rechecks it immediately before
worker startup. A separately tagged hybrid driver candidate now cross-builds
with the matching pinned programs; a matching upload/release diagnostic also
cross-builds, and simulated tests check its same-FD receipt and failure paths.
The candidate was subsequently installed for the trial described below. A separate full
client now connects upload, seal, generation and final cleanup on the same
FD; synthetic tests cover normal output, cancellation, long requests and
failure cleanup. A Haiku test client cross-builds with that path. No real
model has yet generated a token through it, so there is no measured hybrid
speed or quality result. The private non-loadable test client and candidate
client reject each other's driver identity.
Non-cryptographic page hashes do not prove the format or contents of card
memory, and no full-model or hardware test is implied.

**First physical hybrid trial failed at startup.** On November 7 the one-shot
test completed the model/cache preflight and uploaded all 21,137,039,360
bytes, but the client reported `HYBRID_START_FAILED start; close only` and
exited 1. The parent recorded `FAILED_70_NO_AUTO_RETRY_RESET_OR_RESTORE`.
No tokens were produced and there is no hybrid speed or quality result. The
error does not identify the internal startup stage or cause. The one-shot
stage is `/boot/home/pioneer-hybrid125-full-trial-20261107-a`; the permanent
`.trial-once` marker means do not rerun it. A copy of the available logs is
in `/private/tmp/pioneer-hybrid125-failed.h6FLuiMP/` on the development Mac.
Following the failure a controlled single power-off and single power-on were
sent; verify a fresh boot and the driver state before claiming recovery or
running any further card work. The candidate is not automatically restored
on failure. Improved failure reporting is in development; a reported reply
on a failed control operation is untrusted diagnostic data only and cannot
authorize cleanup or a retry.
The closed-evidence hashes are recorded in the
[failed-trial record](benchmarks/qwen38-hybrid125-start-failed-20261107.txt).

**Offline startup defect found and fixed; hardware confirmation pending.**
A new test feeds the actual complete tracked receipt into the real QGW5
native binding code with simulated OS and bank endpoints. Before the fix,
binding failed because the runtime-region validator required BFP8-converted
FFN stripes to have the original Q4 byte size. The fixed check accepts only
the exact BFP8 size for the designated converted stripes; malformed test
layouts still fail. The joined test now reaches a successful native bind
without contacting a device. The physical trial did not report its internal
failure stage, so this is a concrete defect consistent with that trial, not
proof that it was the only failure or that inference now works.
See the [offline binding result](benchmarks/qwen38-hybrid-runtime-bind-offline-20261107.txt).
An additional [offline integration test](benchmarks/qwen38-hybrid-native-init-sim-20261107.txt)
now carries that same sealed hybrid receipt through binding and all three
native state initializers using the real zero program on simulated endpoints.
It passed without modifying the simulated learned ledger. This is not a
physical startup, model execution, quality or speed result.
The same simulated owner also passed preparation and restoration of held
worker memory for all 120 workers, followed by runtime-metadata staging and
readback; no worker was launched. The original-layout runtime regression also
passed. These are simulated startup checks, not physical inference.
An extended simulation then booted the actual QGW5 program on all 119 worker
emulators; they reached idle with the root still held. No model token was run.
In a further simulation the root also reached its startup marker and accepted
a cancellation request; the test paused before model arithmetic and did not
verify cancellation consumption or a token.
An extended run reached its first simulated learned-data request, but paused
before the data was delivered or any embedding was computed.
A further test delivered a synthetic Q4 row and computed its 5,120-value
embedding on the simulated root, matching a separate test decoder. This was
one artificial row, not a real model, full layer, token or hardware test.

The corrected build and diagnostic now have a separate tag126 identity;
their offline lifecycle tests reject the older tag125. The tag125 driver
builder is retired so changed code cannot silently reuse that identity. A
tag126 driver and client cross-build successfully, but neither has been
installed or run. The existing tag125 one-shot parent is not a tag126 test
harness and must not be reused. Recovery of the normal driver, an independently
guarded new test, and physical confirmation are still required. A source
snapshot from commit 78895c4b06 reproduced the driver and client byte-for-byte
using the same SDK, excluding unrelated worktree edits; the SDK itself has not
been independently reproduced. Neither build is approved for deployment.
See the [tag126 build record](benchmarks/qwen38-hybrid126-build-offline-20261107.txt).

A separate [fresh-epoch recovery helper](pioneer_hybrid125_fresh_recovery.sh)
has been prepared and exercised only against fake endpoints. It preserves the
failed trial, checks the original candidate and stable-driver hashes, requires
a fresh idle generation and stops on any uncertain operation. It has not run
on the Pioneer. Its attestation file is an operator reminder, not proof of a
power cycle: independently verify the OFF/ON sequence, fresh boot and pinned
SSH identity before creating it or attempting recovery.

The QGW5 test configuration previously selected the older QGW4 startup
grammar. It now selects the matching QGW5 grammar. A simulated held-memory
journal has staged and verified the 1,344-word address table and restored
the original memory; a simulated late write failure was quarantined. Neither
check is a hardware launch or proof of production-safe cleanup. The table's
5,376-byte temporary workspace now lives in the heap-only diagnostic owner;
a host-build stack check passed. A separate compile-only check now also builds
the hybrid layout and manifest/journal templates with the real Haiku RISC-V
kernel toolchain (maximum reported static frame: 1,216 bytes). The object has
undefined mock I/O hooks and cannot be loaded as a driver; role checks reject
native and Work builds. This does not connect or qualify the inference path.

Pass requires a real 27B request using the held weights on the P100A, checked
output, and safe cleanup. Report actual loading time and generation rate.

## Milestone 3: improve the measured full-model rate

**Not yet passed.** Profile the real end-to-end request, change the largest
measured bottlenecks, and compare against the same prompt and settings. Keep
CPU validation separate from Work mode. An isolated kernel's speedup or
accuracy result does not establish full-model speed or quality.

The saved full-model profile attributed about 48.85% of its measured
generation body to four projection phases. Even hypothetically eliminating
those phases entirely, with everything else unchanged, would yield only about
0.34 tokens/s from the 0.173 baseline. This is an upper bound for that
specific hypothetical, not a prediction or a limit on a broader redesign.
The 2.83x BFP4 result is for one small operation and does not show a path to
20 tokens/s by itself. A usable solution also needs major improvements to the
other on-card work; evaluate the integrated hybrid once, then reassess.

One architectural direction to investigate is overlapping data movement and
matrix work. The current experimental BFP4 band explicitly drains its engine
twice per input tile; whether that is a dominant cost needs measurement.
Tenstorrent's [compute-engine documentation](https://github.com/tenstorrent/tt-metal/blob/main/docs/source/tt-metalium/tt_metal/advanced_topics/compute_engines_and_dataflow_within_tensix.rst)
describes the separate unpacker, matrix and vector engines, but does not
establish the achievable speed of this implementation or this card.

A separate approximate RMS candidate has passed synthetic host and emitted
RV32 simulations, plus three Qwen-derived vectors using unit rather than
learned weights. It is selected by a separate hybrid inspection image and its
linked final-RMS entry has been simulated; full layer execution and real-card
or model-quality tests remain outstanding. See
[`benchmarks/qwen38-fast-rms-synthetic-20261107.txt`](benchmarks/qwen38-fast-rms-synthetic-20261107.txt).

Another isolated worker experiment avoids refetching the same activation for
each output band. Simulated worker output matched the existing BFP4 result,
with fewer on-card network reads. A separately marked coordinator and worker
inspection build now agree on its changed receipt in simulation; neither is
connected to the native owner. There is no card timing or full-model rate.
A corruption control showed that changing the retained input can silently
change later output, so a real ownership/lifetime proof is required before
integration. A separate on-card checksum diagnostic caught the tested
corruptions in simulation but is expensive there and is not an ownership
proof; it is not included in the fast experiment. See the
[input-reuse experiment](benchmarks/qwen38-bfp4-input-reuse-offboard-20261107.txt).

## Milestone 4: Assistant

**Not yet passed.** Expose the faster, qualified path in Assistant while
preserving the existing working mode and the machine's CPU/RAM/boot behavior.

Keep existing source, logs and experimental files until their ownership and
value are reviewed. Cleanup does not imply deleting them.
