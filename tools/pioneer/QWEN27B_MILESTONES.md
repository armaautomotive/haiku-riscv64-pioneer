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

**Not yet passed.** The experimental upload session can now keep the bank
session held after upload until explicitly released. Its held/release and
failure behavior have only been exercised using synthetic data and a fake
device. The held session can derive the coordinator's address table from that
same uploaded layout; this is not yet staged to the card. It is not connected
to the native inference owner, and it is not an
Assistant or Work-mode implementation. Model layout, firmware, ownership and
safe cleanup must all be integrated and qualified together before a real
full-model run.

**Integration blocker:** An offline upload-layout test recorded every
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
presence-only upload ledger: the actual session still needs a trusted format
binding before it can prevent mislabeled bytes from being executed.

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
