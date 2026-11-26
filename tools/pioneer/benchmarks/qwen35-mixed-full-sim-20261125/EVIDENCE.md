# Recorded mixed-layer and full-decode simulator work, 2026-11-25

## Two-position diagnostic result and new capture, 2026-11-26

All three isolated warm query, key and value projections passed their CPU
comparisons across 120 simulated workers: maximum absolute errors were
`9.53674316e-06` for 8,192 query/gate values, `2.86102295e-06` for 1,024 key
values and `1.31130219e-06` for 1,024 value values. The saved values and
hashes are in `warm-layer3-projections-reference-20261126.json`.

An offline CPU counterfactual replaced all three position-one projections
with the exact simulated tensors and read each replacement back. A
self-injection reproduced all 87 pre-existing CPU outputs and traces
byte-for-byte. An independent two-position float-attention calculation used
the CPU-normalized/rotated injected query and key, the injected value and
gate, and the captured simulated prior KV. It matched all 4,096 simulator
context values within `5.36441803e-07`; the CPU flash-attention context
still differed on 170 values, maximum `0.0005030334`. The pinned llama.cpp
flash F16 path converts the query to F16 and accumulates values in F16;
the rewrite uses F32 query and accumulation. This is strong evidence of an
attention-precision reference difference for this isolated case, not proof of
full-model correctness or hardware behavior. The calculation's synthetic
head-mapping, malformed-input and mismatch tests passed. Inputs, hashes and
results are in `warm-layer3-float-attention-reference-20261126.json`.

The same offline CPU model produced tokens `[261, 89598]` with flash/F16 KV,
`[261, 1850]` with non-flash/F16 KV, and `[261, 89598]` with non-flash/F32 KV.
The previously failed simulator diagnostic selected `[261, 1850]`. Injecting
the simulated warm projections into the non-flash/F16 CPU run changed its
second selection to `89598`, illustrating input sensitivity. The matching
token is not a full hidden-state pass, and the original strict failure was
not relaxed. See `cpu-attention-modes-20261126.json`. No card was accessed.

Separate warm query, key and value projection probes were derived from the
original dense-layer stream and independently audited against all 120 worker
records. They execute 3, 4 and 6 steps respectively on fresh simulated
fleets, using the same position-one CPU layer input; no prior KV is read by
these slices. The CPU trace's 85 existing outputs and traces remained
byte-identical after adding key/value projection capture. The simulations
completed as described above, with logs
`/private/tmp/q35-warm-{q,k,v}-probe-20261126d.log`. These probes do not
establish hardware operation or throughput.

The nine-step isolated warm hybrid-attention-context replay reached its
120-worker terminal audit but failed the independent CPU context comparison
on 171/4,096 values, maximum absolute error `0.000502943993`; log
`/private/tmp/q35-warm-layer3-context-sim-20261126d.log`. The discrepancy is
therefore present before the attention output projection and FFN in this
isolated simulator case. The prior KV injection control and host rotary check
are described below. This does not by itself identify which current-position
projection or attention operation differs, or prove the full-model cause.

The isolated warm dense layer-3 replay completed all 18 barriers and its
120-worker audit but failed the final-hidden comparison on 2,489/2,560 values,
maximum absolute error `0.00480294228`. The CPU oracle was then given the
exact captured position-zero simulator KV before its second position. A
self-injection control reproduced all 68 CPU artifacts byte-for-byte, and
the simulated-KV injection left the CPU position-one layer-3 output
byte-identical to the original. The isolated simulator discrepancy therefore
remains with identical prior KV and the same CPU layer input in this test;
see `warm-layer3-kv-control-20261126.json`. This does not prove the cause of
the full-model failure.

An 11-step attention-only warm replay reached its 120-worker audit but failed
the CPU attention-residual reference on 1,998/2,560 values, maximum absolute
error `0.000288227573`; log
`/private/tmp/q35-warm-layer3-attn-sim-20261126d.log`. A host same-input
check of the actual preload RoPE table and rotary function against traced
CPU position-one queries and keys matched all 5,120 values bit-for-bit; it
does not establish device rotary behavior. The nine-step context replay
result is recorded above. The full two-token capture remains session `45891`.
No card access or throughput result.

