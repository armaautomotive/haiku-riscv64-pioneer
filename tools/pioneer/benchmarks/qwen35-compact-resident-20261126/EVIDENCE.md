# Compact resident simulator image, 2026-11-26

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
probe modes and missing guards. The actual four-layer simulation is running
as session `52825`, log `/private/tmp/q35-resident-normal-four-layer-20261126i.log`;
it started before the additional command-count guard, and its saved command
was independently checked to have count 433, token 42 and position zero.
No outcome is available yet. Its intentionally faulted next record is not a
clean driver unload or physical recovery test.

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
