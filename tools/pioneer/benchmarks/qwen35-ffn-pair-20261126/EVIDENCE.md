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
