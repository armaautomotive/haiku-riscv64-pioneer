# Qwen3.5 Work: Tenstorrent device-program redesign

This note records the implementation change requested after the Tag75 timing
showed that local command-register tuning was not addressing the bottleneck.

## Upstream pattern

TT-Metalium builds a device `Program` from per-core reader, compute, and writer
kernels, circular-buffer definitions, and runtime arguments. The reader moves
tiles from device DRAM to local SRAM, the compute kernel feeds the Tensix
matrix engine, and the writer streams results back out. Circular-buffer
producer/consumer synchronization lets data movement overlap matrix work.
Programs are submitted as device workloads; fast dispatch queues them on the
device. TT-NN/tt-transformers then retain device weights and KV cache and use
decode-specific kernels and cached operation programs.

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
Port the TT-Metalium device-program pattern into the freestanding Blackhole
images:

1. Give each Tensix core a stable shard of a tensor and a persistent program
   entry. The program receives a projection or fused-operation descriptor,
   derives its row range from its core ID, and processes a tile loop locally.
2. Split each core's dataflow into reader, compute, and writer roles. Use
   double-buffered local SRAM circular buffers; overlap quantized-weight and
   activation reads with Tensix matrix operations and result writes.
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

The public TT-Metalium C++ runtime is not currently used by this Haiku driver;
the implementation work here is to port its device-side programming pattern,
not to introduce CPU inference or a host fallback.

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
workspace regions already reserved by `PioneerQwen35PackedPlan`. It page-aligns
each bank fragment, stripes 16-row groups across banks, supports resolving a
row range only when it stays inside one fragment, and tracks allocated ranges
to reject overlap. The initial use is a pair of ping-pong row buffers large
enough for the tied vocabulary projection. Its bounds/coverage test passes.
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