The isolated position-one replays for recurrent layers 1 and 2 each
completed their 120-worker, 16-barrier audit and matched all 2,560 CPU
reference outputs, maximum absolute error `7.15255737e-07` for both. Each
used its captured position-zero simulator state and exact CPU input for that
layer on a fresh simulated fleet. Hashes and scope are recorded in
`warm-layers1-2-reference-20261126.json`; logs are
`/private/tmp/q35-warm-layer{1,2}-sim-20261126d.log`.

The same isolated replay now supports the dense layer's prior F16 KV state.
The layer-3 records, ranges and KV map were independently checked against
the full graph and completed four-layer capture; optimized and ASan/UBSan
builders produced identical nine fixture files. The warm layer-3 simulator
result is recorded above (log `/private/tmp/q35-warm-layer3-sim-20261126d.log`). The
prior KV comparison still failed two values at the existing strict tolerance.
None of these isolated tests establishes full-model warm output, physical
ownership, recovery or speed.

Two isolated warm replays now use the recorded full-model layer 1 and 2
programs, the independently captured position-zero simulator states, and the
independent CPU position-one input and output for the respective layers. The
builder uses each position-zero write buffer as the position-one read buffer;
an independent verifier compared all 120 reindexed streams and the exact
state ranges to the original graph. Both state maps matched the audited
four-layer capture, and the six captured recurrent-state files still match
their recorded SHA-256 values. The ordinary cold layer-1 slice passed the
existing independent audit after the builder change. The command framing,
state geometry, wrong-layer and overlap rejection tests passed; optimized and
ASan/UBSan layer-1 builders produced identical nine fixture files. The layer-1
and layer-2 simulations completed as described above.
They use a fresh simulated fleet and cannot replace a full resident handoff
or hardware test. No card was accessed.

The four-layer capture completed its 120-worker, 67-barrier audit and passed
the existing final-hidden check (maximum absolute error `5.00679016e-06`).
All six recurrent-state comparisons for layers 0–2 passed the existing
`2e-5 * max(1, abs(reference))` tolerance. Layer 3's position-zero dense KV
comparison failed: 12 of 2,048 half values differed and two exceeded that
tolerance, maximum absolute error `6.103515625e-05`. The independent CPU
trace left all 68 earlier output artifacts unchanged. The full result and
hashes are `four-layer-state-reference-20261126.json`.

A second CPU trace injected the separately audited simulated layer-2 boundary
before layer 3 and read it back exactly. Against the same captured KV, four
half values differed, including the same two strict failures; each CPU F32
value was within `1.38e-07` of the midpoint between the two adjacent F16
values. See `layer3-kv-injected-reference-20261126.json`. This supports a
rounding-threshold investigation, but the exact layer-2 input in this
particular four-layer execution was not captured independently, and these
checks do not prove the cause or excuse the failed tolerance. The first 50
records of this four-layer fixture matched the full hybrid fixture for all
120 workers. No hardware inference or performance was measured.

The separate token-261 warm layer-0 replay completed all 21 barriers on 120
simulated workers and reached its intentional record-22 fault. It imported
the prior token-42 simulator state, and its full 2,560-value layer output
matched the independent position-one CPU reference at the unchanged tolerance,
with maximum absolute error `2.38418579e-07`. The runner separately checked
all 524,288 updated delta values against its own same-input operator reference
(maximum error `5.34374923e-07`). Results and hashes are in
`layer0-token261-warm-reference-20261126.json`; log:
`/private/tmp/q35-token261-layer0-warm-20261126c.log`. This uses a separate
synthetic memory layout and simulator process, not the failed full-model run;
it establishes neither full-model correctness nor hardware output or speed.

The offline oracle now exports the 24 convolution and 24 delta inputs used
at position one. Its existing 68 layer/logit/token artifacts remain
byte-identical to the two-position reference, and the 48 captured states have
the expected sizes and finite values. The already saved, hash-verified
position-zero layer-0 simulator handoff from the separate first-layer fixture
was compared against these inputs: all 24,576 history values and all 524,288
delta values passed the existing `2e-5 * max(1, abs(reference))` tolerance;
maximum absolute errors were `7.62939453e-06` and `1.07288361e-06`. See
`layer0-state-reference-20261126.json`. This first-layer fixture used the
real model and token 42 but is not the failed full-model execution.

To test its next layer input with the actual token 261, a synthetic-layout
first-layer replay was generated and run with the saved simulator state; its
completed comparison is recorded above (log
`/private/tmp/q35-token261-layer0-warm-20261126c.log`).
The 16-row embedding tile matched the original authenticated GGUF bytes in a
separate raw-row audit, and a corrupted tile was rejected. Optimized and
ASan/UBSan fixture builders produced identical 15 files; the token-42 control
remained identical to the earlier fixture. This replay is in a separate
simulator process and does not establish full-model correctness or speed.

