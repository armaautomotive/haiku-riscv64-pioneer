# Compact resident simulator image, 2026-11-26

## Haiku native compiler build of paired dense K/V variant (2026-11-26)

The source closure from commit `e154a69362` (archive SHA-256
`82e89fb8bc91c85fb8868450e7efdffc8f30964f404dcd6ef94f0be9d92fac8`) was
compiled on the Pioneer with its installed Haiku RISC-V GCC and binutils. The
RV32IMF/ILP32 compile and link used the rewrite builder's freestanding `-Oz`
flags and the `PIONEER_QWEN35_DENSE_KV_PAIR_COMPILE_ONLY` guard. Haiku does not
have Python installed, so the builder's compile/link commands were reproduced
directly; its stack-envelope checker was extracted unchanged and run locally
against the generated GCC `.su` reports.

The service linked without unresolved imports, used the expected boot jump,
contained its required entry/graph/projection symbols and calls, and passed
the atomic and unsupported-FPU instruction scan. Boot/text/dispatch sizes are
4/56,428/37,472 bytes; SHA-256 values are `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b`,
`829628b0991e9d86702da3645d5bd526b5ceb31009eaddfd7fd569dab6911f11`, and
`58839e67c2992edbf138654ede2116f061a500bac446ee38aa6f5e43b51b872f`.
Compiler stack envelopes, including the existing 1,024-byte allowance, are
7,648/8,192 bytes for the resident graph, 5,728/8,192 for fused RMS/save,
6,288/8,192 for hybrid flash, 5,120/8,192 for delta shard, 3,232/8,192 for
settlement, and 2,832/8,192 for compact root. These are compiler estimates,
not whole-device stack proofs.

This is a non-loadable compile-only witness. No driver, model, accelerator,
simulator inference, or benchmark was run. The paired stream still lacks its
matching numeric fixture, so full-model correctness and decode throughput
remain unverified.

## Paired dense K/V four-layer simulator pass (2026-11-26)

The fixture recorder compiled on Haiku and hashed the pinned Qwen3.5-4B
Q4_K_M file before constructing the version-7 four-layer fixture. Its layout
is `C 4 54 55 0`: 54 model records execute, followed by the intentional fault
record. It stages 289,628,160 model-weight bytes. The matching compile-only
image and fixture both carry the `dense-kv-pair.marker`; the simulator runner
now refuses a missing or mismatched marker and admits this matching pair.

The exploratory matrix simulator (`libttsim.so` SHA-256
`1dbc4a6b269aa743870dcb7be5d89e9c6db142f27140296ea68bc00e41ec6d67`) ran the
54-step graph and completed the worker audit. All 2,560 final values matched
the previously saved four-layer output bit-for-bit (maximum difference 0);
the four layer snapshots were captured after their barriers. That prior
output passed the independent llama.cpp four-layer comparison with maximum
absolute error `5.00679016e-06`, as recorded below. The paired run log is
`/private/tmp/q35-dense-kv-pair-four-layer-sim-patched-20261126.log`; its
reference tensor SHA-256 is
`23e2d5d9e1753671df264bea4a50fbd1eed1dd53d9af5ca836612f7ae952d563`.

The image section hashes are boot `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b`,
text `829628b0991e9d86702da3645d5bd526b5ceb31009eaddfd7fd569dab6911f11`,
and dispatch `58839e67c2992edbf138654ede2116f061a500bac446ee38aa6f5e43b51b872f`.
The command/templates/records/weights hashes are respectively
`d8cfef8d83fadcc178cb7e77be46bb5ca6779b56cd7fce2c749f287e3b41b33d`,
`e052f18f9bdb7f3569fd0104bba14989487f55077bd182c5ec2b681d4f40c7d2`,
`0048b5d25bbc503e49ac804614e092e8b560c287a7b3bffddfc71d13bde0c116`, and
`e1a6c306c260268ba3164a91020a9c657ee8e254a6e59644653d8c47e1b87997`.
The current reader/writer role image directory was
`/private/tmp/q35-current-shared-roles-20261126h`. Reader shim/code hashes are
`94457371ed8a2e2985db8fdd664741597f2f740b85aede7ce72933ab9486f0d1` and
`58adee1e53441f5a0adf275fb8ee5776d534279bd23e7188da4ad1e3d1fe15d7`;
writer shim/code hashes are
`066ef535345f43e98794b1d64a9f20f2d6a5f753491b5db95c40c3070c15fbb6` and
`6fbbd6435aa4b55e97312b4e5740d9f73bd2fa264c46cf2605cd5a698f30678d`.

