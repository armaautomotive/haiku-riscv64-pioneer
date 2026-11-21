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

The pre-rewrite Work baseline is `1b2b57c561`; the branch tip immediately
before adding the BRISC worker was `830ec1011e`. Rewrite checkpoints are pushed
incrementally to `codex/qwen35-4b-p100a`; use that branch's current tip for the
latest checkpoint.

The Haiku RV32 cross-compiler is available at
`/Volumes/HaikuBuildLocal/generated.riscv64`; use
`tests/run_pioneer_qwen35_dataflow_target_compile_test.sh` for the checked
worker compile. The local Apple Clang 17 build has no RISC-V target.

The lower-level queue role templates remain offline contracts. The current
concrete path is `PioneerQwen35DataflowBlackholeWorkerEntry`, which binds the
BRISC-side NoC and Tensix operations into one bounded per-core worker. It
prefetches the next weight tile over NoC 0 while the matrix engine processes
the current tile. Tenstorrent's open `tt-lab` worker pattern is the closest fit for
this freestanding Haiku image ([project](https://github.com/tenstorrent/tt-lab),
[BRISC firmware](https://github.com/tenstorrent/tt-lab/blob/main/src/brisc.cpp)).
It is still missing a resident owner that materializes and launches all 120
invocations, downstream sharded activation chaining, native image validation,
and silicon correctness/performance evidence.
