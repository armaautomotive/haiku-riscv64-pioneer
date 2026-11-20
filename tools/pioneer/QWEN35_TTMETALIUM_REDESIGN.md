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