This is a four-layer simulator result only. The full 32-layer graph, warm
outputs, physical ownership/recovery, clean unload and decode throughput
remain unverified. The patched simulator clock is not P100A throughput.

## Combined 32-layer paired graph fixture and simulation (2026-11-26)

The local 4B model file has the pinned SHA-256
`00fe7986ff5f6b463e62455821146049db6f9313603938a70800d1fb69ef11a4` and
size 2,740,937,888 bytes. The matching compile-only image at
`/private/tmp/qwen35-dense-kv-pair-e154a69362/image` carries the dense K/V,
FFN pair, fused RMS/save, hybrid flash, and compact two-region markers. A
full authenticated fixture was generated locally with the same graph flags;
its layout is `C 32 425 428 1`, and it stages 2,740,473,856 weight bytes.
Fixture hashes are command
`fb2521fb1c97674c7af14ac63b08af934b404cd301951a78f50c47035961f7e8`, templates
`41bb5d74e31dbc4e367b28212a275cb4d530df53d1cc3316f4184fdac014438c`, records
`6a903aaa5a9ef0e175c9053bab18dc5aba3993ca1190eda9a7a4cdee633e5a7c`, and
weights `591910f9cb91abd0392ac0efbb86c0b4c09f12e32a7bb4315b48972d23752be6`.

The first full simulator launch exposed a stale runner length guard: it
expected 436 decode steps instead of the paired graph's 428. The runner now
uses 425/428 for the matched dense K/V variant, with explicit tests retaining
the other graph lengths (521/524, 529/532, 497/500, and 433/436). The focused
marker/count test passes. The full 428-step simulation is running on the
exploratory matrix simulator with all 32 layer snapshots requested. At poll
2,650, it had captured layers 0–7; each of their 2,560 F32 values is
byte-identical to the earlier full simulator output. This includes both dense
attention layers 3 and 7. Their snapshot hashes
`23e2d5d9e1753671df264bea4a50fbd1eed1dd53d9af5ca836612f7ae952d563` and
`84647096a90db77bafe61c06c8877789afdd78d8ab1a73d79b037c98d013bd6a` match
the baseline captures. This establishes graph equivalence through the first
two paired K/V operations, not independent full-model CPU correctness. The
full decode has not completed. This is simulator-only, not a physical run or
throughput measurement.

## Paired dense key/value recorder schedule, 2026-11-26

The compile-only rewrite adds a distinct stream version 7. Its recorded graph
uses 428 decode steps and 425 prompt steps, saving one step for each of the
eight dense layers. The actual graph recorder test builds prompt and decode
schedules and checks that every dense key/value pair writes to disjoint
scratch, with 60 active projection workers for each matrix, before separate
KV packing records. It also checks all 120 stream tickets through the dynamic
patcher, warm-position recording, preservation of the original candidate
state, and rollback after a partial stream write. The ASan/UBSan test passes.
The same test executes the actual dense-attention worker template for
PackValue against mock storage, checks all 128 packed half-precision lanes,
and confirms the Up/context alias is rejected for key packing and attention
run. This validates the worker's pack operation, not NoC access or projection
results.

