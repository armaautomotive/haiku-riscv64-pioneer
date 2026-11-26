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

## Final ranges and isolated-layer diagnosis

An ASan/UBSan metadata audit inspected the last four records for all 120
workers in the baseline, hybrid and paired full-decode fixtures. In each,
the recorded output checked by the simulator is the final residual; final
RMS reads that range and writes a disjoint activation, while the tied output
and argmax use a separate logits range. A corrupted output-layout copy was
rejected. This checks declared records, not actual runtime writes or hidden
physical aliasing, and does not determine the numerical mismatch's cause.

The simulator-only slice builder now extracts a layer's records from an
authenticated full fixture, reindexes the stream, and injects that layer's
independent reference input. It links the source weights without modifying
them. An independent verifier checked all 120 records against their source,
the deliberate fault, input/output layouts and separation from staged
weights and streams. Baseline layer 15 uses original steps 244..260 and
layer 31 uses 504..520; paired variants at steps 232..248 and 480..496 were
prepared and audited but not run. The relevant independent inputs are finite.

The first layer-15 attempt failed before execution because the new input
layout rows were omitted from the runner's recognized-entry count; the count
was fixed. Corrected layer-15 and layer-31 single-layer simulations are active
in sessions `96093` and `11360`, with logs
`/private/tmp/q35-baseline-layer15-slice-sim-20261126b.log` and
`/private/tmp/q35-baseline-layer31-slice-sim-20261126.log`. An 8-layer prefix
with full-output export and a 16-layer prefix are also active (sessions
`83974` and `5931`). None has produced a numerical result yet. The slices
are offline diagnostics using the exploratory simulator; their injected CPU
reference values are not a Work inference path and establish no P100A speed
or physical recovery.

## Isolated dense passes and recurrent probes

The position-zero isolated baseline layer-15 replay completed 17 records on
120 simulated workers, then faulted intentionally; its 2,560 output values
matched the independent layer-15 reference with maximum absolute error
`2.68220901e-07`. Layer 31 likewise passed, maximum absolute error
`1.04904175e-05`. Logs are
`/private/tmp/q35-baseline-layer15-slice-sim-20261126b.log` and
`/private/tmp/q35-baseline-layer31-slice-sim-20261126.log`; both actual outputs
were retained separately. These inputs came from the independent offline
oracle, so the results isolate those layers but do not validate the preceding
model state, final-logit operations or accumulated full-model output.

The slice builder now also extracts the target recurrent layer's position-zero
convolution-history and delta-state read ranges. The simulator explicitly
zeros those ranges before injecting the independent input. An independent
record/ABI audit checked the exact 14 zero ranges against all 120 recorded
workers for layers 14 and 30 and verified they do not overlap weights,
streams, activations or one another. The runner rejects malformed, negative or
overlapping clear ranges and non-finite input; focused negative tests passed.
The baseline layer-14 and layer-30 replays are active in sessions `41714` and
`91431`, with logs `/private/tmp/q35-baseline-layer14-slice-sim-20261126.log`
and `/private/tmp/q35-baseline-layer30-slice-sim-20261126.log`. No result is
available for these recurrent probes. The 8-/16-layer prefixes and both
hybrid full simulations also remain active. None of this is hardware
performance, live inference, physical recovery or a full-model pass.

Both adjacent recurrent replays subsequently passed. Layer 14 completed 16
records on 120 simulated workers, then intentionally faulted at record 17;
all 2,560 output values matched its independent reference, with maximum
absolute error `4.76837158e-07` and maximum scaled error `3.48618451e-07`.
Layer 30 likewise passed with maximum absolute error `3.81469727e-06` and
maximum scaled error `1.55971315e-06`. Logs and retained actual outputs are
`/private/tmp/q35-baseline-layer14-slice-sim-20261126.log`,
`/private/tmp/q35-baseline-layer30-slice-sim-20261126.log`, and the
corresponding `*-actual-20261126.bin` files. These are isolated layers with
independent inputs, not a successful sequential full-model run. The 8-/16-
layer prefix and hybrid full-model simulations are still active.
### Offline intermediate sensitivity experiment (2026-11-26)

The standalone CPU oracle, used only offline and never as a Work fallback, can
now insert a finite 2,560-float replacement at a selected layer boundary. It
reads back the written tensor and rejects short, long, non-finite, symlink and
out-of-range replacement inputs. The authenticated model and the output files
are unchanged by the injection mechanism. Replacing layer 15 with the oracle's
own layer-15 output produced all 32 layers, logits and token byte-for-byte
identical to the original reference. Injecting the retained isolated simulator
layer-15 output twice gave byte-identical results across both runs.