The state-layout extractor validates every recorded core's stream and state
destinations; its first four layers match the full graph's ranges, and a
corrupted record was rejected. The optimized and ASan/UBSan four-layer maps
match. The Python capture checks range geometry and overlap with staged
weights, stream records and outputs, and writes state only after the worker
terminal audit; its map must still be independently tied to the executed
fixture. The four-layer result is recorded above (log
`/private/tmp/q35-first-four-state-20261126c.log`). The full two-position
capture remains active in session `45891`.
No card was accessed for any of these diagnostics.

The offline oracle can also replace every position-zero layer boundary with
the corresponding simulator boundary before advancing to the next layer. All
32 inputs matched the saved, already audited snapshot SHA-256 manifest and
each was read back exactly. The forced first position selected token 261; the
next position still selected 89598, not the simulator's 1850. The forced
oracle's second-position layer 0 was byte-identical to the ordinary reference;
layers 3 and 31 differed, with maximum absolute errors `0.00475425273` and
`0.238363504`, respectively. This shows the first-position boundary changes
alone did not reproduce the simulated second token under this CPU computation;
it does not recreate or compare the simulator's recurrent/KV state and does
not eliminate per-operation or state-handoff defects. An all-layer
self-injection reproduced all 68 baseline files byte-for-byte. A missing
completion marker was rejected; the oracle checks all replacements are
finite and read back exactly. Logs:
`/private/tmp/q35-llama-force-{self,sim}-output-20261126b.log`.

The offline CPU oracle now permits a tensor replacement on the second of two
positions, after computing the first position normally and retaining its CPU
recurrent state. An authenticated full two-position reference saved all 32
layer outputs at each position; its second final layer was byte-identical to
the earlier reference, and the tokens were again 261 and 89598. Injecting its
own position-one layer-zero output reproduced all 68 first- and second-position
layer, logits and token files byte-for-byte. Changing one value in that
replacement left all 34 position-zero files unchanged, read the replacement
back exactly at position one, and changed the position-one final layer. A
first-position self-injection also reproduced all 34 original files. The
oracle was compiled with warnings as errors; its use remains offline only.
The CPU position-zero recurrent state is not the simulated recurrent state,
so a same-input warm-layer comparison with this oracle alone cannot rule out
state-handoff differences. Reference and control logs are in
`/private/tmp/q35-llama-{two-all-layers,warm-self-inject,warm-changed-inject,first-self-inject}-20261126b.log`.

The unpaired hybrid diagnostic completed both 532-step decodes in the same
simulated resident fleet. The first candidate selected the reference token 261
but failed the strict hidden-state comparison on 2,558/2,560 rows, maximum
absolute error `0.370739102`. The simulator-only diagnostic explicitly
acknowledged that mismatch to investigate the next position. The second
candidate and its fleet receipts were audited, but its token was `1850` versus
reference `89598`; the process exited 1. The runner rejected the second token
before saving its hidden state, so that state was not captured. This is a
failed diagnostic, not a valid warm run or a hardware result. The first
position's divergent state also means the result alone cannot distinguish
state-handoff defects from accumulated numerical differences. Log:
`/private/tmp/q35-flash-two-decode-diagnostic-20261126.log`.

The simulator runner can now retain second-position layer boundaries and the
terminal hidden state even if the last token differs, but only in explicit
diagnostic mode; it still fails the run and continues to reject a mismatching
first token, non-finite data, invalid receipts or missed snapshot windows. The
existing numerical-comparison and boundary-window tests passed. A repeat with
the same unpaired image, fixture and reference is running as session `45891`,
log `/private/tmp/q35-two-position-warm-boundaries-20261126b.log`, with new
capture directories under `/private/tmp/q35-two-position-*20261126b`. Captures
are provisional until the simulator completes the fleet audit. A repeat
cannot by itself establish the cause or performance. No card was accessed.

## Completed baseline same-input audit, 2026-11-26

