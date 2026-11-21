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

Complete the device-program rewrite before benchmarking; integrate with the
assistant only after the hardware generation and throughput gates pass.

## Current status (2026-11-21)

The rewrite now has a standalone Haiku generation client and an offline-built
120-core resident BRISC/Tensix service. Inference has no CPU fallback, and the
client reports authentication, model load, prompt processing, and decode
separately. The target remains two valid warm outputs at 100 decode tokens/s
or higher, followed by clean unload. No valid output or decode-rate measurement
has been produced yet.

The latest pushed code checkpoint is `f6d746c23b` (tag0109 rewrite and
artifact hashes). Tag0109 passed guarded preflight and uploaded all
`2,740,498,432` resident bytes. Authentication took `100.467732` s, prompt
tokenization `13.243073` s, and model load/upload `334.616263` s. Generation
ioctl `0xAF56` returned `EINVAL`; the client diagnostic printed the request
fields, but no prompt processing, valid output, decode timing, or tokens/s was
produced. Unload again failed at stage9/core0/detail3, the restored-core idle
proof. The client exited; the guarded wrapper retained the exclusive lock and
stable-driver backup while the candidate reported generation3 `cold1
active_owner1`. The guarded recovery helper restored stable driver55 at
generation2; the lock is absent, and postflight confirmed both model sizes,
64 CPUs, and full RAM. Run details and raw evidence are in
[`benchmarks/qwen35-dataflow-tag0109-20261121/RUN.md`](benchmarks/qwen35-dataflow-tag0109-20261121/RUN.md).

Tag0109's request-rejection diagnostic and bounded restore settle delay did
not resolve either hardware failure. The offline variant-60 build, owner and
PCI-memory ASan/UBSan fixtures, RV32IMF compile, and guarded-script syntax
checks pass. After restoring stable driver55, continue the rewrite offline to
make the request ABI acceptance explicit and make error teardown recoverable
before another benchmark. Tag0109 build hashes are in
[`benchmarks/qwen35-dataflow-tag0109-20261121/BUILD.md`](benchmarks/qwen35-dataflow-tag0109-20261121/BUILD.md).

## Progress

- 2026-11-21 (tag0109): Added structured generation-ioctl rejection details
  and split fleet-resume diagnostics, plus a bounded 10,000-spin settle window
  before restored-core idle checks. The all-core Tensix rewrite still has no
  CPU inference path. The full offline variant-60 driver/client build passes;
  owner and PCI-memory ASan/UBSan fixtures pass; the RV32IMF role compile and
  both guarded-script syntax checks pass. Driver SHA-256 is
  `b87123aab5ff32e7080f14addf7e8e8a945c100e6380032eb52745e6a0387f84`, and
  client SHA-256 is
  `d55bed03a653529c78383c04d19584def654fda83d874257e06ab48e406be4a0`.
  Tag0109 silicon evidence is now recorded. It still has not passed generation
  or clean unload. See
  `benchmarks/qwen35-dataflow-tag0109-20261121/BUILD.md`.
- 2026-11-21: Added the standalone resident-dataflow benchmark client and
  builder. It authenticates the pinned GGUF, uploads the packed model, binds
  its manifest and graph workspace, installs the 120-core resident service,
  requests two repeatable greedy generations, reports model-load, prompt, and
  decode timings separately, checks the known `11751,13` output prefix, and
  explicitly closes the service and seven-bank session. Variant 60 now enables
  the required span uploader under its fixed build tag without enabling the
  legacy Work path. The Haiku RV64 client and variant-60 driver build offline;
  driver SHA-256 is
  `0f9e9b04ebcd66509f05da5053f47bd21c323122fd9e41db9ce5934e532cae7a`, client
  SHA-256 is
  `b2f45646e69e4ae7c83bff3f0572ed60c4ea97d822bebfda158f4456f1a49c08`, and
  service text SHA-256 remains
  `2e0e144040f9c29f9679d5300ff923847c75216b3bf0e577366be4e929e5f8c8`.
  Read-only Haiku preflight found no 27B diagnostic process and reported stable
  driver55 tag `2026102100010012`, generation14, `quiesced0 cold0
  active_owner0 open_count1 guard1`. No model inference has run yet. Build and
  preflight evidence is in
  `benchmarks/qwen35-dataflow-tag0102-20261121/BUILD.md`.
