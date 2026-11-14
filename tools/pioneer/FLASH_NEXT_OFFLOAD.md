# Qwen3.8 Flash-Next Q4: one P100A plus Pioneer host memory

October 20, 2026. Initial user-space offload implementation; **not an installed
driver, complete decoder, Assistant backend, or native performance result**.

The first concrete target is Unsloth `UD-Q4_K_XL`. The reproducible repository
revision, four full-shard SHA-256 hashes, lengths, and observed directory totals
are in [flash_next_q4_manifest.json](flash_next_q4_manifest.json). These pins came
from the publisher's Hub API. Only the metadata shard and bounded header ranges
of the other three shards were downloaded for this work. The other three full
weight files have **not** been downloaded or authenticated here.

## Implemented

`tenstorrent_flash_next_preflight.py` inventories local split GGUFv3 files and
creates a packed-weight memory proposal. It handles the zero-tensor metadata
shard, split ordinals/totals, tensor extents, duplicate names, architecture
geometry, all 144 separate routed-expert tensors, and the n-gram head ranges.
It accepts bounded header captures only with explicit full file lengths.
No templates are evaluated and no device is opened.

`PioneerFlashNextOffload.h` implements packed host expert execution and an
explicit static residency controller. Gate/up/down execute as
`down(silu(gate(x)) * up(x))`; the outer graph supplies ten distinct expert IDs
and normalized routing weights. Pinned experts use the logical device-session
interface. Other experts deliberately execute on the CPU. A device error does
not switch to CPU execution. All ten contributions finish before output is
published. Cancellation occurs between completed experts. Failed/uncertain
device operations quarantine the session; clean END is explicit.

The CPU implementation is a numerical baseline, using bounded block decoding
and floating-point dot products. It is not yet an optimized threaded RISC-V
expert kernel. It retains packed weights and expands at most one block at a
time, rather than materializing floating-point matrices.

The controller exposes an opt-in `RunRouted` device operation for the case
where all ten experts selected for a layer are already resident. Its contract
is one device transaction that performs expert computation and weighted
combination on-device. The default adapter capability is false, so existing
adapters continue using their per-expert calls; a false/failed execution never
falls back to CPU. Tests exercise the one-call contract and quarantine behavior
with a mock only. The routed controller also reuses fixed hidden-state scratch
buffers across calls rather than allocating them per layer/token. There is
still no native adapter, and no speedup is claimed.

Supported packed arithmetic covers F32, F16, BF16, Q8_0, Q4_K, Q5_K, Q6_K,
Q5_1, and IQ4_NL. This matters for the actual Q4 artifact: 43 down-expert tensors
are Q5_1, five are Q8_0; 94 gate/up tensors are Q4_K and two are Q5_K. A filename
containing Q4 does not mean every tensor uses Q4_K.

`PioneerFlashNextSource.h` binds the 48 layers' packed expert arrays to retained,
authenticated weight owners and exposes zero-copy expert slices. It checks
shape, packed extent, ownership, overlap, and source epochs. The owner is an
interface: a full same-file-description authenticated split-GGUF mapping loader
is still required. A layout inspection alone cannot make an owner authenticated.

The same header implements host n-gram gathering from the graph's 16 hashed row
IDs. Only 16 x 160 values are expanded. With IQ4_NL, that reads **1,440 packed
bytes per gather** from the 28.8 GB table. Hashing, PLE projections and PLE
convolution remain graph responsibilities.

## Observed Q4 layout and capacity proposal

The complete four-shard directory contains 1,224 tensors:

| Packed component | Bytes |
| --- | ---: |
| N-gram table, proposed host placement | 28,800,138,240 |
| Routed experts across all layers | 77,017,907,200 |
| Other weights, proposed card placement | 5,505,584,640 |
| All learned tensors | 111,323,630,080 |

The planner defaults to **21 GiB**, matching the existing qualified seven
3 GiB bank extents, rather than assuming all advertised card memory is already
available to the driver. With a provisional 2 GiB runtime reserve, the original
packed representation permits **99 complete experts per layer** on card and
90,925,911,040 bytes of host-offloaded weights, including the n-gram table.
Which expert IDs to pin should come from workload profiling.