The independent audit of the completed baseline run passed 30 of 32
same-input layer checks. Layers 9 and 18 failed the unchanged tolerance; the
input-specific quantization-threshold investigations below account for the
observed discrepancies in those two cases. Replacing the CPU reference's
final hidden state with the exact simulated state selected the same audited
token, `261`. The full audit still **failed** because two layer checks failed;
it does not validate all inputs, warm decode, physical inference or speed.
The complete report, including hashes and individual results, is
`baseline-same-input-audit-20261126.json`. No card was accessed.

## Baseline full execution and final-layer replay, 2026-11-26

The 524-step baseline full-model simulation completed its worker and
candidate audits and selected reference token `261`. Its final hidden state
failed the unchanged fixed-reference comparison on 2,558/2,560 rows, with
maximum absolute difference `0.370739102`; the simulator process exited with
failure. The separately audited layer-31 replay from the captured layer-30
input reproduced the full run's final hidden tensor byte-for-byte. The
execution marker and all layer snapshots now exist. The independent full
same-input boundary audit and token check are reported above. This is not a
full numerical pass, a warm run,
hardware inference, recovery or throughput. Logs:
`/private/tmp/q35-baseline-full-layer-snapshots-20261126.log` and
`/private/tmp/q35-baseline-full-same-input-audit-20261126.log`.

## Two-position diagnostic progress, 2026-11-26

The unpaired hybrid simulator's first decode completed the full worker and
candidate audits and selected reference token `261`, but its fixed-reference
final hidden state failed on 2,558/2,560 rows (maximum absolute error
`0.370739102`). The explicit diagnostic-only option acknowledged the audited
candidate and began position one with the same model and recurrent state; the
run will still fail if either hidden comparison fails. This is not a valid
warm-run pass. Log: `/private/tmp/q35-flash-two-decode-diagnostic-20261126.log`.
No card was accessed and no throughput measured.

## Final-layer replay from captured layer 30, 2026-11-26

An isolated layer-31 replay used the exact provisional layer-30 activation
from the running baseline capture. All 120 workers' 17 replay records matched
the original decode fixture's steps 504–520 after sequence renumbering, and
each worker's next record was the intentional fault. The replay completed
the worker audit and matched all 2,560 values against an independently
authenticated CPU continuation within the existing tolerance (maximum
absolute difference `1.90734863e-05`). The reference retained the exact
replacement input. Log:
`/private/tmp/q35-baseline-layer31-chained-sim-20261126.log`.

Separately, the CPU reference retained the exact simulated layer-31 output
and selected token `261`; log:
`/private/tmp/q35-baseline-layer31-chained-token-reference-20261126.log`.
This token check is offline reference work, not Work-mode inference. The
running full simulation has not yet produced or audited its final layer or
candidate, so the input snapshot and this replay are provisional and cannot
establish a full decode, warm runs, physical recovery or throughput. No card
was accessed.

## Provisional transitions through layer 30, 2026-11-26

The baseline capture's same-input CPU checks passed all six transitions from
24→25 through 29→30 under the existing absolute/relative tolerance. Each
reference replaced its input with the exact captured simulator activation and
verified the replacement and both snapshot files remained unchanged. Hashes,
individual results and limitations are in
`/private/tmp/q35-provisional-boundaries-24-29-20261126/provisional.json`.
These snapshots remain provisional until the running 32-layer simulation
finishes its full worker and candidate audit. No card was accessed.

## Layer-18 output-projection sensitivity, 2026-11-26

With the same captured input, isolated simulations passed the Z projection
(4,096 values, maximum error `4.29153442e-06`), alpha and beta projections
(32 values each, maxima `4.29153442e-06` and `3.81469727e-06`), convolution
(8,192 values, maximum `3.87430191e-07`), and the recurrent update and gated
normalization (8,192 retained QKV values, maximum `6.2584877e-07`). Each
included the 120-worker and intentional-fault checks. The three-record QKV
slice passed as recorded below.

An independent CPU Q8_K counterfactual compared the reference recurrent
output with the exact simulator output before the authenticated model's
`blk.18.ssm_out.weight` projection. One quantized value and its block sum
changed. The full projection changed by up to `0.000223390758`, versus
`1.04308128e-07` for scale changes alone. Using the counterfactual projection
and the same residual predicted the simulator attention residual within
`9.53674316e-07` across all 2,560 rows. The original CPU/simulator residual
difference was `0.000223394483`. This supports a quantization-threshold
explanation for this specific attention discrepancy. An independent CPU
continuation with the simulator's exact attention residual reproduced all
2,560 final-layer values within the existing tolerance, with maximum absolute
difference `1.14440918e-05`. This supports propagation of the attention
discrepancy into this particular final-layer mismatch. CPU continuation log:
`/private/tmp/q35-layer18-residual-injected-oracle-20261126.log`. No tolerance
was relaxed and no card was used.