- 2026-11-21: Added the request-level greedy generation ioctl at `0x8847`.
  It accepts prompt token IDs, skips unused vocabulary projection for prompt
  positions before the last, returns generated token IDs, stops at an optional
  EOS token, and enforces the model context bound. The model ABI is now 5. The
  response separates prompt time (through selection of the first output
  token) from decode time (subsequent generated tokens), so decode throughput
  is computed from `output_count - 1` over `decode_time_us`. ASan/UBSan checks
  cover prompt head skipping, bounded positions, EOS, and timing fields; the
  RV32IMF target compile and offline variant-60/default-control build pass at
  tag `2026112100010102`. Driver SHA-256 is
  `fea103ea9e5290da1742c2efe3585564eddf5dd627a95b54f904a7ab53582b87`; service
  text remains 52,900 bytes with SHA-256
  `2e0e144040f9c29f9679d5300ff923847c75216b3bf0e577366be4e929e5f8c8`.
  This code is not yet validated on the Pioneer.
- 2026-11-21: Added device-side greedy token selection after the tied Q6_K
  vocabulary projection. The resident BRISC worker scans the on-card FP32
  logits and writes a checked token/score/sequence record to core 0's mailbox;
  all 120 workers still receive, complete, and acknowledge the command. The
  host owner verifies each invocation readback, the stable result record, all
  worker receipts, runtime state, and drained queues before committing the
  selected token. The decoder ioctl now returns `output_token` in its existing
  40-byte ABI record. ASan/UBSan checks pass for stable greedy ties, NaN
  rejection, all-core owner choreography, vocabulary authorization, and the
  full mixed decoder sequence. The Haiku RV32IMF target compile passes with no
  A-extension instructions or runtime helpers. An offline variant-60 build at
  tag `2026112100010101` passes: driver SHA-256
  `1cacb820faa496a9d72eabad3d551fbd166ef7ecc9aa5a40b92bb3977550bedf`,
  resident service text SHA-256
  `2e0e144040f9c29f9679d5300ff923847c75216b3bf0e577366be4e929e5f8c8`
  (52,900 bytes of the 57,344-byte limit), reload guard SHA-256
  `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229`, and
  default-control SHA-256
  `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d`.
  This was offline evidence only; the subsequent entry adds the request loop.
  Pioneer correctness and decode rate remain unmeasured.
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
The resident all-120-worker owner, driver command path, offline generated-image
validation, full decoder sequence, and request-level greedy generation are now
implemented. Silicon correctness and performance evidence remain open.

## Current checkpoint: 2026-11-21

Checkpoint `55df940409` is pushed with device-side greedy token selection and
the full decoder sequence. The current local rewrite adds the bounded
request-level generation ioctl: token IDs go in, generated token IDs come out,
and prompt and decode time are returned separately. Prompt timing includes the
first output token; decode timing covers later output tokens, with throughput
reported as `(output_count - 1) / decode_time`. Tied projections use all 120
Tensix matrix engines. The BRISC argmax worker scans logits; all 120 resident
workers take part in each command barrier and receipt protocol. The driver
uses build tag `2026112100010102`.

Offline ASan/UBSan tests cover deterministic tie selection, non-finite-logit
rejection, argmax authorization and owner barriers, full decoder sequencing,
prompt generation, EOS, context bounds, and phase timing. The RV32IMF target
compile and variant-60 driver, resident service image, reload-guard, and
default-control builds pass. The resident service text is 52,900 bytes and
fits the 57,344-byte region. Tag0105 was the first hardware install attempt for
this request-level client. It uploaded and bound the model but failed before
worker readiness, so no silicon output, prompt time, or decode throughput was
produced. The verified failure and post-cycle recovery state are recorded in
`benchmarks/qwen35-dataflow-tag0105-20261121/`.

The offline request/generation loop and standalone benchmark client are
implemented. The initial preflight verified that the prior 27B diagnostic
had exited and stable driver55 was idle. The first tag0105 hardware trial
reached service startup and reported worker-readiness failure. Stable driver55
has been restored at generation2 with the recovery lock removed. Next, add
per-worker readiness diagnostics and bounded startup rollback before another
guarded hardware trial.