These are byte-accounting proposals, not allocations. Native tiling, alignment,
temporary buffers, context length, and actual decoder state must be accounted
for before native placement. The 2 GiB reserve is not a qualified context limit.
Keeping a complete host mapping/mirror requires more host storage than keeping
only the offloaded weights; the planner reports both cases and a separate host
runtime reserve. Neither memory fit nor active parameter count predicts speed.

Streaming all ten selected experts in every layer could touch roughly 1.50 GB
of packed expert weights per token before cache hits. The initial controller
therefore uses host computation for nonresident experts. A future streaming
policy should be compared using measured transfer cost, CPU expert cost, and
observed expert reuse. The user's AMD/V100 result of about 20 tokens/s is a
comparison target, not a measured or predicted Pioneer result.

Run a read-only plan on a local full shard set:

```sh
python3 tools/pioneer/tenstorrent_flash_next_preflight.py \
  /path/to/Qwen3.8-Flash-Next-UD-Q4_K_XL-0000[1-4]-of-00004.gguf
```

`--card-gib 28` requests a hypothetical full-capacity plan; it does not enlarge
the installed driver's qualified banks. `--full-sizes` accepts one exact full
length per header capture. Neither option authenticates absent model bytes.

## Validation

```sh
sh tools/pioneer/tests/run_pioneer_flash_next_offload_tests.sh
```

The test runner builds only pristine pinned GGML arithmetic from commit
`c060ca974c773c7c3d17fd1b66dc9d312bc292c0` in a temporary directory. It does
not modify or build the dirty llama working tree. ASan/UBSan cover:

- 4,608 packed blocks compared with upstream dequantizers;
- real-size Q4_K/Q5_1 host expert math against a separate constant-matrix oracle;
- mixed host/device routing, budget/duplicate/route refusals, and source epochs;
- logical device BEGIN/upload/run/partial-output/END faults and quarantine;
- cancellation without partial publication;
- source directory and per-expert offset authorization using synthetic extents;
- real packed IQ4_NL gathers compared with upstream, plus malformed head ranges;
- split-header geometry, truncation, duplicate, and bounds refusals in Python.

Device callbacks in these tests are mocks. No native P100A computation,
full-model generation, model quality, throughput, or Assistant readiness is
established by these tests.

## Next integration gates

1. Implement the same-FD authenticated, retained split-GGUF mapping owner and
   feed its real tensor directory into `PackedSource` / `NgramTable`.
2. Add independently qualified native expert transport for widths 2560 and 640
   and the actual Q4_K/Q5_K/Q5_1/Q8_0 formats. The current dense-27B cohort ABI
   is shape-specific and must not be relabeled as Flash-Next support.
3. Connect the `qwen4exp` graph: router/shared expert, hyper-connections, QSA,
   Gated DeltaNet and PLE hashing/convolution. Existing vendored llama lacks
   this architecture; the new controller only supplies routed contributions.
4. Prove full token/state lifecycle, cancellation and resource cleanup, then
   wire the qualified runtime into Assistant selection, loading and generation.
5. Benchmark prompt processing and warm decode separately, including host RAM
   bandwidth, CPU expert cost, card residency hits and actual transfer traffic.

The existing 27B driver/runtime and Assistant selections are not changed by this
initial work. Concurrent changes elsewhere in the checkout belong to the
ongoing 27B/Assistant work.

Architecture/tensor references:
[Qwen model card](https://huggingface.co/Qwen/Qwen3.8-Flash-Next),
[Q4 artifacts](https://huggingface.co/unsloth/Qwen3.8-Flash-Next-GGUF/tree/766911a6b7369840a91dbcd95f9f997acaab6cd6/UD-Q4_K_XL),
[upstream graph](https://github.com/ggml-org/llama.cpp/blob/master/src/models/qwen4exp.cpp).
