# Recorded mixed-layer and full-decode simulator work, 2026-11-25

The production recorder and packed plan generated a four-layer prefix from the
SHA-256 authenticated Qwen3.5-4B Q4_K_M GGUF
`00fe7986ff5f6b463e62455821146049db6f9313603938a70800d1fb69ef11a4`.
The fixture places the real weights across seven simulated banks and starts
with token 42's real embedding. Three recurrent layers and the first dense
attention layer completed 66 recorded steps on 120 simulated workers. All
workers reached the deliberate next-record fault. All 2,560 output values
matched an independently evaluated llama.cpp reference with maximum absolute
error `5.00679016e-06` (acceptance `2e-5 * max(1, abs(reference))`).
The log is `/private/tmp/q35-recorded-four-layer-sim-20261125a.log`.

The llama.cpp reference is a standalone offline validation program built
without Metal, from submodule commit `c060ca974c773c7c3d17fd1b66dc9d312bc292c0`.
It authenticates the same opened GGUF, records layer activations and greedy
choices, and can retain its own state for a second reference position. It is
not linked into Work mode or used as an inference fallback. Its layer-zero
result agreed with the earlier separately simulated reference to within
`8.94e-8`. Its greedy token choices for the two positions are 261 and 89598.

The full decode fixture contains all 524 recorded steps and stages
2,740,473,856 bytes of weights, including the tied output head. Its first run
stopped because this exploratory simulator does not implement the RISC-V
`cycleh` CSR used by decode profiling. No simulator CSR behavior was faked.
An explicitly marked compile-only image disables that profiling exclusively
for simulation, and both hardware-image packagers reject the marker. The
normal profiled control image remained byte-identical to the prior build. A
120-worker synthetic decode fault-path test passed with the special image.
The complete recorded decode was retried with that image and is running at
this checkpoint; **its final output is not yet verified**. The active log is
`/private/tmp/q35-recorded-full-decode-sim-20261125b.log`.

The fixture generators reproduce byte-identically under optimized and
ASan/UBSan builds for the tested one-layer case; four-layer binary files also
match. Header packaging rejection, corrupted candidate receipts, command
freshness, and the 120-worker receipt audit passed their offline tests. The
experimental simulator contains exploratory matrix patches and is not a
qualified hardware reference. These results do not establish physical card
execution, a valid full-model output, clean unload, no-reboot recovery, or a
decode rate. No P100A was accessed for this checkpoint.

Full fixture SHA-256: records
`fa2e60a7617262957e8ca3c3f09727e7474aa07417c2b86e896638213035bdda`,
staged weights
`591910f9cb91abd0392ac0efbb86c0b4c09f12e32a7bb4315b48972d23752be6`.
The simulator-only image text SHA-256 is
`60cbf8f5946a960a809acf2bf91160de586711e67e646215dc9258d61ea23a09`.

The simulator runner now supports a second full decode after auditing and
acknowledging the first result, preserving the same simulator fleet and
model state. Its fresh ticket uses the actual first token. A sanitizer-built
verifier checked the generated command using the actual resident reader and
all 120 recorded descriptors, and rejected a corrupted command and stale
sequence or epoch. Framing tests also checked corruption and exhaustion. The
two-token full-model path has not been run and remains unverified.

The sanitizer-built handoff verifier subsequently walked all 62,880 actual
full-decode records through the resident dynamic patcher. For token 261 at
position 1, it checked the tied embedding address (120 records), convolution
history swaps (2,880), recurrent state swaps (5,760), and dense attention
positions (2,880); every other record was byte-identical. A corrupted command
was rejected. This is a transformation audit, not execution of a warm run.

## Completed full-decode mismatch — 2026-11-26

The unprofiled non-hybrid full-decode process exited with failure after its
120-worker candidate audit. It selected token `261`, matching the independent
reference, but its first compared final hidden value was
`-2.2534914016723633`, against reference `-2.3027868270874023`; the
difference exceeds the existing `2e-5 * max(1, abs(reference))` threshold.
The process did not retain the other actual values, so neither the worst error
nor the cause is known. Correct token selection alone is not a numerical pass.
Log: `/private/tmp/q35-recorded-full-decode-sim-20261125b.log`.

The standalone offline CPU oracle was extended to export all 32 layer outputs
from the authenticated GGUF. Its layer-0, layer-3, layer-31 and token files
are byte-identical to the earlier independent oracle run. A new authenticated
16-layer non-hybrid fixture contains 261 executed records and an intentional
fault record; its simulator run against the independent layer-15 output is
active in session `5931` (`/private/tmp/q35-baseline-16-layer-sim-20261126.log`).
This is a diagnostic for the divergence, not Work-mode inference or a
performance test.

The simulator runner now reports the number of failing rows and worst error
rather than stopping at the first mismatch, and optionally saves a single
actual layer output before comparison using exclusive creation. Focused tests
checked aggregate error, non-finite data, truncation and refusal to overwrite
an existing output. Existing simulations started before this runner change
and do not gain those diagnostics. The two hybrid full simulations remain
active; no physical card, recovery or throughput result is implied.