The dense-attention baseline `e533c033ef` includes
Tensix matrix projections, device-side Q6_K token embedding and hidden RMS
normalization, manifest-bound graph projections, SiLU×Up, residual save/add,
feed-forward sequencing, recurrent convolution, Alpha/Beta projections,
delta-state update, fused per-head RMS normalization plus SiLU gating, and
dense attention/KV. RecurrentOut uses the Tensix projection path with an
explicit input offset into the retained QKV V channels. Recurrent and dense
layer sequences are implemented. Mixed dense/recurrent full-model
orchestration, vocabulary readout, and token selection remain incomplete. No
hardware test or decode rate is available.

Checkpoint `6e0f12b4ff` adds exact authorization and readiness
tracking for the dense QueryAndGate, Key, and Value Tensix projections. The
projection test now checks the required QG -> K -> V order, workspace role and
layer ownership, and all 120 active Tensix assignments for QueryAndGate. The
focused host test passes with ASan/UBSan, and the Haiku RV32IMF compile check
passes without A atomics or runtime helpers. Dense RoPE, K/V cache packing and
writes, attention, AttentionOut, mixed-layer sequencing, vocabulary readout,
and token generation remain open; there is still no model output or decode-rate
measurement.

Checkpoint `e533c033ef` adds the dense-attention path. A manifest-bound
descriptor reserves the model's rotary table, per-layer KV cache, Q/K norms,
QKV output, gate input, and attention-output scratch. BRISC workers perform
Q/K RMS normalization, RoPE, half-precision K/V cache packing, and grouped
query attention with tiled NoC reads and online softmax; the AttentionOut
matrix projection remains on the Tensix path. Every command materializes and
reads back all 120 worker invocations and audits receipts before committing
authority state. Four workers own the K/V heads and sixteen own query heads;
the all-core barrier keeps the resident service synchronized.

`DataflowDenseLayerSequence` now composes residual save, attention norm,
QueryAndGate/Key/Value projections, K/V cache writes, attention, AttentionOut,
residual add, and the existing FFN sequence. Driver commands `0x8844` and
`0x8845` expose the dense-attention primitives and full dense-layer sequence.
ASan/UBSan checks pass for the dense math, worker, program/authority/owner, and
sequence fixtures. The math fixture matches the pinned operators exactly for
RMS, RoPE, F16 conversion, and online softmax. The RV32IMF target compile
passes without A atomics or runtime helpers. Offline resident-service-image,
reload-guard, and default-control builds pass; service text SHA-256 is
`1166102743dc8017be249c1d6a83a7a55642bc425ebbfa3b8eda74e4dbc98a59`,
reload guard SHA-256 is
`d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229`, and
default-control SHA-256 is
`5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d`.
These are offline checks only. Mixed recurrent/dense model orchestration,
vocabulary readout and token selection remain before full-model output.
No Pioneer run or benchmark has been performed.

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

Checkpoint `00b34e48eb` adds the recurrent output projection. Its descriptor
keeps the whole manifest-bound QKV RowSpace and selects the 4096-row V slice
through `inputBase`; the offset is copied to every core invocation and the
reader maps it across source fragments. Only the exact same-layer RecurrentOut
tensor and a successfully completed delta step are accepted. The authority
invalidates the destination before work and consumes QKV after a successful
commit or audited abort. ASan/UBSan delta-authority tests cover the exact V
offset, rejection of a shifted offset, all 120 matrix-worker assignments, and
one-time completion. Existing projection/authority checks pass. The RV32IMF
role compile passes without A atomics or runtime helpers. The resident service
image is 44,224 bytes with no unresolved imports and fits the fixed 57,344-byte
region. The offline driver60 build at tag `2026112000010099` passes; its SHA-256
is `e39be4fb33c5d614cc8959d7dd8a5c60510fe33c4ce6960d73c759c1c0af378d`.
Service text SHA-256 is
`7eea8370b070336d98ffd28897e2ce44fec519249f533b1e83506f80da58f950`; reload
guard and default-control builds pass. These remain offline checks; there is no
Pioneer output or performance measurement.