This is a schedule/metadata test, not numerical projection or attention
execution. Both service-header packagers reject the
`dense-kv-pair.marker`. The generic simulator runner accepts the variant only
when the matching recorded fixture carries the same marker; the four-layer
numerical result is recorded above. The target compiler is not installed in
the workspace; the Haiku-hosted RISC-V compiler built and linked the
compile-only variant. No accelerator was accessed and no decode speed was
measured.

## Recorded projection traffic and worker use (2026-11-26)

The auditor reads the actual 436-step compact decode fixture, validates all
120 installed descriptors and every recorded worker command, checks each
projection invocation and rejects overlapping weight segments in the same
step. Its 169 projection steps request `2,734,387,200` weight bytes in total.
The seven banks receive 390,368,768 to 390,769,664 bytes each over the decode.
Of those projection steps, 153 assign weight work to all 120 workers; the
other 16 assign it to 64 workers. These are the key and value projections in
eight dense layers, which currently share an output scratch region and run in
separate steps. The other workers participate in the fleet protocol, but do
not execute the matrix projection on those steps. This does not yet meet the
user's request to use all cores for those projections.

For planning only, 100 tokens/sec would require `273.44 GB/s` of projection
weight requests alone if all recorded weights are fetched for each token.
Tenstorrent specifies 448 GB/s peak memory bandwidth for P100A:
https://docs.tenstorrent.com/aibs/blackhole/installation.html . An idealized
calculation assuming seven equal independent 64 GB/s banks and no overlap
between graph steps gives `405,715,968 / 64e9 = 6.34 ms` per token for these
weight reads, leaving 3.66 ms of the 10 ms target for everything else. This
assumption is not a measured bank capability, and the calculation omits
embeddings, activations, state, non-projection work, barriers and overhead;
it is neither a throughput measurement nor a guarantee of attainability.

The evidence log and fixture/tool hashes are `projection-traffic-20261126.log`
and `projection-traffic-20261126.json`. The overlap checker passed ASan/UBSan;
corrupt template and truncated record controls were rejected. No card was
accessed. A direct next rewrite opportunity is to give the independent key
and value projections disjoint scratch and schedule them together across the
fleet, then prove state ordering and numerical equivalence before using it.


## Resident compiler stack-chain evidence (2026-11-26)

The compact builder can optionally emit GCC stack call graphs. Enabling this
option reproduced the exact prior normal-counter `service.elf`, boot, text
and dispatch images byte-for-byte. Merging the seven compilation-unit graphs
and checking the linked symbol table yields a longest known static chain of
4,656 bytes from `qwen35_resident_graph_service_main`, with no unresolved,
dynamic or cyclic nodes reported by the compiler-graph audit. Its detailed
chain and hashes are `resident-compiler-stack-audit-20261126.json`. The
existing broader stack-envelope check with its separate allowance also passed.

This is compiler evidence, not a whole-device stack proof: assembly entry,
interrupts, linker-selected weak definitions, actual L1 layout and physical
stack high-water behavior are not established by the graph. It does not grant
hardware ownership, recovery, clean unload or throughput. The merge regression
rejects conflicting compiler definitions; the shared graph-auditor tests
cover unresolved, dynamic and recursive cases.


## Exact resident-image startup and bounded graph probe, 2026-11-26

The exact normal-counter compact resident image linked into the non-loadable
Haiku driver started and stopped on all 120 simulated workers with the
matching current-source reader and writer roles. Corrupting one template
produced the expected retained fault on worker 73 while the other workers
stopped. This used the exploratory patched simulator and executed no model
graph or profiling CSR. Image, role, template and log hashes are in
`resident-normal-start-stop-20261126.json`; the two logs are saved alongside
it. This is not physical launch, no-reboot recovery, or throughput evidence.

