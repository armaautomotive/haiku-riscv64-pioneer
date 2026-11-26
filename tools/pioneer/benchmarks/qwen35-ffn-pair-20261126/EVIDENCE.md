# Paired FFN gate/up graph experiment — 2026-11-26

Under the guarded non-loadable `PIONEER_QWEN35_FFN_PAIR_COMPILE_ONLY`
configuration, each layer builds its independent gate and up matrix
projections from the same normalized input with 60 workers each. Their
destinations are independently authorized and must be disjoint; the existing
paired-projection recorder emits one shared step for the 120 invocations.
The remaining FFN operations are unchanged. Failures in either build or the
paired step stop the sequence without running SiLU or the down projection.
The hybrid 32-layer prompt and decode graph counts change from 529/532 to
497/500. This reduces recorded barriers; it is not evidence of a speedup.

The ASan/UBSan graph test recorded complete synthetic 32-layer prompt and
decode graphs, validated each of the 32 paired steps contains 60 gate and 60
up assignments, validated all stream records, and patched every record to
the next token position. The sequence test covered both build failures and
the paired dispatch failure. The previous unpaired sequence regression
passed. A Haiku/RV64 compile of the paired sequence test passed with warnings
as errors. Build guards reject unsupported and qualification configurations.
The full suite started before a final graph-test-only dynamic patch check was
added; the updated graph test was independently rebuilt and passed afterward.

No model numerics have been executed for the paired graph. Image build and
fixture generation, compiled resident execution, physical ownership and
recovery, and hardware performance remain unverified. Existing running
simulations use their earlier images and do not test this change. No card was
accessed, and no decode tokens/sec was measured.

## Compiled paired simulator image and authenticated fixture

The service and resident-image builders accept the paired flag only with the
existing simulator-only hybrid prerequisites and carry a distinct
`ffn-pair.marker`. Both driver-header packagers independently reject that
marker. The simulator runner refuses mismatched paired image/fixture markers
before loading its library, and its fresh decode-ticket helper recognizes
the paired graph length. Negative marker and handoff tests passed. The
ordinary unpaired hybrid resident boot, text, dispatch and flash binaries
rebuilt byte-for-byte against the earlier image.

The paired RV32 device and resident images built without unresolved imports.
A new authenticated 4B four-layer fixture stages 289,628,160 bytes and
contains 63 completed records plus the intentional fault record. A simulator
process is running that fixture against an independently produced 2,560-value
layer-three reference (`/private/tmp/q35-ffn-pair-recorded-four-layer-sim-20261126.log`).
The process has not completed at this checkpoint; do not infer numerical
correctness or speed. The flash suite, header-packaging guards, Python syntax,
state-handoff and image/fixture mismatch tests passed. Physical authority,
processor release, no-reboot recovery, full-model output and throughput
remain unverified.

## Full decode fixture and second-token patch audit

The paired graph recorder generated an authenticated 32-layer, 500-record
per-worker decode fixture at
`/private/tmp/q35-ffn-pair-recorded-full-decode-20261126`. Its
2,740,473,856 staged weight bytes compare byte-for-byte equal to the earlier
authenticated hybrid full-decode fixture. The existing handoff verifier now
uses the configured graph length and checks hybrid producer/reducer position
patches as well as the dense pack records. Under ASan/UBSan it checked the
actual fresh ticket and all 60,000 records, including 120 embedding, 2,880
convolution, 5,760 delta, 1,920 dense and 1,920 hybrid patches; all other
records remained byte-identical. Baseline 524-step and unpaired hybrid
532-step fixture regressions also passed. Separately corrupted copies of the
first record and the command commit were rejected. This verifies recorded metadata and
state-address transformations, not numerical model execution.

The paired four-layer numerical simulation remains running; the paired full
fixture has not been run. Physical ownership, recovery and throughput remain
unverified. No card was accessed.

## Four-layer numerical result and full two-decode run

The paired RV32 simulator image and authenticated four-layer fixture completed
all 63 intended graph records across 120 simulated workers and deliberately
faulted at record 64. All 2,560 final-layer values matched the independent
llama.cpp reference, with maximum absolute error `5.00679016e-06`
(`/private/tmp/q35-ffn-pair-recorded-four-layer-sim-20261126.log`). The
simulator is exploratory and unqualified; the CPU reference is offline
validation, not Work inference.

At the time this checkpoint was first recorded, the full 500-record paired
decode fixture had started a two-consecutive-token run on one simulated fleet
with separate first- and second-position reference files
(`/private/tmp/q35-ffn-pair-two-decodes-sim-20261126.log`, session `88296`).
The final outcome is recorded below. This simulation does not establish
physical execution, firmware/NoC ownership, no-reboot recovery, clean unload
or hardware throughput.

## Full paired-FFN simulator outcome

The simulator completed its first 500-record decode graph and selected token
261, matching the supplied token reference. It then failed the final-hidden-
vector comparison at row 0: simulated `-2.2534914016723633` versus reference
`-2.3027868270874023`. The run stopped there; it does not show that a second
decode position ran, and it is not a valid warm run. The simulator's reported
9,038 seconds are simulated clock time, not P100A timing. Full log:
`/private/tmp/q35-ffn-pair-two-decodes-sim-20261126.log`.

This failure belongs to the paired-FFN variant and does not change the
separate dense K/V paired four-layer pass recorded in
`benchmarks/qwen35-compact-resident-20261126/EVIDENCE.md`. Full dense K/V
simulation, physical ownership/recovery, warm outputs and hardware throughput
remain unverified.