Checkpoint `cd6c25e985` adds a rewrite-only recurrent layer sequence and ioctl
at `0x8843`, using new driver build tag `2026112100010100`. It composes the
attention norm, four exact graph projections, recurrent convolution and delta
step, V-slice RecurrentOut, attention residual add, and the existing FFN
sequence. The sequence rejects dense layers and invalid positions, preserves
the first failing stage, and performs no tensor math itself. ASan/UBSan tests
cover operation ordering, invalid dense-layer requests, and stopping after a
delta failure. The Haiku RV32IMF role compile passes without A atomics or
runtime helpers. The offline driver60 build passes with SHA-256
`057b487b6543f4103934e8c24bc4f05a0af939b35d2013f4fe74b4e53c0772bc`; reload
guard and default-control builds pass. This is still offline-only: dense
attention/KV and final token generation remain before end-to-end model runs.

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

At checkpoint `221022cc34`, the manifest-bound delta program validated QKV,
gate, Alpha/Beta, learned decay/bias vectors, and state ping-pong ranges. It
partitioned 256 sixteen-value panels across all 120 workers. The BRISC worker
performed Q/K normalization, decay and beta gates, state update, and readout;
projections remained on the Tensix matrix path. The following checkpoint
extends this step with per-head RMS normalization and SiLU gating.

The projection authority permits a delta command only after matching QKV,
gate, Alpha, and Beta projections plus the same-position convolution have
committed. It invalidates QKV and consumes Alpha/Beta while the command is in
flight, then advances the delta position only after all 120 invocations,
worker receipts, runtime/queue audits, and acknowledgements pass. Position
zero ignores stale state and writes the alternate slot. Driver opcode
`0x8842` exposes this operation through the resident service.

The `061aa5c8ef` checkpoint assigns each of 32 recurrent heads to one BRISC
worker. Each owns all eight state panels plus the complete 128-value RMS
normalization group; gated values are stored back after all source panels for
that head are read. The 120-worker command/receipt/acknowledgement barrier is
unchanged, and projections still use all 120 Tensix matrix engines.

Offline ASan/UBSan fixtures pass for zero-state and warm-state recurrence,
head gates, bit-for-bit RMS-plus-SiLU comparison with the root operator,
descriptor coverage, authority transitions, and the all-core owner barrier.
The Haiku RV32IMF worker compile passes without A-extension instructions. The
size-optimized service image is 43,964 bytes, has no unresolved imports, and
fits the fixed 57,344-byte region. Offline driver60 SHA-256:
`d523169a4c46c51406a7d2c5b63847383095648e23681866b81b230ff4596578`;
service text SHA-256:
`8363fdcc559620eee9553a37c8061ab8ce9e7166595e90120dec1d277eca3d0f`;
reload guard SHA-256:
`d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229`;
default-control SHA-256:
`5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d`.
These are offline checks only. The operation has not run on the Pioneer and
has no silicon correctness or performance evidence.

The earlier checkpoints below describe implementation history. Hardware
correctness, warm-run output, measured throughput, and assistant integration
remain separate milestones. Do not report legacy Work measurements as rewrite
performance.

## Current rewrite progress (2026-11-21)

The tag0106 silicon trial authenticated, tokenized, uploaded the full 4B model,
and bound its manifest, then stopped at worker 0 startup with the local-core
route check failing. It produced no inference output or model-load, prompt, or
decode timing. The post-cycle recovery helper restored stable driver55 at
generation2 and removed the recovery lock. A read-only check confirmed the
Pioneer is idle, with both the 4B and 27B model files intact.

The next rewrite change drains startup results from all 120 workers after the
first readiness failure, retaining the earliest worker diagnostic. This gives
later workers time to reach a stable ready or startup-fatal state before the
existing all-core stop audit runs. The focused owner and PCI-memory tests pass
with AddressSanitizer and UndefinedBehaviorSanitizer, the RV32IMF role compile
passes without A atomics or runtime helpers, and the offline driver/package
build passes. The rebuilt driver SHA-256 is
`531f419a9c6d213cdf75da0afd0e27e327c6750086cda0375d59e3684d8fd420`; the
service text is unchanged. These checks do not establish silicon recovery.
At that checkpoint the route mismatch and exact cleanup failure still needed
hardware evidence; tag0107 later captured raw route `0x312` and a resume
failure, as recorded below. No warm output or decode rate exists; the 100
tokens/s goal remains unmet. Assistant integration remains deferred until the
rewrite and benchmark are complete.

### Tag0107 recovery and route diagnostics