A separately guarded normal-counter graph probe admits only the fixed
four-layer, 55-executed-step fixture and verifies each of the 120 template
tickets and the command have count 433, which does not enable the image's full-decode
profiling path. Tests reject profiling-enabling tickets, warm mode, mixed
probe modes and missing guards. The exact driver-linked resident image completed all 55 four-layer graph
steps on 120 simulated workers and passed all 2,560 reference values; the
maximum absolute error was `5.00679016e-06`. Record 56 intentionally
faulted. The run started before the additional command-count guard, and its
saved command was independently checked to have count 433, token 42 and
position zero. The log and image hashes are saved in
`resident-normal-four-layer-20261126.log` and
`resident-normal-four-layer-20261126.json`. This is an exploratory patched
simulator run without profiling, not a clean driver unload, physical recovery,
full-model result or throughput test.

## Exact qualification-image projection probe, 2026-11-26

An explicit standalone-only simulator probe ran the exact normal-counter
qualification text and dispatch previously linked into the non-loadable
Haiku driver. For each of Q4_K, Q5_K and Q6_K it completed two model-weight
projection commands, compared all 1,920 values per command, and observed all
120 simulated workers and reader/writer roles stop. The maximum normalized
errors matched the unprofiled results recorded below. Image and log hashes
are in `standalone-qualification-sim-20261126.json`; the three raw logs are in
`standalone-qualification-sim-20261126/normal-counter-q*.log`.

The explicit probe admits only the marked normal-counter standalone image
with a projection fixture; ordinary simulator runs continue to reject it.
Tests reject missing opt-in, wrong markers, resident templates and incompatible
flags. The projection path did not execute the graph-profiling cycle CSR
instructions, which this exploratory simulator does not support. The
simulator itself contains exploratory matrix patches; neither its elapsed time
nor this result is physical recovery, inference or throughput evidence.

## Standalone qualification projection in simulation, 2026-11-26

A separately marked, unprofiled simulator-only build of the standalone
compact qualification service completed two model-weight projections each
for Q4_K, Q5_K and Q6_K. Each run checked all 1,920 outputs independently,
and all 120 simulated compute workers and reader/writer roles started and
stopped. Maximum error relative to each row's sum of absolute products was
`1.57298922e-07`, `1.21575004e-07` and `8.09506194e-08`, respectively.
Model weight blocks were checked against the pinned 4B tensor hashes. The
two commands used different signed input data. Image, role, fixture and
exploratory simulator hashes and measured errors are in
`standalone-qualification-sim-20261126.json`; output logs are in
`standalone-qualification-sim-20261126/`. Their generic runner label says
"resident" because the service stayed up for both commands; the entry point
used here is the standalone qualification service, not the resident graph.

The first attempt used an older role image and exited on an unsupported NoC
register read. A second role build failed its startup mailbox check. The
successful runs used the same role bytes as the running full-model simulator;
a fresh build of the current role sources reproduced all four files exactly.
The unprofiled service is a separate image from the normal-counter service in
the driver compile check. It is restricted to a separately marked simulator
mode, and compile-header tests reject it. The exploratory simulator contains
matrix patches. These checks do not establish behavior of the normal-counter
image, physical inference, recovery, or throughput.

## Matching compact qualification image, 2026-11-26

The compact image builder can now build a standalone qualification service
from the same normal-counter graph, compute, stream and barrier sources as
the compact resident service. The six shared objects were byte-identical
between independent qualification and resident builds. The qualification
image uses the standalone entry and stopped-park symbol; it is not the
resident service and remains explicitly non-loadable. Two qualification
builds produced the same boot, text and dispatch bytes. Text is 53,744 bytes
at `0x72000`; dispatch is 33,276 bytes at `0xa0400`. The conservative worker
stack envelope is `7088/8192` including a 1,024-byte allowance; it is not a
whole-program stack proof.

A separate compile-only header generator checks the qualification marker,
entry, stopped park, bounds, all required feature flags and forbidden build
flags. Its tests reject swapped image types, mixed entry points, absent flags,
simulator variants and an unprofiled qualification build. The ordinary
resident and qualification packagers still reject compact images. The
existing resident image rebuilt byte-for-byte, and its previous header tests
passed.

