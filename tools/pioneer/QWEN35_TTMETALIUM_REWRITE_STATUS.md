# Qwen3.5 Tenstorrent device-program rewrite

## Scope

Rewrite the Qwen3.5 Work execution path around one resident BRISC worker per
Tensix core. Each worker owns one immutable shard, performs bounded NoC 0
reads, issues projections to the Tensix matrix engine, and sends results with
NoC 1. Keep two alternating L1 pages for weight and activation staging, sharded
intermediate activations, and no host inference path. This follows
Tenstorrent's per-worker BRISC programming pattern; it is not a claim that the
Haiku image runs TT-Metalium or its separate NCRISC/TRISC kernels. Keep the
checkpoint history on `codex/qwen35-4b-p100a`.

During this phase, work only on rewrite code and its offline code checks.
Defer Pioneer benchmark runs and assistant integration until the device-program
rewrite is implemented.

## Progress

- 2026-11-20: Added `PioneerQwen35DataflowTensixOps.h` as the concrete
  compute-role adapter. It binds one immutable core invocation, selects the
  right 16-value activation panel and scale, issues the Q8_0/Q4_K/Q5_K/Q6_K
  unpack and four Tensix matrix phases, validates bounded integer dot
  readbacks, runs the SFPU affine accumulator, and publishes a 16-row FP32
  page. Q5 affine scale/minimum extraction is isolated in the rewrite's
  metadata helper; packed Q5 operand expansion and all projection dots remain
  on the Tensix path. The MMIO emitter latches config/drain failures and
  returns through the queue failure channel instead of parking. The compute
  contract now requires explicit `Bind`. ASan/UBSan host role checks pass,
  and the Haiku RV32IMF probe instantiates the concrete Q5 and full compute
  paths without A-extension instructions or runtime helpers. This is compile
  and instruction-model evidence only: separate runnable device images,
  resident all-core ownership, and silicon validation are still missing. No
  Pioneer access or benchmark was performed.
- 2026-11-20: Added a rewrite-only Q5_K Tensix unpack path. The new panel
  span selects the original `qs` and `qh` byte-major windows from either
  double-buffered weight page; its SFPU instructions combine each low nibble
  with the selected high bit and write the existing lossless integer overlay
  into Dst for the matrix source path. The offline vector-layout machine
  matches the scalar reference across 2,048 panels and 524,288 operands, and
  the Haiku RV32IMF compile probe instantiates the Q5 path. These checks cover
	operand decoding and compile lowering only at that checkpoint: no native
	image or silicon qualification existed then. No Pioneer access or benchmark
	was performed.
- 2026-11-20 status review: the rewrite is at the pushed NoC-role checkpoint
  `eb6518f8a7`; no Tensix compute backend, runnable role images, or resident
  all-core owner exists yet, so this rewrite has no valid tokens/sec result.
  At the time of that review, the immediate model blocker was Q5_K: the
  rewrite vector unpacker
  accepts Q8_0/Q4_K/Q6_K, while the only Q5_K panel packers are explicitly
  scalar/offline references. Since Qwen3.5-4B Q4_K_M uses Q5_K tensors, those
  helpers cannot be put on the Work path. No Pioneer access, benchmark, or
  assistant integration was performed during that review.