Tag0107 now records the phase-2 worker's raw local route, waits for the full
120-core startup scan, and reports the stop-audit stage/core. Its special
recovery proof admits only that exact route-fatal state when the selected
physical core, service image, captured NoC sources, raw route, queues, and
idle NoC are stable; it does not admit the route for inference. The protocol
is ABI 2. Owner and PCI-memory ASan/UBSan tests pass, the RV32IMF service
compile passes without A atomics/runtime helpers, and the offline package
build passes. Driver SHA-256:
`176aacf58aeb972aabbe05d5c3ca6f89f5af1a1f3648ba96d7c932abea085668`; client
SHA-256: `232a98fd41c77f3e559e5de983a156febb65ff483c16da5181dcc28f268d72cd`.
The guarded tag0107 trial and post-cycle recovery helper were run on the card.
The trial authenticated, tokenized the prompt, uploaded
the complete 4B model, and bound its manifest. Startup again stopped at worker
0: phase 2 rejected raw local route `0x312` (786). No prompt execution or model
load/prompt/decode timing was produced. Cleanup stopped after the client
reported `close_failed result=18` (resident-service resume failure); the
client descriptor closed, but the guarded wrapper correctly refused to swap
drivers while the candidate reported `cold1 active_owner1`. The current card
state is tag0107 driver60, generation3, `cold1 active_owner1 open_count1`, with
the recovery lock retained. The stable driver has not yet been restored.

This run confirms the route-fatal branch is reached and the driver does not
silently fall back to CPU inference, but it does not verify the attempted
no-reboot cleanup path. The next rewrite checkpoint needs a precise
restore/resume failure stage and core, then a corrected worker identity/route
contract before another model run. No valid output, warm run, or decode rate
exists; the 100 tokens/s goal remains unmet. The 27B setup was verified intact
at preflight and was not accessed by this 4B run. Assistant integration remains
deferred until the rewrite and benchmark are complete.

### Tag0108 route and cleanup rewrite (2026-11-21)

Tag0108 no longer derives a logical worker number from the firmware's raw
local NoC register. The kernel writes the selected core ID into the startup
mailbox; each worker validates that assignment and checks invocation ownership
against it. During the all-core preflight the host now captures a stable,
unique local route for each selected core and requires that route to remain
unchanged during inference. Projections still have no CPU fallback and use
the Tensix matrix engine across the 120-core service fleet. Close now reports
the exact core and substage when restoring or resuming the fleet fails.

The new mailbox/client protocol is ABI 3. Five focused sanitizer tests pass,
the RV32IMF compile passes without A atomics or runtime helpers, and the full
offline package build passes. Tag0108 driver SHA-256 is
`62d64043cf5f3e2110b96446839efdfe2c61a945c8d87cd21d9d9a52072de1dc`; client
SHA-256 is `cbee1483b73fa8e08754fbd04d1f78376e725ae7dc6e078ad271fdc2a58a256a`.
The guarded two-warm and post-cycle recovery scripts are prepared.

The guarded tag0108 trial passed preflight from stable driver55 generation2
idle, installed candidate generation3, and uploaded all 2,740,498,432 model
bytes. Authentication took 100.027643 s, vocabulary/prompt tokenization
13.277220 s, and model load/upload 334.815789 s. The first generation ioctl
returned `EINVAL`; no generation completed, so there is no valid output,
prompt-processing timing, decode timing, warm-run rate, or tokens/s. The
client's prefix/repeatability messages are failure checks after the rejected
request, not generated text.

Cleanup returned `DataflowServiceResumeFailed` (result18), diagnostic
stage9/core0/substage3; unload was not clean, and the wrapper correctly kept
the lock and stable backup while tag0108 reported `cold1 active_owner1`.
After verifying the client had exited, one authorized power cycle and the
pinned SD-free Linux-to-Haiku handoff allowed the tag0108 recovery helper to
quiesce the fresh generation1 candidate and restore stable driver55 at
generation2. A read-only postflight confirmed stable55 idle, the lock absent,
both model files intact, and 64 CPUs/full RAM. Raw logs and exact post-recovery
hashes are in
[`benchmarks/qwen35-dataflow-tag0108-20261121/RUN.md`](benchmarks/qwen35-dataflow-tag0108-20261121/RUN.md).

The next code-only rewrite step is to add a precise receipt for the rejected
generation request and resolve the core0 resume phase3 failure before another
hardware trial. The 100 decode tokens/s target remains unmet. Assistant
integration remains deferred until the rewrite and benchmark are complete.