The complete fused Haiku/RISC-V driver compiled and linked with no unresolved
symbols using both actual compact resident and matching qualification images.
Each image's text and dispatch appeared once in both the driver object and
linked artifact. The linked `init_hardware` instructions remained identical
to the no-I/O `B_NOT_ALLOWED` compile-only guard. Both temporary artifacts
were deleted without installation; this removes the earlier qualification
placeholder from this build check, but does not establish qualification image
behavior in simulation, physical ownership, recovery or throughput.

Qualification hashes: boot `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b`,
text `0f2e107c0c1fb29ecc11678afd9df9f11aec96bf28cb5ec14c1bf74b70791b1c`,
dispatch `b4cc9f91a675d2465e7f03d99ac7010aff5db7c5265bd95a61654f9a610e15ee`.

## Full compact simulator result, 2026-11-26

The unprofiled, size-optimized two-region image completed all 436 recorded
steps and the 120-worker and candidate audits. It selected reference token
261, but failed the strict fixed-reference final hidden-state comparison on
2558 of 2560 values, maximum absolute error `0.370739102`; the process exited
with failure. All 32 audited snapshots were byte-identical to the previously
completed three-region fused run; their per-layer hashes are in
`compact-full-layer-comparison-20261126.json`. The earlier same-input audit
of those identical boundaries passed 30/32 checks. This does not validate
warm decode, physical inference, recovery, or throughput. The normal-counter
image used for the guarded driver link is a separate image and was not this
simulator run. Log: `/private/tmp/q35-fused-compact-full-layer-snapshots-20261126.log`.

## Non-loadable compact driver link, 2026-11-26

The complete fused tree driver and PCI profile compiled and linked against
the Haiku/RISC-V kernel with `--no-undefined`, using the actual compact
resident text and dispatch. The temporary linked driver contained exactly one
copy of each image, and its `init_hardware` disassembly matched the object
checked to return `B_NOT_ALLOWED` without I/O. The separate qualification
image remains an older compilation placeholder. The temporary driver was
deleted without installation or card access. Log:
`/private/tmp/q35-actual-compact-driver-link-final-20261126.log`.

The normal-counter image is still compile-only and has not passed simulator
or hardware inference. This link does not establish physical ownership,
no-reboot recovery, clean unload or throughput. The existing image packagers
continue to reject compact images.

## Actual compact image in a driver compile, 2026-11-26

A test-only header generator now supplies the actual mixed-optimization,
normal-cycle-counter compact resident image to the complete fused tree Haiku
driver object compile. The object contained exactly one copy each of the
resident text (SHA-256
`c15d1ed9048d92cf1d0d7b077b0550176e10f52213e615a70c2ca047cb2df9d6`)
and dispatch (`4deb2c1466880fcf9045832d0702933239b90f18bcdd4a0cc31f21e614471ad2`).
The old qualification image header remains a compilation placeholder. That
first object was not linked or installed; its compiled `init_hardware` was checked
to return `B_NOT_ALLOWED` without I/O. Log:
`/private/tmp/q35-actual-compact-driver-compile-final-20261126.log`.

The temporary header requires the compact compile-only flag and all matching
resident graph guards, rejects qualification, trial, dense-shard and
unprofiled simulator modes, and is used only by the standalone object-compile
test. Tests reject missing markers and flags, incompatible markers, a third
code region, invalid bounds and an invalid park instruction. The existing
resident and qualification packagers still reject compact images. This is
compile and byte-identity evidence, not a linkable driver, simulator result
for the cycle-counter image, physical launch, recovery or throughput.

## Fused resident driver compilation, 2026-11-26

Compiling the complete Haiku driver translation unit with the fused tree
graph exposed old host-per-operation sequence handlers that tried to call
operations unavailable in the resident graph. In the fused compile-only
configuration, the FFN, recurrent-layer, dense-layer and decoder-sequence
handlers now return `B_NOT_ALLOWED`; generation remains on the resident
graph path. Ordinary configurations retain the existing handlers.

