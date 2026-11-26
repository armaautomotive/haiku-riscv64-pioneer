# Fused residual save and normalization, 2026-11-26

## Latest: fused four-layer result and full run

The four-layer run completed all 55 recorded graph steps with 120 simulated
workers and reached its intentional record-56 fault. All 2,560 final values
passed the independent reference at the existing tolerance; maximum absolute
error was `5.00679016e-06`. The log is
`/private/tmp/q35-rms-save-four-layer-sim-20261126.log` and the actual tensor is
`/private/tmp/q35-rms-save-four-layer-actual-20261126.bin`. This supersedes the
earlier pending-result statements below. It does not establish full-model
correctness, hardware execution or a decode rate.

The 436-step full decode with all intermediate snapshots has started in
session `20202`; its log is
`/private/tmp/q35-rms-save-full-layer-snapshots-20261126.log`. It has no
completed output yet. A separate baseline run has an unresolved layer-9
same-input numerical discrepancy documented in the mixed full-simulator
evidence. No card was accessed.

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
resident graph witness. At the start of this work the graph recorder did not
emit this opcode; later integration and simulator results appear here. The design could
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

## Graph integration and simulator run

The compile-only paired hybrid recorder now emits the fused operation in
place of both residual-save/RMS pairs per layer. Its prompt and decode graphs
contain 433 and 436 steps, down from 497 and 500 in the otherwise matching
paired graph. An ASan/UBSan full-graph test verified all 64 fused steps and all
32 paired FFN steps over 120 records per step, both prompt and decode; it
also verified dynamic-token patch invariance and sink abort on a partial
fused-stage failure. This is recording evidence, not fleet execution.

The first candidate device link exceeded the primary L1 code region; the
experimental fused image now uses separate simulator-only linker layouts and
fits its reserved regions. The final resident image has 57,024 bytes of
primary text, 26,028 bytes of dispatch and 14,240 bytes of extra simulator
code. Its added fused-path stack-frame envelope is 5,712/8,192 bytes including
a 1,024-byte allowance; this is not a whole-program proof. The final rebuilt
boot, text, dispatch and extra-code regions match the image used below byte
for byte. The generated fused service header contains a compile-time error,
and both driver-header packagers reject its marker. Baseline paired graph,
FFN sequencing, marker mismatch, header packaging and worker tests passed.
The pre-existing paired-hybrid control was also rebuilt after these changes;
its boot, text, dispatch and extra-code binaries matched the previous
`/private/tmp/q35-ffn-pair-resident-20261126` image byte for byte.

The authenticated 4B fixture recorded a four-layer prefix with 55 executed
steps and an intentional 56th-record fault. It is running in exploratory
ttsim as session `13066` using 120 simulated workers; its log is
`/private/tmp/q35-rms-save-four-layer-sim-20261126.log`. No numerical outcome
is available yet. The simulator-only extra code window remains unqualified
for hardware, and the in-place recovery and physical ownership proofs are
still incomplete. No P100A was accessed and no tokens/sec was measured.

The actual fused recorder method also cross-compiled for Haiku/RV64; the
emitted binder frame is 976 bytes and the probe frame 128 bytes, neither a
complete kernel stack bound. Build and stack records are at
`/private/tmp/q35-rms-save-recorder-rv64-20261126.{o,su}`.

The simulator boundary observer now also understands the fused RMS/save
record: it checks all 120 nested core IDs and input spaces, and rejects any
normalized-output or residual destination overlapping the captured input.
The record offsets are asserted in the C++ ABI for host and RV32 builds.
The 120-worker four-layer recorded fixture passed this audit for all three
intermediate boundaries; synthetic wrong-core, wrong-mode and overlapping
destination cases were rejected. This only establishes the observation
layout. The already-running fused simulator did not use intermediate capture,
and its numerical result remains pending.

An authenticated full 32-layer decode fixture has now been prepared with
436 records and 2,740,473,856 staged bytes. Its staged-weight SHA-256
`591910f9cb91abd0392ac0efbb86c0b4c09f12e32a7bb4315b48972d23752be6`
matches the earlier full fixture; records SHA-256 is
`456b4d67a22922b7f3ae64b079440d9ad0fbd34f0bbd9e8d4c6ff5970e807523`.
The observer independently audited all 31 intermediate boundaries across
the 120 worker records. This prepares a full-model simulator run but is not
execution evidence. The fused four-layer execution remains active and must
be evaluated before treating this fixture as validated.
