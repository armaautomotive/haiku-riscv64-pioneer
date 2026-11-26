# Fused residual save and normalization, 2026-11-26

The resident rewrite currently emits a separate all-worker residual-save step
and RMS-normalization step twice per layer. `PioneerQwen35DataflowRmsSave.h`
implements a proposed fused worker: it validates both per-core invocations,
uses the existing RMS input read to save the same 16-value panel, and then
normalizes it. It rejects different core IDs, routes or input spaces, final
normalization, and overlap between the residual destination and the RMS input,
output or weights. A failure after a residual write may leave a partial result;
the caller must retain graph ownership and must not treat it as completed.

The new version-6 stream opcode and worker dispatch are behind
`PIONEER_QWEN35_FUSED_RMS_SAVE_COMPILE_ONLY`, restricted to the non-loadable
resident graph witness. The production graph recorder and layer sequences do
not yet emit this opcode; the candidate has not been linked into a full image
or run in ttsim or on the P100A. If successfully integrated, the design could
remove 64 separate residual-save barriers per full 32-layer graph and avoid
one input-panel read per saved panel, but this is a structural opportunity, not
a measured speedup or a prediction of reaching 100 tokens/sec.

The ASan/UBSan host test exercised the actual fused worker over all 120
simulated core assignments and compared every normalized value bit-for-bit to
the existing pinned hidden RMS operation and every saved value to its input.
It checked exact panel read/write coverage, stream encode and corruption
rejection, bad core, route, overlap and mode, as well as failures before and
after a partial residual save. The selected fused service source and the
standalone fused worker probe cross-compiled for RV32IMF with warnings as
errors; the standalone object had no undefined symbols. An incompatible
non-witness build was rejected. These do not prove physical writer exclusion,
concurrent NoC behavior, graph lifecycle, real model output or recovery.

The independently captured eight-layer graph passed all eight same-input
oracle comparisons, with maximum absolute errors ranging from `8.94069672e-08`
to `9.53674316e-06`; its final output was byte-identical to the earlier
sequential eight-layer simulation. The ordinary reference comparison still
fails at layer 7 because of the documented quantization threshold. The
32-layer capture and two-decode diagnostics remain active. No card was
accessed for this work.