The standard configuration passed a Haiku/RISC-V syntax check and the fused
configuration compiled to a Haiku/RISC-V object with warnings as errors.
The initial compile used previously generated image headers as placeholders;
the newer actual-compact compile is described above. The compile-only guard
remains active. A focused test compiled and called all four fused-build
handlers and confirmed rejection. Reproduce the current compile checks with
`tests/run_pioneer_qwen35_fused_driver_compile_test.sh BUILD WITNESS_IMAGES COMPACT_IMAGE`;
the temporary object is deleted.

## Higher-level resident ownership checks, 2026-11-26

The service-owner simulation fixture now models both activation-sharing L1
packets. They start nonzero, must be cleared and read back before any worker
starts, and remain part of the existing held-fleet checks. The earlier fixture
did not model these addresses, so using it with the fused graph correctly
failed startup; this was a fixture limitation, not a device result.

The real template installer and held-service setup also passed with the fused
tree configuration against fake per-core MMIO, including their existing
failure cases. The reusable test is
`tests/pioneer_qwen35_fused_resident_templates_test.py`. A fused tree resident
model host probe cross-compiled for Haiku/RISC-V. Both checks are now in the
dataflow target suite.

With the fused graph and tree settlement selected, the actual resident-owned
runner and driver owner passed their simulated tests. Additional tests using
explicitly simulated permission covered reuse of a transport and two authority
objects across tokens, retention after failed close, and retirement only with
matching simulated recovery evidence. Ordinary and paired controls passed as
well; see `/private/tmp/q35-resident-owned-modes-final-20261126.log`,
`/private/tmp/q35-resident-owner-modes-final-20261126.log`, and
`/private/tmp/q35-resident-{cache,recovery,close_fault}-tree-fused-20261126.log`.
The broader target suite exited zero (`/private/tmp/q35-fused-metadata-target-suite-20261126.log`),
but started before these fixture and test edits, so it does not by itself
validate the final source state. The affected tests were rerun separately.
These tests use
fake MMIO and simulated authority; they do not qualify real firmware exclusion,
physical owner lifetimes, launch, recovery or throughput. No card was accessed.

## Resident host metadata integration, 2026-11-26

The resident host metadata previously could not prepare the fused graph: it
had no fused RMS/save operation and counted flash dense attention's producer
and reducer as one step. It now validates and applies the fused residual and
RMS authority transitions as one step, and counts both dense-attention
records. A malformed fused pair is rejected without mutating the live state.

The actual metadata state matched the independently recorded graph for both
prompt and decode at positions zero and one, including all 64 fused RMS/save
operations and 32 paired FFN steps. Resident host transaction and session
tests passed under ASan/UBSan in ordinary, paired, hybrid and fused modes,
including prompt/decode promotion and retained-failure scenarios. A fused
resident-model host probe cross-compiled for Haiku/RISC-V. These are offline
tests; the compact image remains non-packageable, physical ownership and
no-reboot recovery remain unqualified, and no throughput was measured.

The mixed-optimization two-region image also completed the four-layer
simulation: all 120 worker receipts were checked, the next record faulted as
intended, and all 2,560 output values matched the reference within a maximum
absolute error of `5.00679016e-06`. Its output was byte-for-byte identical to
the size-optimized compact image. Log:
`/private/tmp/q35-fused-compact-mixed-four-layer-sim-20261126.log`. This is
simulator evidence only, not a full-model or hardware performance result.

The fused hybrid resident image previously used a third code region at
`0xd0000`, overlapping the old ROOT scratch. This simulator-only candidate
compiles the same selected sources with GCC `-Oz` and places all worker code
in the existing primary and dispatch regions. The primary text is 56,152
bytes at `0x72000` (limit `0x80000`); dispatch is 37,608 bytes at `0xa0400`
(limit `0xaa000`). The latter uses more than the older linker's `0x7000`
limit, but remains within the service owner's declared `0x9c00` maximum.
The existing PCI snapshot view describes `0x6e000..0xb0000`; those bounds
alone do not prove physical ownership, write exclusion or recovery.