Logs: `/private/tmp/q35-baseline-layer18-{z,alpha,beta,conv,recurrence}-sim-20261126.log`.

## Layer-18 replay results, 2026-11-26

The isolated same-input layer-18 replay completed its 120-worker audit and
reproduced the captured simulator layer byte-for-byte, but failed the unchanged
reference tolerance on 2,469/2,560 rows (maximum absolute difference
`0.00355863571`). Its attention-residual slice also failed on 1,861/2,560
rows, maximum `0.000223394483`. An earlier two-record attention-normalization
slice passed all 2,560 values bit-for-bit, including the worker and intentional
fault audits. These findings localize the discrepancy after normalization;
they do not yet establish its cause. Logs:
`/private/tmp/q35-baseline-layer18-chained-sim-20261126.log`,
`/private/tmp/q35-baseline-layer18-attention-sim-20261126.log`, and
`/private/tmp/q35-baseline-layer18-norm-sim-20261126.log`.

An independently audited three-record QKV slice passed against the exact
same normalized input, matching all 8,192 rows within `5.00679016e-06`.
The first 116 workers share its 8,192-row output; the
remaining four workers use separate 32-row outputs. All records were checked
against the original full fixture, and the observed QKV output ranges do not
overlap those other outputs. Log:
`/private/tmp/q35-baseline-layer18-qkv-sim-20261126.log`. No card was accessed.

## Provisional layer-18 discrepancy, 2026-11-26

The active baseline capture has not yet completed its full fleet audit. Its
position-zero snapshots for transitions 15→16, 16→17 and 18→19 through
23→24 passed the original same-input tolerance. Transition 17→18 failed on
2,469 of 2,560 rows, with maximum absolute error `0.00355863571`; the cause
is unknown and the earlier layer-9 threshold explanation must not be assumed
to apply. Hashes and individual outcomes are in
`/private/tmp/q35-provisional-boundaries-15-23-20261126/provisional.json`.

An isolated layer-18 fixture using the exact simulator layer-17 activation
was independently checked against all 120 original worker records, input,
output and zero-initialization ranges. A CPU trace with the exact input
reproduced the untraced layer-18 reference byte for byte. The replay results
are recorded above; no tolerance has been changed.

The existing ttsim checkout explicitly reports the cycle CSR read as
`UnsupportedFunctionality`. A temporary experimental counter-read modification
was discarded once this restriction was confirmed and was not used for any
model run. Building that separate checkout removed the original temporary
library, so it was rebuilt from the unchanged exploratory source; SHA-256 is
`1dbc4a6b269aa743870dcb7be5d89e9c6db142f27140296ea68bc00e41ec6d67`.
All previously running simulator handles remained live. The profiled image
remains blocked from ttsim and from driver packaging; this is not a hardware
timing, output or recovery result. No card was accessed.

## Provisional later-boundary check, 2026-11-26

The active baseline 32-layer capture has not completed its full worker audit;
the following snapshots are provisional. The standalone offline oracle was
run from the exact retained simulator input at each boundary, checking that
both the source snapshot and its oracle replacement remained byte-identical.
Transitions 7→8 and 9→10 passed the existing all-row tolerance, with maximum
absolute errors `1.19209290e-07` and `1.90734863e-06`, respectively. The
8→9 transition failed: 36 of 2,560 rows exceeded the existing tolerance,
with maximum absolute error `2.74516642e-05` at row 1715. The cause is not
established; a prior layer's quantization sensitivity does not explain this
new same-input discrepancy by itself. The unmodified tolerance remains in
force. The checked hashes and results are in
`/private/tmp/q35-provisional-boundaries-7-9-20261126/provisional.json`.

An isolated layer-9 replay fixture was derived from the authenticated
baseline decode graph. An independent audit checked all 120 workers' records,
input/output and recurrent-state zero-initialization ranges, and separation from the weights;
it passed. The replay with the exact captured layer-8 input ran as
session `74537`, log
`/private/tmp/q35-baseline-layer9-chained-sim-20261126b.log`, against the
same-input oracle's layer-9 result. The first launch used an incorrect image
path and exited before loading a fixture; the corrected launch is the one
above. No card was accessed.

