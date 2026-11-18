# Qwen3.5-4B on Pioneer P100A

November 17, 2026: switch development to the smaller dense model. Keep the
27B model and driver work intact. The 27B GitHub checkpoint is `ec4b44ffe9`;
the tag224 diagnostic already running on the Pioneer was allowed to continue
without touching its device, driver, or model files. The 27B goal is paused.
Do not run a 4B card trial while the 27B run owns the card or the driver is
quarantined. A terminal result and independent idle stable-driver check are
required first.

## Pinned 4B development model

- Base: [`Qwen/Qwen3.5-4B`](https://huggingface.co/Qwen/Qwen3.5-4B), Apache 2.0.
- Quantized text model: [`unsloth/Qwen3.5-4B-GGUF`](https://huggingface.co/unsloth/Qwen3.5-4B-GGUF/tree/e87f176479d0855a907a41277aca2f8ee7a09523),
  `Qwen3.5-4B-Q4_K_M.gguf`, revision `e87f176479d0855a907a41277aca2f8ee7a09523`.
- Size: 2,740,937,888 bytes. SHA-256:
  `00fe7986ff5f6b463e62455821146049db6f9313603938a70800d1fb69ef11a4`.

The full download matched the publisher's SHA-256. The independent local
preflight rehashes the entire file and validates the 426 descriptors on the
same open handle. No vision projector is required for this text target.

```sh
python3 -B tools/pioneer/qwen35_4b_p100a_preflight.py /path/to/model.gguf
python3 -B tools/pioneer/tests/qwen35_4b_p100a_preflight_test.py /path/to/model.gguf
```

The upstream config and verified GGUF agree on 32 layers, one full attention
layer every four, and widths of 2560 (hidden), 4096 (recurrent output) and
9216 (FFN). The model's embedding and output head share the same Q6_K tensor;
the 27B has separate tensors. The 4B model is about 6.9 times smaller on disk,
but this is not a speed estimate. Its GGUF has 48 Q5_K recurrent projection
matrices that the existing 27B role does not support. Reading the alternate
Bartowski Q4_K_M header located 16 Q5_K tensors there too; it is a different,
larger artifact and was not selected or loaded. The source should stay fixed
for matched comparisons.

## First card milestones

1. Use real 4B bands to validate the existing Tensix Q4_K/Q6_K/Q8_0 math
   at the three new input widths. Add a lossless Q5_K operand path and prove
   it against the original quantized coefficients and independent CPU
   reference in the separate validation target. No CPU inference in Work.
2. Bind the small ROOT/worker layout, 24 recurrent and eight full-attention
   layers and tied-vocabulary readout. Run a short exact-model comparison
   to verify prefill, decode, state, and clean closure before tuning speed.
3. Measure repeated warm 4B generation separately from authentication and
   load. Promote any change to Assistant only after it passes that test.
   Keep tuning applicable kernels at real 27B widths before resuming that
   goal; a result on the 4B does not establish a 27B rate.

Checkpoint `94117b2279` covers download, authentication, and layout. At that
checkpoint the 4B driver, firmware and inference were still pending.

## Off-card packed-panel preparation

`PioneerQwen35Q5Panel.h` expands an original Q5_K block into a single
16-by-16 integer-overlay operand and retains exact affine coefficients.
The tag225 diagnostic uses it on the device, as described below.

```sh
python3 -B tools/pioneer/tests/run_pioneer_qwen35_panel_tests.py /path/to/model.gguf
```

The test hashes and inventories the original file on the same open handle,
checks its identity through sampling, and compares sampled packed panels with
separate scalar GGUF row calculations. With the pinned file it passed under
ASan/UBSan for 512 synthetic Q5_K tiles and, from every quantized tensor,
432 Q5_K, 1,179 Q4_K, 198 Q6_K and 432 Q8_0 real tiles. These are sampled
host-side checks, not Tensix execution, model accuracy, or proof of a usable
4B driver. Card validation of these formats at the new widths, the 4B layout,
full-model state and two warm runs remain required.

`PioneerQwen35Gguf.h` and `PioneerQwen35TiledReader.h` are separate packed
source and lossless page-permutation components; neither is a Work backend.
The reader checks all 426 roles and ties the output head to the original Q6_K
embedding. Its caller must authenticate the complete source on the same open
handle before use. The following test rehashes the original, checks the first
and last 16-row groups of every quantized tensor (5,184 pages), and tests
malformed sparse headers and a changed source under ASan/UBSan. The separate
`PioneerQwen35PackedPlan.h` proposes seven-bank weight and state storage,
allocates the tied tensor once and checks every fragment for bounds and
coverage. Those proposed addresses and state shapes are not yet qualified
device allocations or proof that a worker uses the right layout:

```sh
python3 -B tools/pioneer/tests/run_pioneer_qwen35_gguf_tests.py /path/to/model.gguf
```

The reader/plan test translation unit also cross-compiles for Haiku riscv64
with warnings as errors. The isolated Q5 panel compiles freestanding for the
RV32 Tensix controller with no undefined symbols. Neither result executes it.

Tag224 subsequently terminated with a worker counter failure, both exit codes
1 and `RESTORE_NOT_CONFIRMED_DO_NOT_RETRY_OR_RESET`. Its parent and worker
were absent from a later process listing; the shared lock was retained and
the installed driver file was still tag224's SHA-256
`76f7448d3d4fe29247c032e84f40ee5393ee3e0203757edd2daeb49d2172d3b2`.
The nine copied records in `benchmarks/qwen38-tag224-failed-20261117` matched
independent remote SHA-256s. The failed run produced no valid output or
clean END.

## Tag224 recovery and 4B projection validation

One front-panel OFF hold followed a successful Haiku sync. After the off
interval, pinned SSH and ICMP were unreachable. One ON pulse returned to
Fedora with the expected P100A PCI function and a clean Samsung BFS superblock.
The pinned SD-free helper returned to Haiku with 64 CPUs and 137,034,936,320
bytes of RAM. The recovery matched all nine tag224 logs, retained the failed
candidate, and restored the original driver. A separate pinned-SSH postflight
verified SHA-256 `041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403`,
tag `2026102100010012`, generation 2, `cold0 active_owner0 guard1`, and an
absent trial lock.

The 4B projection diagnostic has its own tag225 driver, ABI 3 and firmware
marker. It reuses the checked Tensix rows engine while admitting only the eight
format and input-width combinations in this GGUF. The Haiku validator
authenticates the full pinned model before opening the device, samples a real
32-row band at each combination and checks projection output and bank closure.
Its host arithmetic belongs to this diagnostic. Work mode remains a separate
full-model target.

```sh
python3 -B tools/pioneer/build_qwen35_projection_validation.py HAIKU_BUILD NEW_OUTPUT
```

Software replay of the linked image passed all eight real bands against a
scalar GGUF reference. The transport tests passed 168 shape/bank/row-group
combinations and 15 injected faults. The original 27B checked rows image and
client regressions passed. `PioneerQwen35Embedding.h` also decodes one lane
from the original tiled Q6_K data shared with the output head. Its resolution
and decode matched 21 complete sampled token rows across all seven banks.

The physical tag225 run on Haiku passed all eight real 32-row projections
with zero bit mismatches. Both Q5_K inputs (2560 and 4096) passed. Each receipt
verified completion and stop with no cold recovery flag. The validator exited
0 with an explicit bank END. A separate pinned-SSH postflight verified the
original driver hash, generation 4 idle, no trial lock, 64 CPUs and full RAM.
The staged model at `/boot/home/pioneer-qwen35-4b-225/Qwen3.5-4B-Q4_K_M.gguf`
matched the pinned hash before the trial and was rehashed by the validator.

The physical logs are in `benchmarks/qwen35-projection-20261117`. All four
archive files matched the remote postflight hashes. Checked projection calls
took 49–251 ms per band, including staging and receipt checks. The raw log
records sample upload and CPU reference timings separately. Full-model load,
prefill, token decode, and the two warm Work runs still need measurement.

Driver SHA-256: `175b8240c548f54ddf83a7755b39ef54cbfdd54bb43366af7cac2c3bb5cadf67`.
Validator SHA-256: `bc4f4e161d1c9ccff409ae2acddb1b3799b114928bac2cad9a5a90742a74ea25`.
Tensix text SHA-256: `d2494fd4ef2f768a0ad3a213438389285fd12b707880e900e48204158a817989`.

`PioneerQwen35FleetPlan.h` divides all 249 quantized matrices into bounded
bands for the ROOT and its workers. Its host test checks complete row
coverage, nonoverlap and each band's placement inside its packed fragment.
The bands still need live worker and ROOT integration, including sampler
and per-request state closure.

## Standalone 4B ROOT

The experimental ROOT compiles independently of the 27B image. Its token
loop covers the 24 recurrent and eight dense attention layers, learned RMS
normalization, FFN, tied Q6_K embedding and greedy readout. It invokes the
tag225 Tensix matrix loop for learned projections, visiting one bank-local
band at a time on the ROOT. Scalar operations and state updates run on the
same device. Recurrent state is double-buffered by position, and the dense
attention cache exposes only the current request's causal prefix. Each
request starts at position zero with the same immutable weight manifest.

The request receipt records prompt cycles through the final prompt layer.
Decode cycles begin with the first vocabulary readout and include subsequent
token evaluation. Model authentication and upload are host-side timings
outside these intervals.

```sh
sh tools/pioneer/build_qwen35_work_image.sh HAIKU_BUILD NEW_OUTPUT
c++ -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -fno-omit-frame-pointer tools/pioneer/tests/pioneer_qwen35_token_core_test.cpp \
    -o /tmp/pioneer-qwen35-token-core-test
/tmp/pioneer-qwen35-token-core-test
```

Two local builds produced the same RV32 images and had no unresolved symbols.
The test exercises recurrent and dense operators against independent scalar
references, traverses the exact matrix schedule, and checks fresh state and
commit behavior across two synthetic requests. The authenticated GGUF test
checks the serialized manifest against the host placement of every projection.
The ROOT still needs a Haiku owner with checked upload, boot, request, and
restore operations before live model generation can be tested.

The separate CPU reference used Haiku llama.cpp `c060ca974c77` with
`--device none -ngl 0 --temp 0 --seed 1 --no-conversation --no-warmup`.
It independently rehashed the pinned source and exited 0. The raw prompt
`The capital of France is` encoded to `[760, 6511, 314, 9338, 369]`.
Its eight-token continuation began with ` Paris.\nA. True\nB`, where
`\n` denotes a newline. The Haiku logs are in
`/boot/home/pioneer-qwen35-4b-225/q35-reference.cafSzCza`. SHA-256 of the
complete stdout (including echoed prompt and final newlines) is
`a6657e5aa60fcce061c20560c8dd1a0815c24fbae035324edc50de892bf564a8`.