The repeatable builder is `build_qwen35_compact_resident_ttsim.py`; it emits
explicit simulator-only markers before compiling and no driver header. Both
driver-header packagers reject the compact marker independently of the other
simulator-only markers. The simulator requires an explicit matching compact
flag, the fused hybrid markers and absence of an extra code image. Negative
tests reject marker/flag disagreement and an extra code image before loading
the simulator. A preexisting output directory is refused.

The build had no unresolved imports or forbidden RV32 atomics/divides, and
all 244 code symbols lay within the boot, text or dispatch ranges. The
existing conservative stack checks passed: the largest graph envelope was
7,648/8,192 bytes, including a 1,024-byte allowance. This is not a
whole-program stack proof. The rebuilt boot, text and dispatch are byte-for-
byte identical to the independently linked feasibility candidate. Build log:
`/private/tmp/q35-fused-compact-builder-final-20261126.log`.

The actual service owner was also exercised with a 37,608-byte synthetic
dispatch image in simulated IO. Both ordinary and shared-role configurations
staged and read back every word on 120 simulated workers, closed cleanly, and
retained the owner when a high-offset write or readback was faulty. Separate
ASan/UBSan tests of the actual PCI-memory adapter wrote both the first word
beyond the old `0x7000` limit and the last word of the declared dispatch
range, rejected writes past the limit and while running, and verified the
simulated snapshot restoration. These checks establish software bounds and
failure handling in fake memory only; no physical writer exclusion or device
restoration is implied.

The builder also offers `--mixed-optimization`, keeping the stream and graph
dispatch objects at their earlier `-O2` setting while the rest use `-Oz`.
That image linked within the same two declared ranges: 56,152-byte text and
38,988-byte dispatch. Its conservative graph stack envelope is
7,920/8,192 bytes including a 1,024-byte allowance, not a whole-program
proof. The service-owner fake-IO staging, readback, high-offset failure and
close checks also passed at this larger dispatch size in ordinary and
shared-role configurations. The mixed builder log is
`/private/tmp/q35-fused-compact-mixed-build-20261126.log`.

An authenticated four-layer simulation of the mixed image is active in
session `99286`, log
`/private/tmp/q35-fused-compact-mixed-four-layer-sim-20261126.log`. It has no
result yet. Retaining an earlier compiler setting is not evidence of faster
execution; both compact images remain non-packageable and hardware-unqualified.

The authenticated four-layer simulation using the byte-identical `-Oz`
candidate completed 55 barriers on 120 simulated workers, reached its
intentional record-56 fault, and passed all 2,560 values with maximum absolute
error `5.00679016e-06`. Its actual output matched the earlier three-region
fused simulator output byte for byte. Log:
`/private/tmp/q35-fused-compact-four-layer-sim-20261126.log`. A full 32-layer
compact simulation with snapshots is active in session `98440`, log
`/private/tmp/q35-fused-compact-full-layer-snapshots-20261126.log`; it has no
final result. Other full-model simulations continue separately. The compact
image has not run on a P100A, may have different performance from the prior
build, and cannot be installed until model correctness, physical role
ownership and clean no-reboot recovery are established.

The builder now also has an explicitly non-loadable
`--profiled-compile-only` mode that compiles the normal graph cycle-counter
path instead of the simulator's unprofiled switch. Both size-optimized and
mixed-optimization variants compiled and linked into the two declared code
ranges. The mixed version has 56,152 bytes of text and 39,236 bytes of
dispatch; the size-optimized version has 56,152 and 37,784 bytes. Existing
conservative stack checks passed for both; the mixed graph envelope remains
7,920/8,192 including the allowance, not a whole-program proof. A 39,236-byte
synthetic dispatch also passed fake-IO service-owner staging, failure and
close tests for ordinary and shared roles. A dedicated marker is rejected by
both driver packagers and the simulator before library load. The unprofiled
mixed image rebuilt byte for byte unchanged after this builder change.
These are compile and simulated-IO results, not a measurement or proof that
hardware profiling, inference, recovery or clean unload works.