The isolated layer-9 replay finished all 16 barriers on all 120 workers and
reproduced the same 36-row numerical failure and maximum error. Its retained
output `/private/tmp/q35-baseline-layer9-chained-actual-20261126.bin` matched
the provisional full-capture layer-9 snapshot byte for byte. This establishes
a reproducible same-input simulator-versus-oracle difference for this layer,
not its cause or a physical-card result.

To localize the layer-9 discrepancy, a second fixture executes only its first
nine records through the attention residual. The independent slice audit also
passed for all 120 workers. A same-input CPU trace produced an
`attn_residual` reference and retained a layer-9 output byte-identical to its
untraced counterpart. The attention-only simulator replay is running as
session `29535`, with log
`/private/tmp/q35-baseline-layer9-attention-sim-20261126.log`. It completed
all nine barriers on 120 workers, reached the intentional record-10 fault,
and passed all 2,560 attention-residual values at the existing tolerance;
maximum absolute error was `1.90734863e-06`. This narrows the strict
whole-layer mismatch to behavior after that point, although a small prior
difference could still affect subsequent quantization.

An 11-record replay through post-attention normalization was also derived;
an independent audit checked its bytes against the original full graph and
the per-worker normalization output layout. This replay is active as session
`84984`, log `/private/tmp/q35-baseline-layer9-postnorm-sim-20261126.log`,
against the same-input oracle's `attn_post_norm` trace. Its result follows.

The 11-record post-attention normalization replay subsequently completed on
all 120 workers and passed all 2,560 values at the existing tolerance, with
maximum absolute error `3.81469727e-06`; record 12 intentionally faulted. A
Q8_K diagnostic comparing this simulated tensor with the original
same-layer-input CPU normalization found two differing block scales but no
differing quantized integers or block sums; for the actual layer-9 Q4_K gate
and up weights, the CPU dot products differed by at most `3.57627869e-07`
and `2.38418579e-07`, respectively. This check alone does not establish the
cause of the final layer mismatch.

The standalone offline oracle now optionally replaces an attention-residual
tensor, reads it back and rejects a missing or repeated replacement. A
self-injection control matched all 32 layers, logits, token and 13 existing
trace tensors byte for byte. Supplying the exact isolated simulator attention
residual at layer 9 still left the simulated final layer outside tolerance:
36 rows differ, maximum absolute error `2.74404883e-05`. Expanded CPU traces
for FFN gate, up and SwiGLU retained all earlier control artifacts byte for
byte; a separate dense-layer trace also matched all 32 original layers,
logits and token byte for byte. These checks are offline only and never
provide inference or fallback to Work mode.

The simulator runner now allows a 9,216-value output only for a marked
isolated layer slice, comparing every value at the same tolerance. Tests
rejected a mismatch in the last row and rejected a wide output without the
explicit mode; the command-line guard also rejected wide mode without a
slice. An independently audited 14-step layer-9 SwiGLU replay uses the same
simulated layer-8 input and the CPU continuation from its exact attention
residual. It is running as session `82534`; log
`/private/tmp/q35-baseline-layer9-swiglu-sim-20261126.log`. The result is not
yet available at that point and no physical card was accessed.

The 14-step SwiGLU replay subsequently passed all 9,216 values on 120
simulated workers, maximum absolute error `2.38418579e-07`; the next record
intentionally faulted. Comparing its actual activation to the same-residual
CPU activation with the GGML Q8_K quantizer found 34 different block scales,
one different quantized integer and one different block sum. Applying the
real layer-9 Q6_K down weights gave maximum dot-product difference
`2.74332124e-05`; keeping the original quantized integers and sums while
using the changed scales gave only `1.19209290e-07`. This closely matches
the previously observed final-layer discrepancy.

As an independent same-input check, the offline CPU oracle can now replace
the full 9,216-value SwiGLU tensor. Self-injection preserved all 32 layers,
logits, token and 16 trace tensors byte for byte. It rejected short, long,
non-finite and symlink inputs and an out-of-range layer. When injected with
the exact simulated SwiGLU, the CPU down projection plus the exact simulated
attention residual matched the isolated simulator's final layer to maximum
absolute error `1.19209290e-07`. The injected trace was read back and matched
the original simulator bytes. This supports a quantization-threshold cause
for this particular input; the existing strict comparison is not weakened,
and hardware and other layers or positions remain unqualified.