- 2026-11-20: Added version-2 per-core descriptors with separate bank route
  tables for NoC 0 and NoC 1. Each table is checked for valid, unique XY
  routes and copied into the immutable core invocation; the future device
  owner must provide independently audited routes for both fabrics. Added
  `PioneerQwen35DataflowBlackholeIO.h`: its reader operations issue bounded,
  counter-proved NoC 0 reads for the core's activation fragments and assigned
  weight tiles, and perform exact Q8 activation packing; its writer operations
  issue bounded, counter-proved NoC 1 writes for only the core's assigned
  output rows. This follows the Blackhole BRISC/NoC 0 and NCRISC/NoC 1 data
  movement split described in the [TT-Metalium watcher guide](https://github.com/tenstorrent/tt-metal/blob/main/docs/source/tt-metalium/tools/watcher.rst)
  and [Blackhole NIU memory map](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/MemoryMap.md).
  The host role test passes under ASan/UBSan, and the Haiku target compile
  check now instantiates these actual NoC transfer paths. Its matching route
  fixtures are offline geometry only, not measured NoC 1 routes. Reader and
  writer operations are not yet installed as separate device images; the
  Tensix compute backend and resident all-core program owner remain to be
  implemented. No card work or benchmark was performed.
- 2026-11-20: Replaced the queue's compare-exchange first-error word with
  per-role first-error slots. Each Reader, Compute, or Writer image is the
  sole writer to its role slot, and queue polling checks all slots. The L1
  queue-state reservation now uses 32-byte aligned records. RV32 builds use
  explicit `fence` instructions and aligned volatile 32-bit loads/stores, so
  the queue protocol does not require the RISC-V A extension or a libatomic
  helper. Host checks retain compiler atomics for concurrent test coverage.
  The returned queue error uses fixed Reader/Compute/Writer priority when
  multiple roles fail; each role's first error remains preserved in its slot.
  On RV32, the four failure slots are scanned with one acquire fence after the
  reads, keeping the normal polling path from issuing four fences per check.
  The concurrent role exercise and Q5 panel check pass under ASan/UBSan. A
  compile-only adapter now instantiates the three role templates using the
  Haiku cross-toolchain with the Work image's `rv32imf_zicsr` flags; the
  repeatable check is `tests/run_pioneer_qwen35_dataflow_target_compile_test.sh`.
  The ELF32 object has no unresolved helpers or A-extension instructions, and
  its acquire/release fences are present in disassembly. This checks the role
  contracts' target lowering, not a device backend or runnable image.
- 2026-11-20: User confirmed the work order: finish the Tenstorrent-style code
  rewrite first; only then proceed to hardware benchmarking and assistant
  integration. Keep this file updated as the rewrite's checkpoint log. No
  benchmark or assistant-integration work belongs in the current phase.
- 2026-11-20: Moved the immutable per-core invocation from `0xa0000` to
  `0xaa000`. The prior address overlaps the existing Tensix destination-init
  source window (`0xa0000..0xa03ff`); the new 4 KiB invocation region ends at
  `0xab000`, before root scratch at `0xb0000`. The circular-buffer area still
  ends at `0xa0000`.
- 2026-11-20: Changed queue state publication to acquire/release atomics. A
  producer publishes page writes before advancing `produced`; a consumer
  publishes completion before the producer can reuse the page. Failure
  propagation atomically preserves the first role error. The offline test
  checks the first-error rule and verifies the queue refuses further work
  after failure. A separate ASan/UBSan SPSC stress check exchanged 50,000
  ordered pages between concurrent producer and consumer threads without loss
  or reordering.
- 2026-11-20: Added Q5_K (`type=13`) to the offline dataflow-program and
  activation-packing checks. Its projection tiles use the same Q8_K activation
  packing as Q4_K/Q6_K; Q8_0 remains a separate activation format. This closes
  the Q5 descriptor/activation-plan gap, while its Tensix matrix operation is
  still an unimplemented backend gate. Expanded the descriptor check across
  all eight Qwen3.5-4B quantization/width combinations and rebound every shape
  to all 120 core invocations.
- 2026-11-20: Added an offline-only Q5_K panel-layout reference that preserves
  exact integer weights and FP32 coefficient/minimum metadata in the Tensix Q8
  panel-zero layout. A compile guard keeps this scalar reference out of Work
  code; the eventual backend must produce the layout with device Tensix
  operations. The offline test decodes the synthetic tile through the Tensix
  panel decoder and checks every operand plus metadata.
- 2026-11-20: Reworked the per-core L1 map after finding that two adjacent
  maximum Q6_K weight pages would overlap the existing affine metadata at
  `0x91200..0x91280`. Weight pages now ping-pong between `0x90000` and
  `0x98000` with a checked `0x8000` stride, matching the Qwen35 tile-prefetch
  plan; activation cache and queue state occupy `0x80000..0x89030`, and
  input/output queues start at `0x92000` and `0x92800`. Both the full program
  and copied core invocation reject any page
  layout that overlaps affine metadata, queue state, activation cache, or
  another buffer. Added a rewrite-only vector-unpack entry that accepts the
  current page address and validates both ping-pong slots; its offline check
  verifies every Q8/Q4/Q6 panel source address. Reserved `0x90fc0..0x91280`
  for the Q5 repacker's Q8 panel-zero tile plus affine metadata; compile-time
  offset checks bind its coefficient/minimum fields to `0x91200`/`0x91240`,
  and only Q8 panel zero may unpack from that staging address. The compute Ops
  adapter still does not call these helpers.
  The dataflow and Q5 panel tests both pass under AddressSanitizer and
  UndefinedBehaviorSanitizer after the L1 layout change. A negative compile
  check also confirms the scalar Q5 reference is rejected unless its
  test-only macro is set.
- 2026-11-20: Added `PioneerQwen35DataflowKernels.h` with separate reader,
  Tensix-compute, and writer role contracts over two-page single-producer,
  single-consumer local queues. The compute template requires an explicit
  Tensix-matrix-engine capability, and each role validates its core's shard
  descriptor before touching device data. Role setup errors publish a queue
  failure so a neighboring role can leave its bounded wait.
- 2026-11-20: Added a host-only concurrent reader/compute/writer exercise over
  a complete Q5_K shard using the real queue and role loops. It checks packed
  activation chunks, weight reads, Tensix-call counts, output-page counts, and
  queue drain. This exposed a reader address bug: groups of 16 rows must
  advance by `16 * rowBytes`; the reader now bounds each group and block with
  that stride. The mock Ops validate choreography only and do not perform
  inference. The exercise passes with AddressSanitizer and
  UndefinedBehaviorSanitizer.
- The reader packs each 256-value activation chunk once in local memory and
  reuses that packed chunk across its assigned output groups. Q8_0 weight
  blocks map eight-at-a-time to the same 256-value activation chunk.
- Added `DataflowCoreInvocation`: the full 120-core schedule remains host-side,
  while each core gets only its own immutable shard assignment in the bounded
  4 KiB local region at `0xaa000`. The per-core object is size-checked and
  revalidates its local buffers, queue layout, and bank ranges.
- Added a device-side local-L1 binder for the invocation, queue controls,
  circular buffers, and activation cache at their fixed addresses. It is
  compile-checked only; no Blackhole role image consumes it yet.
- Added reader, compute, and writer entry wrappers that bind that local
  context and publish a role failure to shared queue state if binding fails.
- Expanded the offline program test to compile all three role interfaces and
  check 9,216-row output shards and a 9,216-value Q4_K input across all 120
  per-core materializations. It also checks Q6_K and Q8_0 plans, shard
  coverage, queue address binding, queue publish/consume, ticket wraparound,
  in-place activation packing, Q8 block mapping, and malformed-descriptor
  rejection. The host test passes with AddressSanitizer and
  UndefinedBehaviorSanitizer.
- The existing Work dispatcher and ROOT gather path are retained as the
  checkpoint; no changes to that path are part of this rewrite phase.
- Added `PioneerQwen35DataflowProgram.h`: a 120-core program descriptor and
  builder that maps each projection shard to its weight bank, sharded
  activation input/output ranges, and double-buffered reader/compute/writer
  circular-buffer layout.
- The lower-level concurrent reader/compute/writer loops remain offline
  contracts. The current device implementation is the resident BRISC service
  image and per-core projection worker described below; it overlaps the next
  NoC 0 weight prefetch with Tensix work. The host PCI image loader and physical
  mailbox/recovery adapter are not implemented.

## Rewrite gates

1. Complete offline descriptor and per-core worker checks.
2. Build the concrete resident worker image with its NoC and Tensix operations
   for the Haiku RV32 image, with no CPU inference or fallback.
3. Wire the new host owner to the physical PCI transport so it loads the image
   across all 120 cores, materializes per-core descriptors, drives the mailbox,
   and preserves sharded activations across dependent operations.
4. Complete offline target and full-graph correctness gates. Only then begin
   the hardware benchmark and assistant integration phases.

## Latest rewrite work

- 2026-11-20: Added `PioneerQwen35DataflowBlackholeWorker.h`, a concrete
  one-BRISC-worker-per-core projection path. Each core reads its input and
  weight shard over NoC 0, packs Q8 activation chunks locally, alternates its
  two L1 weight/input pages, issues each output group through the Tensix
  matrix engine, and writes the sharded result over NoC 1. It validates the
  fixed per-core queue state before use and publishes bounded Reader, Compute,
  or Writer failure slots before returning. No host dot-product or inference
  fallback exists. The target compile probe now instantiates this worker, and
  the Haiku RV32IMF cross-compile passes without A-extension instructions or
  runtime helpers. At this initial worker checkpoint, the service loop and
  standalone image builder had not yet been added. No Pioneer access,
  benchmark, or assistant integration was performed.
- 2026-11-20: Added a 64-byte per-core mailbox ABI and a resident BRISC service
  loop. The service verifies its local XY route against the immutable core
  assignment, executes one projection invocation, publishes completion or a
  role failure, then accepts another only after host acknowledgement. On that
  acknowledgement it verifies the local queues are drained and clears their
  error slots. Two weight pages now overlap a NoC 0 prefetch with Tensix work.
  The standalone image builder passes and emits a 25,024-byte RV32 image with no
  unresolved imports; the target compile probe still passes without A-extension
  instructions or runtime helpers. The offline mailbox/route check also passes
  under AddressSanitizer and UndefinedBehaviorSanitizer. This is offline image
  evidence only: the physical host launch/acknowledgement owner is missing,
  and the image has not been loaded or run on the card. No benchmark or
  assistant integration was performed.
- 2026-11-20: Added `PioneerQwen35DataflowOwner.h`, a host-side projection
  coordinator contract. It checks all 120 workers, materializes and reads back
  every immutable invocation before publishing any command, waits for terminal
  receipts from the whole shard set, and only acknowledges after the backend
  audit succeeds. Ambiguous partial publication moves the owner into a faulted
  state; `Recover()` requires an all-core backend recovery audit before it
  acknowledges completed workers and resumes with a higher mailbox sequence.
  ASan/UBSan mock coverage passes normal two-projection execution and partial
  launch recovery, including workers resynchronizing across skipped sequence
  values. This is the orchestration contract and mock only: its physical PCI
  transport, image load, recovery proof, and all-core graph are still missing.
  No Pioneer access, benchmark, or assistant integration was performed.
- 2026-11-20: Added `PioneerQwen35DataflowServiceOwner.h` to own the resident
  image lifecycle around that projection coordinator. It requires an idle
  session and a checked 120-route map, captures all core state before writes,
  holds every core, stages the worker image in bounded 256-byte chunks, and
  reads back code, queue controls, mailbox, and boot word before starting any
  worker. It waits for every worker's ready receipt, then delegates projection
  commands. Close stops and audits the fleet, restores and verifies all
  captured L1 and boot state while cores remain held, then resumes each core's
  captured reset state only after the whole fleet matches. The ASan/UBSan mock
  passes full-fleet startup, preflight refusal, partial-start close, stop-audit
  retry, projection, restoration, and original running/held-state recovery. A
  Haiku RV32IMF compile of the service owner also passes. This is still an IO
  contract and mock: the Haiku PCI/L1/reset
  adapter, generated-image packaging, full model graph, and physical recovery
  proof remain unimplemented. No Pioneer access, benchmark, or assistant
  integration was performed.

The pre-rewrite Work baseline is `1b2b57c561`; the branch tip immediately
before adding the BRISC worker was `830ec1011e`. Rewrite checkpoints are pushed
incrementally to `codex/qwen35-4b-p100a`; use that branch's current tip for the
latest checkpoint.

The Haiku RV32 cross-compiler is available at
`/Volumes/HaikuBuildLocal/generated.riscv64`; use
`tests/run_pioneer_qwen35_dataflow_target_compile_test.sh` for the checked
worker compile. The local Apple Clang 17 build has no RISC-V target.

The lower-level queue role templates remain offline contracts. The concrete
per-core path is `PioneerQwen35DataflowBlackholeWorkerEntry`, which binds
BRISC-side NoC operations and Tensix matrix operations into one bounded worker.
It prefetches the next weight tile over NoC 0 while the matrix engine processes
the current tile. Tenstorrent's open `tt-lab` worker pattern is the closest fit
for this freestanding Haiku image ([project](https://github.com/tenstorrent/tt-lab),
[BRISC firmware](https://github.com/tenstorrent/tt-lab/blob/main/src/brisc.cpp)).
The resident all-120-worker owner, driver command path, and offline generated
image validation are now implemented. Downstream sharded activation chaining,
full model generation, and silicon correctness/performance evidence remain
open.

## Current checkpoint: 2026-11-21

The latest pushed rewrite checkpoint is `221022cc34` (`Add Qwen35 delta-state
worker path`). The offline rewrite includes Tensix matrix projections,
device-side Q6_K token embedding and hidden RMS normalization, manifest-bound
graph projections, SiLU×Up, residual save/add, feed-forward sequencing,
recurrent convolution, Alpha/Beta projections, and the first recurrent
delta-state update. The resident all-core service has an audited delta-state
command and driver opcode. Dense attention, post-delta normalization/gating,
recurrent output projection, final vocabulary readout, and token generation
remain incomplete. No hardware test or decode rate is available.

The graph workspace participates in the projection path. The request can
select the Up, recurrent QKV, or recurrent gate sidecar; the authority maps
each selector to its exact model role and rejects recurrent targets on dense
layers. It tracks readiness per scratch tensor, independently from activation
buffers and from other scratch tensors. ASan/UBSan projection and authority
checks, the Haiku RV32IMF role compile, and driver60, reload-guard, and
default-control builds pass. The driver60 SHA-256 is
`35c2c1e0f5ae691f5943ec56aa18a21a90e3a5bb69c3c691cb919696bf08b90a`.
These selectors make QKV and recurrent-gate projections available through
the existing 120-core Tensix path. No legacy Work changes are part of this
step.

The device-side SiLU×Up step is implemented and pushed in `8ed176883c`. Its
manifest-bound schedule assigns all 576 sixteen-row panels over the 120
resident workers (96 workers handle five panels and 24 handle four). Each
worker reads its gate and graph Up panels over NoC, computes SiLU(gate)×Up on
the device, and writes the gate activation in place. The authority requires a
completed exact Up projection and initialized gate activation, invalidates
both inputs before execution, and commits the gate only after the owner audits
all worker receipts, invocation readbacks, and queues. A root-only `SILU`
control opcode exposes this primitive; it does not add generation or a CPU
fallback.

ASan/UBSan checks pass for the SiLU approximation, 120-core descriptor
coverage, exactly-once worker simulation, authority state transitions, and
owner orchestration. The Haiku RV32IMF compile passes with no A atomics or
runtime helpers. The offline driver60, reload-guard, and default-control
builds pass; driver60 SHA-256 is
`d8435142c513f9b94290a706a17d440449fea8b8dd957c27ece2bf128a9873f2`, reload
guard is `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229`,
and default-control is
`5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d`.
These are offline checks only.

The residual save/add operation is implemented and pushed in `019b7fb29e`. The
manifest-bound schedule assigns all 160 hidden-width panels across the 120
workers (40 workers handle two panels and 80 handle one). `SAVE` copies an
initialized Hidden-row activation into residual scratch; `ADD` adds that saved
residual into the activation in place. The authority invalidates residual
readiness before save and consumes it before add, then commits only after
all-core invocation readback, terminal receipts, runtime/queue audit, and
acknowledgement. The root-only `RESIDUAL` opcode exposes the two explicit
modes without host math or fallback.

ASan/UBSan checks pass for exactly-once coverage and save/add arithmetic on
all 160 panels, authority save/add transitions, and owner orchestration across
both modes. The RV32IMF compile passes without A atomics or runtime helpers;
the offline driver60, reload-guard, and default-control builds pass. The new
driver60 SHA-256 is
`5b1aff677a77cc86b75e725e0bd16c2e2a2c34aeb3b277cabbe5164be1c0961a`; the
service text SHA-256 is
`279cac1dddbc27f0c6621fec07b248d267714073355fb0cf83b812d6cc3cf9fe`.

The rewrite step in `2a2405752a` adds `DataflowFfnSequence`, which composes
those device operations into a complete feed-forward sublayer: residual save,
post-attention RMS, Gate and Up projections, SiLU×Up, Down projection, and
residual add. A rewrite-only root opcode exposes this sequence and reports the
failing stage. The coordinator only builds authorized programs and submits
resident worker commands; it performs no tensor arithmetic. Host ASan/UBSan
checks cover complete ordering, invalid requests, and stop-on-first-failure.
The RV32IMF sequence compile check passes without A atomics or runtime helpers.
The offline driver60, reload-guard, and default-control builds pass; the
driver60 SHA-256 is
`5b59a99ea3346af23399ec273bd810c3f9730bb4f5aa8c8cce0c131264045ed9`.
This is an offline-checked, pushed rewrite checkpoint; recurrent/dense
attention, state updates, final vocabulary readout, and token generation
remain unimplemented. No Pioneer access or benchmark was performed.

Checkpoint `ef1279b585` extends the validated graph-workspace sidecar to six
FP32 tensors: Hidden residual, FFN Up, recurrent QKV (`ConvChannels`),
recurrent gate (`DeltaInner`), and 32-row Alpha and Beta vectors. Manifest
validation requires all six regions in allocator order; the model-bind ABI is
now version 3. This gives the recurrent projection and state operators
reserved, bounded output storage. Checkpoint `cd35b0db45` adds the first
recurrent projection targets (QKV and gate); the projection state machine now
commits or invalidates each target independently. ASan/UBSan projection and
authority checks pass, as does the RV32IMF role compile. The offline driver60,
reload-guard, and default-control builds pass. The updated driver60 SHA-256 is
`35c2c1e0f5ae691f5943ec56aa18a21a90e3a5bb69c3c691cb919696bf08b90a`.
Recurrent convolution, delta-state math, dense attention, KV/state IO, final
vocabulary readout, and token generation remain unimplemented. The GGUF
fixture test was not run because no local pinned-model path is available. No
Pioneer access or benchmark was performed.

2026-11-20 rewrite progress: added the manifest-bound recurrent convolution
operator and its resident worker command in checkpoint `a769eea54c`. It reads
the completed recurrent QKV sidecar, the model's four-tap convolution weights,
and the selected
ping-pong history over NoC 0; each of the 120 cores owns four or five of the
512 channel panels and writes updated history plus the SiLU result over NoC 1.
The projection feeding this operation remains the rewrite's Tensix matrix
path; this convolution step is channelwise firmware math, not a matrix
projection. The new authority transition invalidates QKV while state is
updated and commits it only after all-core completion, invocation readback,
queue/runtime audit, acknowledgement, and ready-state checks. A rewrite-only
driver ioctl exposes the command. The offline fixture checks exact position-0
and warm-position arithmetic, ping-pong history, complete 120-core coverage,
authority transitions, and owner sequencing. ASan/UBSan worker/owner and
existing dataflow tests pass, the Haiku RV32IMF compile passes without A
atomics/runtime helpers, and the offline driver60, reload-guard, and
default-control builds pass. Driver60 SHA-256:
`15e433e1b5c15fa1c2bba19255d9268c8b0e2f91e1e6b4826f518ed5383b18c2`.
This is offline-only evidence. No Pioneer access, silicon validation,
generated output, benchmark, or assistant integration was done. Delta-state
math and Alpha/Beta projections, dense attention, state/KV IO, vocabulary
readout, and token generation remain to be implemented before benchmarking.

Checkpoint `281ca54aab` connects the reserved recurrent Alpha and Beta
workspaces to their exact model roles in the existing Tensix projection
builder. The scheduler now permits fewer than 120 output panels: all 120
resident workers still receive, validate, and acknowledge the command, while
the two workers owning a 32-row gate projection issue matrix work and the
remaining 118 complete as bounded no-op workers. The graph authority tracks
each sidecar independently. Offline tests verify Alpha/Beta role and
workspace binding, active/inactive invocation coverage, and the all-core
owner barrier. ASan/UBSan projection, owner, and recurrent-convolution tests
pass; the RV32IMF role compile passes without A atomics or runtime helpers.
Offline driver60 SHA-256:
`a001c424f14e1c09dc8c67dba326732bcc7dc33b475941b1477b8cc971e2e59c`;
service-image text SHA-256:
`507aa854644964ec091fb7a19271d87edcad0dbf794481125ff0eda593db76b2`.
The driver, reload-guard, and default-control package builds pass. These
remain offline checks; no hardware work or performance result exists.

Checkpoint `307c9d134b` adds per-layer recurrent position tracking to the
authority. A convolution can run only at that layer's next committed position;
replay, skipping, and out-of-order requests are rejected. The new
`ResetRecurrentState()` request boundary resets position tracking and
invalidates recurrent scratch without writing the large resident state
regions. This is safe because position-zero convolution substitutes zero
history instead of reading the old ping-pong slot; the delta-state worker
likewise substitutes zero state at position zero. ASan/UBSan tests cover successful advance,
replay rejection, reset, and reprojection before a new position zero. The
RV32IMF role compile passes, and offline driver60, reload-guard, and
default-control builds pass. Driver60 SHA-256:
`a7cd35960b48c7844910e8f2f0df72b661ce19dd014d8b5ca4c1f96d4a715d15`.
No hardware validation or throughput evidence was produced.

## Delta-state worker `221022cc34` (2026-11-21)

The manifest-bound delta program validates QKV, gate, Alpha/Beta, learned
gate vectors, and the position-selected state ping-pong ranges. It partitions
256 sixteen-value panels across the 120 resident cores: 16 cores receive
three panels each and the other 104 receive two. Every core processes panels.
The BRISC worker performs Q/K normalization, learned decay and beta gates,
state decay, key prediction, rank-one state update, and query readout. Model
projections remain on the Tensix matrix path; recurrent vector/state
arithmetic runs in device firmware. There is no host inference path.

The projection authority permits a delta command only after matching QKV,
gate, Alpha, and Beta projections plus the same-position convolution have
committed. It invalidates QKV and consumes Alpha/Beta while the command is in
flight, then advances the delta position only after all 120 invocations,
worker receipts, runtime/queue audits, and acknowledgements pass. Position
zero ignores stale state and writes the alternate slot. Driver opcode
`0x8842` exposes this operation through the resident service.

Offline ASan/UBSan fixtures pass for zero-state and warm-state recurrence,
head gates, descriptor coverage, authority transitions, and the all-core owner
barrier. The Haiku RV32IMF worker compile passes without A-extension atomics
or runtime helpers. The resident service image is 56,760 bytes and fits its
fixed 57,344-byte region. Offline driver60 SHA-256:
`91b3dc0fcf26dde2244b56b986fcd060aed7dd4b899cc7cf1e9dc596b869fada`;
service text SHA-256:
`f38121fb4700345ceac583b0341c4130dbe6e8f23581a0c8e19cf1b84aa9befb`;
reload guard SHA-256:
`d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229`;
default-control SHA-256:
`5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d`.
These are offline checks only. The operation has not run on the Pioneer and
has no silicon correctness or performance evidence.

Continue completing the recurrent/dense layer operators and decoder graph.
Keep Pioneer runs, performance benchmarking, and assistant integration
deferred until the rewrite graph is implemented and its offline correctness
gates pass.

No model has been loaded on the Pioneer through this rewrite, no rewritten
operation has been validated on silicon, and there is no generated output or
decode-rate result. Do not report the legacy Work rate as rewrite performance.
