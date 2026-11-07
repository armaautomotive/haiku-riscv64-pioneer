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
had 14.3% relative L2 difference, and full-model quality is not established.
See [`benchmarks/qwen38-matched-band-physical-20261107.txt`](benchmarks/qwen38-matched-band-physical-20261107.txt).

## Milestone 1: upload the prepared model

**In progress.** Converted cache files passed a real, read-only preflight.
A one-shot upload-and-release test was started on November 7. Its live process
has reported verifying all 1,344 converted fragments and started uploading,
but its upload and cleanup have not yet completed. The running binary predates the separate
batched-read change, which has passed simulated tests and cross-compilation but
has not been timed on the Pioneer. See
[`benchmarks/qwen38-hybrid-upload-session-20261107.txt`](benchmarks/qwen38-hybrid-upload-session-20261107.txt)
for its stage, safety constraints and observation details.

Pass requires a real successful upload and clean release, backed by the
diagnostic's complete output and process exit. The running test intentionally
releases weights and does not run a token.

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

The QGW5 test configuration previously selected the older QGW4 startup
grammar. It now selects the matching QGW5 grammar. A simulated held-memory
journal has staged and verified the 1,344-word address table and restored
the original memory; a simulated late write failure was quarantined. Neither
check is a hardware launch or proof of production-safe cleanup.

Pass requires a real 27B request using the held weights on the P100A, checked
output, and safe cleanup. Report actual loading time and generation rate.

## Milestone 3: improve the measured full-model rate

**Not yet passed.** Profile the real end-to-end request, change the largest
measured bottlenecks, and compare against the same prompt and settings. Keep
CPU validation separate from Work mode. An isolated kernel's speedup or
accuracy result does not establish full-model speed or quality.

## Milestone 4: Assistant

**Not yet passed.** Expose the faster, qualified path in Assistant while
preserving the existing working mode and the machine's CPU/RAM/boot behavior.

Keep existing source, logs and experimental files until their ownership and
value are reviewed. Cleanup does not imply deleting them.