The still-active baseline full-model capture has also produced provisional
position-zero transitions 10→11 through 14→15. All five passed the original
same-input tolerance; the largest absolute error was `1.54972076e-05`.
Hashes and individual outcomes are in
`/private/tmp/q35-provisional-boundaries-10-14-20261126/provisional.json`.
These captures remain provisional until the complete fleet audit succeeds.

The paired FFN hybrid full-decode simulation completed its first decode and
selected the independently expected token `261`, but exited with failure on
the final hidden-state comparison: its first reported row was
`-2.2534914016723633` versus `-2.3027868270874023`. It therefore did not run
the second decode. Its log is
`/private/tmp/q35-ffn-pair-two-decodes-sim-20261126.log`. Matching the token
does not make the failed full-model hidden-state check a pass.

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

The first four-layer capture control exited 1 after missing the layer-0 safe
window (`/private/tmp/q35-baseline-four-layer-snapshots-20261126.log`); it
produced no snapshot and no completion marker. The runner now shortens the
simulator clock chunks approaching a boundary, while preserving the same
overall simulated-cycle deadline and rejecting any missed window. Synthetic
checks cover the clock transitions and malformed boundaries. A new control
is running as session `96447` with log
`/private/tmp/q35-baseline-four-layer-snapshots-fine-20261126.log`. The
revised method is not yet validated against the layer references.

The revised four-layer control captured its first provisional output after
the previous barrier at root progress `0x51350089`; its layer-0 maximum
absolute difference from the reference is `8.94069672e-08`. The run is still
active, so this is not a completed capture validation. In parallel, the same
instrumentation is running the previously failing eight-layer prefix in
session `50479`, log
`/private/tmp/q35-baseline-eight-layer-snapshots-fine-20261126.log`. Its
snapshot outputs are provisional until the expected terminal fault is audited
and should then be compared with the control and independent reference.

An isolated layer-4 replay using the independent layer-3 input and explicitly
zeroed position-zero state also started in session `80579`, after the
120-worker record, state-range and disjointness audit passed. Its log is
`/private/tmp/q35-baseline-layer4-slice-sim-20261126.log`; no result is
available yet. It will complement the passing isolated layer-5 replay and
the still-running sequential six-layer comparison.

### Six- and sixteen-layer outcomes and layer-7 sensitivity (2026-11-26)

The sequential six-layer baseline completed 98 records on 120 workers and
then intentionally faulted on record 99; all 2,560 values passed the reference
comparison, maximum absolute error `5.00679016e-06`. Its actual output is
`/private/tmp/q35-baseline-6-layer-observed-20261126.bin`. The sequential
16-layer run reached its terminal comparison but failed row 0:
`0.6294565200805664` versus `0.6175908446311951`
(`/private/tmp/q35-baseline-16-layer-sim-20261126.log`); that older runner
reported only the first mismatch and did not export the tensor.

The isolated layer-6 and layer-7 replays each passed with independent
reference inputs and zeroed state where applicable: maximum absolute errors
`1.78813934e-06` and `1.07288361e-06`, respectively. Replaying layer 6 with
the actual six-layer simulator output as input also passed against the CPU
oracle continued from that same input, maximum absolute error `9.53674316e-06`.
The audit verified the 120 worker records and their protected ranges before
launch. Logs are `/private/tmp/q35-baseline-layer{6,7}-slice-sim-20261126.log`
and `/private/tmp/q35-baseline-layer6-chained-sim-20261126.log`. The layer-7
replay using the chained layer-6 output is running as session `53132`.

The chained layer-7 replay subsequently passed against the independently
continued CPU oracle, with maximum absolute error `8.94069672e-07`; all 120
workers completed 17 records before the deliberate record-18 fault. Its
complete output was byte-identical to the prior sequential eight-layer
simulator output. The log is
`/private/tmp/q35-baseline-layer7-chained-sim-20261126.log`, and actual output
is `/private/tmp/q35-baseline-layer7-chained-actual-20261126.bin`. Together
with the Q8_K counterfactual below, this confirms the observed layer-7
difference for this position-zero input as numerical sensitivity, rather
than a discrepancy between this isolated layer and its same-input oracle.
It does not validate the remaining layers, later positions or physical card.

