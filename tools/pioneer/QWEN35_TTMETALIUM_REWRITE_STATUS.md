# Qwen3.5 Tenstorrent-style code rewrite

## Scope

Rewrite the Qwen3.5 Work execution path around a device program with per-core
reader, Tensix compute, and writer roles; double-buffered local circular
buffers; sharded intermediate activations; and no host inference path. Keep
the checkpoint history on `codex/qwen35-4b-p100a`.

During this phase, work only on rewrite code and its offline code checks.
Defer Pioneer benchmark runs and assistant integration until the device-program
rewrite is implemented.

## Progress

- 2026-11-20: Added `PioneerQwen35DataflowKernels.h` with separate reader,
  Tensix-compute, and writer role contracts over two-page single-producer,
  single-consumer local queues. The compute template requires an explicit
  Tensix-matrix-engine capability, and each role validates its core's shard
  descriptor before touching device data. Role setup errors publish a queue
  failure so a neighboring role can leave its bounded wait.
- The reader packs each 256-value activation chunk once in local memory and
  reuses that packed chunk across its assigned output groups. Q8_0 weight
  blocks map eight-at-a-time to the same 256-value activation chunk.
- Expanded the offline program test to compile all three role interfaces and
  check 120-core shard coverage, queue address binding, queue publish/consume,
  ticket wraparound, in-place activation packing, Q8 block mapping, and
  malformed-descriptor rejection. The host test passes with AddressSanitizer
  and UndefinedBehaviorSanitizer.
- The existing Work dispatcher and ROOT gather path are retained as the
  checkpoint; no changes to that path are part of this rewrite phase.
- Added `PioneerQwen35DataflowProgram.h`: a 120-core program descriptor and
  builder that maps each projection shard to its weight bank, sharded
  activation input/output ranges, and double-buffered reader/compute/writer
  circular-buffer layout.
- The role operations still have no Blackhole BRISC/TRISC/NCRISC backend,
  image build, or resident program owner. They are contracts only; this is not
  a runnable Work path or a throughput result.

## Rewrite gates

1. Complete offline descriptor, queue, and role contract checks.
2. Implement the Blackhole device operations and build independent reader,
   Tensix compute, and writer images with a concurrent launch protocol.
3. Connect those images to a resident Qwen3.5 program owner and preserve
   sharded activation outputs across dependent operations.
4. Complete the offline target build and correctness gates, then proceed to
   the hardware benchmark and assistant integration phases.

The latest pushed checkpoint before this rewrite is `1b2b57c561`.