For that one injection, maximum absolute differences versus the independent
baseline were `2.68220901e-07` at layer 15, `9.53674316e-07` at layer 24,
`0.00655296445` at layer 25 and `0.198349595` at layer 31. Both selected token
`261`. Optional layer-25 tracing left both complete runs byte-identical to
their untraced equivalents. The trace found max absolute differences of
`2.39908695e-06` at attention normalization and `0.000956450822` at the
recurrent attention output, followed by `0.00655030459` at FFN output.
The experiment demonstrates sensitivity to this particular perturbation; it
does not explain the full simulator mismatch, validate a full device result,
justify relaxed tolerances, or demonstrate throughput or recovery. The oracle
logs and outputs are in `/private/tmp/q35-oracle-layer15-*-verified-20261126*`
and `/private/tmp/q35-oracle-layer25-trace-*-20261126*`. Continue the running
prefix and hybrid comparisons and investigate the numerical cause.

An expanded trace of the same two runs first observes the jump at the input
projections: the layer-25 normalized input differs by at most `2.39908695e-06`,
while `linear_attn_qkv_mixed` differs by `0.00162911415` and its `z` gate
projection by `0.00113296509`. Later trace points include convolution and
the recurrent output. The `alpha`, `beta` and gate vectors were identical
in this comparison. The expanded traced control and injected runs each match
their untraced outputs byte-for-byte. This points to the offline projection
computation as the first observed amplification, but its precise cause is not
proven and cannot be assumed to explain the separate full simulator failure.
Logs and outputs are under `/private/tmp/q35-oracle-layer25-detail-*-20261126*`.

An offline projection audit then inspected the pinned model's layer-25 Q4_K
gate and Q5_K QKV weights, using the same CPU GGML Q8_K input quantizer and
dot-product functions. Across the 2,560 normalized input values, the two
quantized inputs differed in one Q8_K integer and its block sum, plus six
block scales. Applying both quantized rows to the actual projection weights
reproduced maximum output differences of `0.00113320351` for the gate and
`0.00162863731` for QKV, close to the traced outputs. Retaining the original
integers and sums while changing only scales reduced those maxima to
`9.53674316e-07` and `1.90734863e-06`. This supports a quantization threshold
as the source of the observed amplification in this single CPU experiment.
The audit source is `tools/pioneer/tests/pioneer_qwen35_q8k_projection_sensitivity.cpp`;
the input files and model must come from the separately authenticated oracle
run. It does not establish the cause of the full simulator mismatch, which
still needs investigation with the sequential prefix results.

### Sequential eight-layer result (2026-11-26)

The baseline eight-layer run completed 131 recorded graph steps on 120
simulated workers, then failed its numerical comparison: 2,455 of 2,560
values exceeded the existing tolerance, with maximum absolute error
`0.00240516663` (row 0 `0.6176795959472656` versus reference
`0.6200847625732422`). The actual row was retained at
`/private/tmp/q35-baseline-8-layer-observed-20261126.bin`; the log is
`/private/tmp/q35-baseline-8-layer-sim-20261126.log`. This is not a pass.
The earlier four-layer boundary passed, while the eight-layer boundary fails;
this does not prove that every intervening boundary passed. A newly
authenticated six-layer prefix is running in session `80390` to narrow it.
An independently initialized position-zero layer-5 slice is also running in
session `47385`; all 120 relocated record sequences, input/output boundaries,
and zero-state ranges passed the independent slice audit before launch.

That isolated layer-5 replay subsequently passed: all 120 simulated workers
completed 16 records and intentionally faulted on record 17. All 2,560 values
matched the independent layer-5 reference with maximum absolute error
`7.15255737e-07` (`/private/tmp/q35-baseline-layer5-slice-sim-20261126.log`).
This uses the reference layer-4 input and cleared position-zero state, so it
does not establish that the sequential six-layer output will pass.

The simulator runner now optionally captures intermediate prefix boundaries.
It audits all 120 workers' residual-add and following residual-save records,
checks disjoint activation/residual ranges, and captures only after root
progress shows the previous barrier completed and before root can release
the following save barrier. Reads occur with the simulator stopped. A missed
observation fails closed; captures are provisional unless all recorded
workers reach the expected terminal fault. The marker confirms execution,
not numerical correctness. Synthetic malformed-boundary and progress-gate
tests passed. A four-layer control run using this option is active in session
`97638` (`/private/tmp/q35-baseline-four-layer-snapshots-20261126.log`); its
captured values have not yet been validated. This is simulator instrumentation
only and establishes no hardware completion or inference result.

As a separate offline sensitivity experiment, replacing layer 7 in the CPU
oracle with the simulator's complete observed output produced a layer-31
maximum difference `0.55916214` and row 0 `-2.617931365966797`, versus
`-2.3027868270874023` for the independent reference. Both CPU runs selected
token `261`; replacing the layer with the original reference yielded all
layers, logits and token byte-for-byte unchanged. This confirms that this
specific early deviation can be amplified, but does not reproduce the
baseline full-simulation row 0 (`-2.2534914016723633`), establish its cause,
or validate the model. Files and logs are under
`/private/tmp/q35-oracle-layer7-prefix-{actual,control}-20261126*`. The CPU
is strictly an offline oracle, not a Work inference path or fallback.
