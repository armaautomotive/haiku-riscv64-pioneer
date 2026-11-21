# Qwen3.5 Work: Tenstorrent device-program redesign

This note records the implementation change requested after the Tag75 timing
showed that local command-register tuning was not addressing the bottleneck.

## Upstream pattern

TT-Metalium builds a device `Program` from per-core reader, compute, and writer
kernels, circular-buffer definitions, and runtime arguments. Tenstorrent's
open `tt-lab` also demonstrates a per-worker BRISC firmware pattern that
coordinates explicit NoC movement and Tensix instructions. The Haiku rewrite
adopts this per-worker pattern because its current freestanding build has one
RISC-V firmware image and no NCRISC/TRISC compiler integration. It does not
link the TT-Metalium runtime. The long-term target remains resident weights,
recurrent state, KV cache, and reusable decode programs.

References:

- [TT-Metalium single-core matmul](https://github.com/tenstorrent/tt-metal/blob/main/docs/source/tt-metalium/tt_metal/examples/matmul_single_core.rst)
- [TT-Metalium matmul lab and reader/compute/writer dataflow](https://github.com/tenstorrent/tt-metal/blob/main/docs/source/tt-metalium/tt_metal/labs/matmul/lab1/lab1.rst)
- [TT-Metal model performance optimizations](https://github.com/tenstorrent/tt-metal/blob/main/tech_reports/AdvancedPerformanceOptimizationsForModels/AdvancedPerformanceOptimizationsForModels.md)
- [TT-Metal LLM implementation guide](https://github.com/tenstorrent/tt-metal/blob/main/tech_reports/LLMs/llms.md)
- [TT-Transformers](https://github.com/tenstorrent/tt-metal/tree/main/models/tt_transformers)

## What the current Work path does

The current Haiku backend is a custom freestanding Blackhole image, not the
TT-Metalium host runtime. `DeviceIO::StageBand` builds work in batches of at
most 16 row groups per peer. The ROOT then writes a descriptor and doorbell to
each peer, waits for peers, reads their proof/output records, and gathers each
projection result back into ROOT L1 before the next model operation. The peer
image is resident, but it executes one projection assignment and returns to
its wait loop.

Tag75 verified two exact warm outputs at 0.120546 and 0.120550 decode tok/s.
Each request spent about 97.442 s in peer completion and physical proof and
55.800 s in command issuance plus ROOT matrix work. Exact command-register
reuse did not change that shape. This is a scheduling and dataflow design
problem, not a model-load problem.

## Target architecture

Keep the Haiku driver as the control plane and keep inference on the card.
Port the Tenstorrent per-worker program pattern into the freestanding
Blackhole images:

1. Give each of the 120 Tensix cores a stable shard and a persistent BRISC
   entry. The host materializes one immutable invocation per core; each worker
   processes its projection tile loop locally.
2. Each worker reads activations and quantized weights with NoC 0, issues
   projection operations to the Tensix matrix engine, and writes its output
   shard with NoC 1. It uses two fixed L1 weight pages to prefetch the next
   tile while the matrix engine consumes the current one. Activation packing
   and output writes are still synchronous.
3. Keep intermediate activations in sharded device buffers. Pass buffer
   addresses and shape/stride metadata to the next program rather than
   collecting every projection into ROOT L1 and redistributing it.
4. Use one device-side completion barrier per program/fused operation. Check
   per-core identity, source, counters, and physical PCs when admitting the
   resident session and when closing it. On an error, stop issuing new work,
   retain the current fault-pending path, and run the full physical recovery
   proof before cleanup. Do not weaken clean unload or no-reboot recovery.
5. Keep model weights, recurrent state, and the attention KV cache resident
   across warm requests. Use fixed-size decode programs with runtime position
   arguments so each token does not rebuild or reload the model.

## Porting sequence

1. Build a single-projection dataflow prototype using one core and a small
   verified Q4_K/Q5_K/Q6_K matrix. Check the tiled result against the existing
   scalar oracle and the current Work projection path.
2. Add a stable per-core shard plus circular-buffer reader/compute/writer
   roles. Measure a full 119-peer projection and validate output bytes before
   enabling the path in Work mode.
3. Keep sharded outputs on device for the next projection; remove ROOT gather
   from one two-operation chain and compare it against the existing path.
4. Fuse independent operations that share the same input, then move a complete
   Qwen3.5 layer group onto the program graph. Preserve the exact Q4_K_M model
   and its original quantized coefficients.
5. Run two valid warm requests, report load/prompt/decode separately, verify
   all-core participation, and require measured decode throughput to improve
   before promoting each candidate. The final target remains 100 decode tok/s.

The public TT-Metalium C++ runtime is not currently used by this Haiku driver.
The current implementation adapts the open [tt-lab BRISC worker
pattern](https://github.com/tenstorrent/tt-lab/blob/main/src/brisc.cpp) to the
available freestanding image. Numeric projections are issued to Tensix; there
is no CPU inference or host fallback.

## Performance budget and activation storage

P100A is specified for 448 GB/s GDDR6 bandwidth. The pinned 4B artifact has
2,740,937,888 bytes; reading each weight byte once per generated token would
require about 274 GB/s to reach 100 tokens/s, before KV/state and other traffic.
That is about 61% of peak bandwidth. Tag91's measured 0.123282 decode tok/s is
therefore not explained by the card's raw bandwidth limit alone: a resident,
parallel reader/compute/writer program and elimination of the per-projection
ROOT round trips are required. The roofline estimate assumes one full weight
read per token and is not a measured bandwidth result. See Tenstorrent's
[P100A specifications](https://docs.tenstorrent.com/aibs/blackhole/installation.html).

The new offline sharded activation allocator works over the seven 64 MiB
workspace regions already reserved by `PioneerQwen35PackedPlan`. It can reserve
the bank-0 prefix used by the existing RoPE table, page-aligns each bank
fragment, stripes 16-row groups across banks, supports resolving a row range
only when it stays inside one fragment, and tracks allocated ranges to reject
overlap. The initial use is a pair of ping-pong row buffers large enough for
the tied vocabulary projection. Its bounds/coverage test passes.
This is storage planning only: the Work image does not allocate or use these
buffers yet, and no hardware performance is claimed. The next implementation
step is the device-side writer/reader path that lets a projection publish one
sharded output for the next projection without gathering it through ROOT.

```sh
c++ -std=c++14 -Wall -Wextra -Werror -fsanitize=address,undefined \
  tools/pioneer/tests/pioneer_qwen35_activation_plan_test.cpp \
  -o /private/tmp/pioneer-qwen35-activation-plan
/private/tmp/pioneer-qwen35-activation-plan
```

## Prototype status

`PioneerQwen35DataflowBlackholeWorker.h` now contains a resident per-core
service and concrete worker body. It binds the invocation, fetches and packs
activation locally, alternates two weight pages, prefetches the next weight
tile over NoC 0 while Tensix consumes the current tile, and writes each
completed 16-row result over NoC 1. Its 64-byte mailbox uses monotonic
command/completion/acknowledgement sequences; the worker remains resident
between invocations and permits a new one only after the host acknowledges
completion or has audited a fault. After acknowledgement, it verifies each
queue is drained and rebases equal queue counters so another projection can
run. A version-2 mailbox shutdown request parks the resident worker after a
clean drain for verified unload. The standalone image builder produces a
freestanding image with no unresolved imports. `PioneerQwen35DataflowOwner.h`
specifies the host-side all-core sequencing and recovery contract, with a
passing offline mock. The bounded PCI helper is tested offline, but its
`PioneerSessionIO` binding, physical route/idle audit, and all-core graph
ownership remain to be implemented; the image has not been loaded or run on
the card.

`PioneerQwen35ProgramSchedule.h` builds a stable 120-core partition for a
whole projection. It assigns contiguous logical row ranges and splits a
core's work only at physical bank-fragment boundaries. The schedule is still
not consumed by the current Work image, so it makes no speed claim.

Tag76 added a checked one-tile lookahead to the active peer. Its plan test
passed 5,136 cases and the cross-build succeeded, but the hardware run faulted
before prefill. The peer started the next original-size weight-tile NoC read,
then fell through to the legacy synchronous `FetchBlock` call. Fault evidence
places a peer in that fetch path. The wrapper unloaded the client and restored
stable driver55 at generation32 without reboot. Tag76 produced no valid output
or decode measurement.

Tag77 fixes that dispatch fallthrough. The candidate cross-build and plan
test pass, and disassembly confirms that the Qwen35 prefetch path proceeds
directly to projection work. It produced two exact warm outputs and cleanly
restored stable driver55. Decode measured 0.120852 and 0.120861 tok/s across
the full 120-core layout, only about 0.25% above tag75. The peer completion
and proof phase still consumes about 96.75 s per request, while issue plus
ROOT matrix work consumes 55.80 s. The 100 tok/s goal remains about 827 times
above this result. The full TT reader/compute/writer program graph and
sharded activations remain unimplemented.

Tag90 tried to place the Qwen35 Work runtime descriptor and peer wake sequence
in the same 64-byte packet. Offline tests and cross-build passed, but hardware
faulted in the first projection before prefill. The peer reported projection
precheck reason `0x90`; the saved fault snapshot showed the descriptor was
eventually complete, which is consistent with the trailing wake sequence
becoming visible before preceding packet words. This race diagnosis remains
a hypothesis. The one-packet protocol is rejected and the Tag91 candidate
restores the previously tested separate descriptor-then-doorbell order. Tag90
did not produce output or a decode rate; the failure and recovery are recorded
in `benchmarks/qwen35-work-tag290-20261120/RUN.md`.

Tag90's fault was followed by a generic dispatch STOP failure at close phase
30. The driver retained the owner and set its cold-recovery latch; a cold power
cycle was then needed before the guarded stable-driver restore. The exact peer
or packet that prevented STOP completion is not established. Tag91 restores
the proven descriptor-then-doorbell path and completed two warm requests with
clean unload and stable restoration without reboot.

Tag92 adds a Work-only recovery path for the phase30 STOP failure. It requires
two complete all-core physical audits, a fault-parked ROOT with a stable fault
marker, each peer at either its prior wait loop or expected STOP park, exact
code and source proofs, and idle NoC queues before holding any core. Offline
close tests pass both payload builds, including mixed waiting/stopped peers
and negative cases that must quarantine without a reset. The Tag92 candidate
cross-build passes and retains Tag91's ROOT/peer images and client. It has not
been installed or tested on the card, so this is not yet a physical recovery
claim. Other retained-owner failures and other close phases still quarantine;
there is no general root-only recovery ioctl.

Tag91's latest hardware result is 0.123282 decode tok/s. The 100 tok/s goal is
about 811 times higher. Tag92 changes only the faulted STOP cleanup path and
does not claim a throughput gain. The next performance step remains an
executable resident device program with sharded activations between
projections and fewer ROOT/peer synchronization waves.

## 2026-11-20: rewrite checkpoint

The rewrite now has a resident per-core worker image, a stable 120-core
projection schedule, an all-core projection owner, and a service owner that
stages/verifies the image, waits for all workers, stops them, audits state,
restores saved L1/reset state, and resumes the prior fleet. The projection
owner now requires the IO adapter to authorize each program before it writes
invocations or publishes commands; denial leaves the sequence and device
untouched. `DataflowProjectionMatchesLayout` provides the concrete range gate:
it requires one exact manifest weight, two distinct declared activation
buffers, and the audited per-bank NoC routes. Its caller must already have
validated the full model manifest and physical routes. Tests reject forged
weight/input ranges, activation aliasing, and route mismatches. The
authorization state machine tracks the two ping-pong buffers: it requires an
initialized input, invalidates output before staging, and commits output only
after completion audit and acknowledgement. Four offline worker/owner tests
pass with AddressSanitizer and UndefinedBehaviorSanitizer, and the freestanding
RV32IMF target compile passes without atomics or unresolved runtime helpers.

This is still an offline prototype. The physical PCI IO adapter that binds the
validated model and audited routes, proves activation writes/readbacks, and
invokes this state machine has not been implemented; neither has Work-mode
integration. No rewritten projection has run on the P100A, so the rewrite has
no valid-output, warm-run, decode-rate, or physical-unload result. Next is the
PCI adapter and its route/idle/snapshot/recovery proofs, then a small verified
projection before model-graph integration or benchmarking.

The first bounded PCI L1 memory transport is now implemented as a standalone
driver helper. It only accepts the service text, queue state, invocation,
mailbox, and boot-word ranges; requires a held core for install and restore
writes; captures the full local-memory view and boot/reset state; and restores
and verifies that
snapshot while held. A fake-PCI test passes the bounds, idle-gate, snapshot,
and restore cases under AddressSanitizer and UndefinedBehaviorSanitizer. For
resident workers, it permits writes only to the invocation and the individual
command/acknowledgement words, requires reset to remain released, and asks the
platform adapter for the exact wait/receipt and idle-NoC proof before the
write. Worker-owned mailbox status fields remain read-only to the host. The
helper also implements the resident service owner's per-core snapshot calls.
Its stop path first audits all 120 cores, requests shutdown only from cores
running the exact resident image, waits for their stopped receipts, then holds
the fleet before restoration. The fake-PCI test mixes resident workers with
original dispatcher waiters and held cores, checks clean stop/restore/resume,
and rejects fleet resume when any snapshot is missing. It is now connected to
`PioneerSessionIO` under the separate driver60 build identity
(`2026112000010099`). The binding verifies the original all-core dispatch
images and wait receipts before takeover, validates the compiled service image
and local physical node ID while resident, and gates writes on stable ready
mailboxes, drained queues, and idle NoCs. It restores all saved L1 panels and
boot words before resuming the original reset states. The separate build
rejects both Qwen Work implementations at compile time.

The offline driver60 cross-build, reload-guard build, and default-driver
control build pass. Candidate SHA-256:
`160a0b85cbe6f01c9ab6a1f3469da94a44f67c96090a2e6bcad72656844530e8`; service
text SHA-256 remains
`10d9737cb1835c36acc546d778aee96f68c276ba5651f569f1622d25cb344dd7`. The
PCI mock passes the all-core capture, stop, restore, and resume audit. The
binding covers install and clean unload only; model upload, projection
authorization/auditing, in-flight projection recovery, and token decoding are
still unimplemented. No image has been loaded on the card and no rewrite speed
has been measured.
The standalone service image builder completes with no imports, a 4-byte boot
stub, and 25,532 bytes of worker text (SHA-256
`10d9737cb1835c36acc546d778aee96f68c276ba5651f569f1622d25cb344dd7`). This
confirms the updated shutdown-capable freestanding image only; it has not been
loaded on the P100A.

The driver60 binding now has a public, root-only support/query/install/close
protocol. INSTALL prepares the all-core dispatch wait lease internally and
then installs the resident dataflow service; QUERY reports its lifecycle and
projection sequence; CLOSE audits and restores the previous core state. The
rewrite driver rejects the generic dispatch RUN opcode, so this interface has
no route back to the old inference path. The offline driver60 cross-build
passes with this control path; candidate SHA-256:
`da07aeca4e6c57e146fc87d3c241171461e25a60409fa9e14d66b2c625a78d2a`.

The rewrite now has root-only model-bind, activation-seed, projection, and
recovery controls. Model bind validates the full 4B manifest against the
initialized-page ledger before preparing the all-core dispatch lease, then
binds an immutable manifest copy after the route proof. Activation seed moves
caller-supplied finite float rows into only the declared activation extent,
page by page, using the checked bank transport and readback; it does not
perform model arithmetic. The projection request contains only layer, role,
and activation-slot indices. The driver builds its own 120-core schedule from
the immutable manifest and route table. The projection owner now audits the
terminal mailbox/queue/image state before acknowledgement and marks the output
usable only after every worker returns ready. Activation authority tracks the
number of initialized rows so a short projection result cannot feed a wider
projection. Recovery double-audits the whole fleet before acknowledging
terminal workers; uncertain or non-idle NoC state remains quarantined. A
root-only activation-read opcode now reads only initialized float rows through
the checked resident-bank page transport, after an all-120-core idle audit and
before a second idle audit. The support ABI advertises readback, matrix
projection, device token embedding, and hidden RMS normalization as
input/layer primitives; it does not advertise token generation.

## Current rewrite progress (2026-11-20)

The device-side Q6_K embedding-panel decoder and manifest-bound schedule are
now executed by the resident worker command. A token lookup is split into 160
output panels across all 120 workers (40 handle two panels, 80 handle one).
Workers read only the needed Q6_K tiles through NoC 0, unpack their assigned
panels locally, and write disjoint activation rows through NoC 1. The driver
now has a root-only `EMBED_TOKEN` operation, verifies the request against the
bound manifest, stages and reads back each worker invocation, audits all 120
terminal receipts and queues, and marks the activation ready only after clean
acknowledgement. Its support response advertises token embedding and hidden
RMS normalization as input/layer primitives; neither is token generation.

The hidden-width RMS path is now manifest-bound and dispatched by the resident
worker. Core 0 computes the reference-compatible scale and publishes a
sequence/checksum-validated 64-byte record in an unused activation group.
Every core waits for that record, reads its own input and learned-weight panel,
and writes its disjoint output panel. The builder binds only F32 hidden
normalization weights, distinct activation slots, audited bank routes, and a
scratch group beyond the 2560 output rows. The root-only `RMS_NORM` control
uses the same invocation readback, fleet receipts, queue audit, clean
acknowledgement, and activation commit as embedding and projection.

ASan/UBSan fixtures cover exact hidden RMS math, descriptor validity and
all-120-core output coverage, worker simulation, authority commit, and a
resident-service sequence containing projection, embedding, and RMS. The
RV32IMF role compile passes without A atomics or unresolved runtime helpers.
The offline driver60, reload-guard, and default-control builds pass; the
driver60 binary SHA-256 is
`de0d340564d71168b8909e0f38ee061f5a7ea120374f622d94cd2d7044c0040c`, and the
service-image text SHA-256 is
`025c0012bbc31697ddd181cc802dc796e5ced0cfb2431a699587c3ffec93ce7e`.

The rewrite model-bind ABI now carries a separate, validated graph-workspace
sidecar while keeping the retained two-buffer model-layout ABI unchanged. The
sidecar reserves an FP32 residual tensor (Hidden rows) and an FP32 up-projection
tensor (Ffn rows) after the two existing Ffn-sized activation spaces. Its
builder replays the pinned activation allocator and checks exact placement;
model bind checks sequential bank extents, untouched page-ledger state, and
the existing per-bank workspace limit before authority retains the immutable
sidecar. ASan/UBSan activation-plan and dataflow-authority tests pass, and the
GGUF manifest test compiles with positive and tampered-sidecar checks. The
driver60, reload-guard, and default-control builds pass; the updated driver60
binary SHA-256 is
`bbaa3cfab9d9a9c5333dc32c091162879619a2fd323980d836cf51a6d26d3fd7`.

The projection control now accepts the graph Up-output selector, but the
authority admits it only for an exact layer Up tensor, an initialized
activation input, and the validated sidecar row space. The normal 120-core
projection invocation and Tensix path are reused; begin, physical audit, and
commit track Up-scratch readiness separately. ASan/UBSan authority checks
cover this output path, and the driver60, reload-guard, and default-control
builds pass. The updated driver60 SHA-256 is
`f7e22d02d4269000aca4ef7750b84db45051b3b0a5f792fda536956ed3b7299d`.

The device-side SiLU×Up operation is implemented and pushed in `8ed176883c`. Its
manifest-bound program divides all 576 sixteen-row panels among all 120
resident workers. Each worker reads gate and Up panels over NoC, computes
SiLU(gate)×Up on the device, and writes the gate activation in place. The
root-only `SILU` control validates that the exact Up projection and gate
activation are ready; its all-core owner stages/readbacks descriptors, checks
terminal receipts, runtime state, and drained queues, then commits the gate
activation. An error invalidates the gate and Up readiness before recoverable
service close. No host inference or fallback path was added.

ASan/UBSan math, descriptor coverage, worker simulation (each panel written
exactly once), authority, and 120-core owner checks pass. The RV32IMF target
compile passes without A atomics or unresolved runtime helpers. Offline
driver60, reload-guard, and default-control builds pass; the driver60 SHA-256
is `d8435142c513f9b94290a706a17d440449fea8b8dd957c27ece2bf128a9873f2`.
These checks do not include a physical run. The next rewrite step is the
residual update. The full GGUF manifest test was not run because no local
pinned-model path was available.

This remains a rewrite prototype. The recurrent/dense model graph, state
updates, tied-vocabulary readout, tokenizer client, and generation loop are
not integrated. No model has been loaded on the Pioneer, no rewritten
projection or embedding has run physically, and there is no valid generated
text, decode rate, or physical unload result yet. The Pioneer remains
untouched. Work remains focused on completing this rewrite before benchmark
and assistant integration.