Image SHA-256: boot
`e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b`,
text `0cae7786d840092fecf0fa5a128b0a4a558da23d1d3ebea97caf52b68dc81d97`,
dispatch `eb7436a3b151dab2ef46fc2027d26e6cb195765701829d4fe15f631d4bca4f0f`.

## Full paired K/V and FFN graph capture, 2026-11-26

The exact pinned local Qwen3.5-4B Q4_K_M file generated a 32-layer recorded
decode fixture for the combined paired dense K/V, paired FFN, fused RMS/save,
and flash-hybrid graph. It contains 428 decode steps, 425 prompt steps, and
2,740,473,856 staged weight bytes. The generic simulator runner initially
rejected it because its length guard only knew the older 436-step schedule.
The guard now computes the supported step count from the graph markers, and
the focused rejection/count test passes.

The first full numerical simulation of this combined graph is active in
session 31294. At poll 9,300 it had captured layers 0–28. Each 2,560-element
F32 boundary through layer 28 is byte-identical to the earlier full simulator
capture. Latest layer hashes:

- Layer 26: `1fc0b0d3c61baa88a805450a08fb19c7500dc739ef8d22c7649dfb01b3d236a7`
- Layer 27: `3518c51e5ab820574f0d652d8df4b657ad6fe9d884286fe976a3bfdf9f120e3e`
- Layer 28: `86efde314c9f51c338a0d34c60a59baa1deffa82a7d2d6a1e991f666e89508a7`

A provisional same-input CPU audit used the pinned model SHA-256
`00fe7986ff5f6b463e62455821146049db6f9313603938a70800d1fb69ef11a4` and
the existing oracle SHA-256
`12083a72ac852e0fc445d2e0d173d40409c564d04e05a78e46d57f8d9f6d86b7`.
Through layer 15, initial layer 0 and 14 of 15 transitions passed. Transition
layer 8→9 failed on 36/2,560 values with maximum absolute error
`2.74516642e-05`, reproducing the earlier full-baseline discrepancy. All 15
CPU continuation runs selected token 261. The captured boundaries remain
provisional until the 32-layer simulator completes, and the CPU mismatch
means this is not a numerical full-model pass. Machine-readable results are
in `provisional-same-input-audit-20261126.json`.

A targeted CPU replay used the current capture's exact layer-17 activation
(SHA-256 `6e41a05fcf7bd29c7117faeb5aaef85d3fddbf0478aaa8a99d1cb1bdfc5d8f1e`).
It reproduced the layer 17→18 discrepancy on 2,469/2,560 values, maximum
absolute error `0.00355863571` and maximum scaled error `0.00186395645` at row
1. The CPU continuation still chose token 261. Candidate layer 18 is byte-
identical to the earlier simulator capture. This agrees with the prior
isolated layer-18 investigation, which localized sensitivity to a one-value
Q8_K quantization threshold in the recurrent-attention output projection;
the strict numerical discrepancy remains. See
`provisional-layer18-audit-20261126.json`.

A targeted replay of transition layer 24→25 passed the existing CPU tolerance
with maximum absolute error `1.90734863e-06` and selected token 261. The
captured simulator layer 24 and 25 tensors each match the previous full-run
snapshots byte for byte. Machine-readable result:
`provisional-layer25-audit-20261126.json`.

No second position or full-model final output has been verified. This remains
simulator evidence, not physical P100A execution. No model load time, prompt
time, decode rate, warm-run result, or clean physical unload has been
established.

A fresh strict SSH attempt with the pinned Haiku key and host key returned
`No route to host`; a separate attempt with the Linux profile timed out.
Process-list, exclusive-lock, and stable-driver status could not be observed.
Neither attempt accessed the accelerator.