The intermediate capture now supports one full position-zero decode: its
120-worker records were checked for all 31 intermediate boundaries in the
baseline, hybrid and paired fixtures. A baseline full capture is active as
session `75817` (`/private/tmp/q35-baseline-full-layer-snapshots-20261126.log`).
For each completed capture, the new standalone batch oracle reinjects each
simulated boundary into the authenticated CPU-only reference and compares the
next simulated layer with that same-input continuation. It verifies fixture
command token 42 and position zero, validates all worker boundary records,
records hashes, refuses incomplete or changing snapshots, and requires a
separately audited candidate token for a full decode. The four-layer control
passed all four checks, maximum errors `8.94069672e-08`, `2.38418579e-07`,
`9.53674316e-07` and `8.58306885e-06`; see
`/private/tmp/q35-four-layer-same-input-audit-v3-20261126.log`. A mock-oracle
test also confirmed that a bad layer yields a nonzero exit and that an
incomplete capture is rejected. This is diagnostic evidence only; it does not
test Work inference, other tokens or positions, hardware recovery or speed.

Replacing the standalone CPU oracle's layer-6 boundary with the actual
chained layer-6 output produced a layer-7 output within `8.94069672e-07`
maximum absolute error of the previously retained sequential eight-layer
simulator output. A same-input CPU control was byte-identical to the
original CPU reference. In the two CPU runs, the traced layer-7 normalized
input differed by at most `1.52587891e-05` and its Q projection by
`0.000762939453`; traced runs were byte-identical to their untraced
equivalents. Using the CPU GGML Q8_K quantizer and real layer-7 Q4_K Q weights,
the inputs differed in one quantized integer and one block sum (and ten
scales). Actual dot products differed by at most `0.000762701035`, whereas
keeping the original integers/sums while changing the scales differed by at
most `2.86102295e-06`. This supports a quantization threshold as the cause of
the observed layer-7 jump in these specific continuations. The evidence does
not by itself prove all later layers, tokens, physical execution or hardware
performance. Artifacts are `/private/tmp/q35-oracle-layer7-trace-*-20261126*`.

The four-layer intermediate-snapshot control completed and all captured
layers matched the independent reference, with maximum absolute differences
`8.94069672e-08`, `2.38418579e-07`, `7.15255737e-07`, and
`5.00679016e-06`. The instrumented eight-layer run is still active. The
hybrid full-decode simulation selected reference token `261` and reported the
same row-0 mismatch as the baseline (`-2.2534914016723633` versus
`-2.3027868270874023`); it exited 1 before the second decode. The simulator
runner now offers an explicitly diagnostic continuation only for finite
hidden-state mismatches and only when both reference tokens are provided. It
still refuses a wrong token or non-finite value and exits failure if any
hidden-state comparison fails. A two-decode diagnostic started in session
`73034`; no result exists yet. This option never qualifies a model or runs
in Work mode. All these experiments are off-card.

The instrumented eight-layer run subsequently completed its expected
all-worker terminal fault and retained all eight snapshots. Its final tensor
was byte-identical to the earlier sequential eight-layer output. The ordinary
comparison passed layers 0 through 6 and failed layer 7, as before. The
same-input batch oracle then independently passed layer 0 and all seven
subsequent boundary checks; the largest maximum absolute error was
`9.53674316e-06`. The result is logged at
`/private/tmp/q35-eight-layer-same-input-audit-20261126.log`. This verifies the
eight position-zero layer transitions in the exploratory simulator using
same-input references; it is not a 32-layer, warm-position, hardware, recovery
or performance result.

The isolated layer-4 replay subsequently completed 16 records on 120 workers,
then intentionally faulted at record 17. All 2,560 outputs matched the
independent reference with maximum absolute error `4.76837158e-07`
(`/private/tmp/q35-baseline-layer4-slice-sim-20261126.log`). This uses the
reference layer-3 input and cleared position-zero state. It is not the
sequential six-layer result, a physical-card result or a full-model pass.

An offline CPU-oracle continuation from that isolated simulated layer-4
output remained close at layer 7: maximum absolute difference
`1.07288361e-06`, compared with the independently observed eight-layer
simulator difference `0.00240516663`. Its maximum difference later reached
`0.370859146` at layer 31; both runs selected token `261`. Replacing layer 4
with the original CPU reference gave byte-identical layers, logits and token.
This shows that this isolated layer-4 perturbation alone does not reproduce
the eight-layer discrepancy; it does not rule out a different perturbation
from the sequential earlier layers or an error in a later simulated layer.
Artifacts are `/private/tmp/q35-oracle-layer4-slice-{actual,control}-20261126*`.

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
