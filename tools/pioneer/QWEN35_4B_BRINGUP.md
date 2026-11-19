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
token evaluation. Haiku also measures these boundaries with its monotonic
clock and reports decode tokens per second from those observations. Model
authentication and resident upload have separate host-side timings.

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
The Haiku Work driver has a separate tag226 and only exposes the resident
upload and 4B Work interfaces. It checks the complete manifest against the
bank upload ledger, snapshots the ROOT, and verifies the installed program
and request before release. It keeps the weights resident across requests.
Each completion checks the request ID, token count, terminal PC and quiet
NoC before holding the ROOT. Explicit STOP restores the snapshot before
the seven-bank session can release its device and host resources.

The separate CPU reference used Haiku llama.cpp `c060ca974c77` with
`--device none -ngl 0 --temp 0 --seed 1 --no-conversation --no-warmup`.
It independently rehashed the pinned source and exited 0. The raw prompt
`The capital of France is` encoded to `[760, 6511, 314, 9338, 369]`.
Its eight-token continuation began with ` Paris.\nA. True\nB`, where
`\n` denotes a newline. The Haiku logs are in
`/boot/home/pioneer-qwen35-4b-225/q35-reference.cafSzCza`. SHA-256 of the
complete stdout (including echoed prompt and final newlines) is
`a6657e5aa60fcce061c20560c8dd1a0815c24fbae035324edc50de892bf564a8`.

```sh
python3 -B tools/pioneer/build_qwen35_work.py HAIKU_BUILD NEW_OUTPUT
```

The full host preload test covered 3,450 spans and 2,740,498,432 bytes at
context 128. It also exercised incomplete uploads, missing immutable pages,
state aliases and a shifted RoPE reservation. The driver owner passed a
two-request simulation with exact restoration and retained ownership after
a simulated device fault. The Haiku client's vocabulary-only check matched
all five reference token IDs. The library reported that it skipped tensors.
The Work client creates no CPU inference context.

## Two resident Work runs on Haiku

The November 17–18, 2026 trial used tag226 built from
`490063a12174666d5e9c4cdd6caf02f2944138f8`. Before the exchange, the prior
diagnostic had exited and the stable driver reported generation 4,
`cold0 active_owner0 guard1`. The candidate loaded one copy of the model
and ran the same five-token prompt twice at context 128. Both runs returned
` Paris.` as token IDs `11751,13`, matching the two-token prefix of the
independent CPU reference.

| Warm run | Prompt tokens | Prompt processing (s) | Output tokens | Decode (s) | Decode (tokens/s) |
| --- | ---: | ---: | ---: | ---: | ---: |
| 1 | 5 | 2024.599319 | 2 | 564.955529 | 0.003540 |
| 2 | 5 | 2024.595147 | 2 | 564.955576 | 0.003540 |

The single model load took 195.079089 seconds. File authentication took
68.390657 seconds, and vocabulary preparation and tokenization took
12.634251 seconds. Decode timing starts at the end of prompt processing
and includes the first tied-vocabulary projection. This build runs bands
serially on one Tensix core. The workload is a short functional comparison.

The client exited 0 after explicit STOP and BANK_END and reported
`unload_clean=1 state=2 active=0 cold=0`. The original trial wrapper stopped
at its process check after the clean unload. A corrected process check
examines the executable name. The guarded restore was then performed
separately from the finished Work run. A new postflight verified the
original stable driver SHA-256, generation 6, `cold0 active_owner0 guard1`,
64 CPUs, full RAM and an absent lock. The original 27B model and setup
were preserved.

The seven captured records were checked against the independent Haiku
hashes archived in [the postflight log](benchmarks/qwen35-work-tag226-20261118/postflight.stdout).
Exact timing, IDs, output and closure are in [the Work stdout](benchmarks/qwen35-work-tag226-20261118/work.stdout).

- Driver SHA-256 `7f1fd5267933967d49eeac256cbf6258f1714381401efce3f1b47c4657498d0f`
- Client SHA-256 `68e84fb427776345e5745c8b46d97e0df0a7cc2df796eca8a3290c80b13e40bb`
- ROOT text SHA-256 `27fe7bbeb544d8b317d9013ca3646d52ae259be00cc2bcc78fe86082653be247`

## All-core Work follow-up

The November 18 tag227 trial passed its preflight: the previous diagnostic
had exited, the stable driver was idle, and the original 27B model was
preserved. Tag227 authenticated and uploaded all 3,450 spans (2,740,498,432
bytes); model load took 222.101549 seconds. Its first Work request parked at
the fixed fault loop (`PC 0x40060`) with zero committed tokens. It produced
no valid output or decode-rate measurement. The failure log is in
`benchmarks/qwen35-work-tag227-20261118`.

Tag228 adds failure-stage diagnostics and schedules each projection wave
across the tensor's bank fragments together. This matters because the packed
model stripes rows across seven banks; dispatching each bank fragment
separately left many Tensix cores idle on medium projections. Each worker
still receives at most 16 contiguous row groups from one bank fragment. The
ROOT plus all 119 peers receive work whenever at least 120 groups remain;
the test verifies complete nonoverlapping coverage and seven-bank
participation for balanced layouts. The schedule tests pass under ASan/UBSan,
and the RV32 image cross-build rejects integer division/remainder opcodes.
Work protocol version 2 reports completed full-core waves and total projection
waves in the request receipt. The client and trial wrapper require at least
one completed full-core wave in each warm run, in addition to
`tensixProjections=1` and `hostInference=0` support. The 120-core dispatch is
internal to the matrix schedule.

Current tag228 offline build hashes:

- Driver SHA-256 `04ddf2e6a69f7d19aa1f2ac0a932031f3b60cbebc44c42856589fc9503aafe0c`
- Work client SHA-256 `c588a5cafd5a76e09ddf71a00139da783e6b9754850914e9881d9a86694cf513`
- ROOT text SHA-256 `052f8fc11d760f03ccb5aa0512d18c8f73e0c2b7aa38f1460a40b434a8095b2b`

## November 18 all-core follow-up

Tags228–233 were built and tried under guarded driver swaps. Each run started
after the previous client exited and stable041 was verified idle; failures
that left the candidate cold/owned were recovered through one controlled
power cycle and the pinned SD-free Haiku path. The 27B model remains intact.

- Tag228 stopped before token commit at stage `0x54000201`.
- Tag229 stopped before token commit at stage `0x54000211`, detail
  `0x46503851` (`Q8PF`).
- Tag230 stopped before token commit at stage `0x54000216`, detail
  `0x01073620` (worker1, peer PC `0x73620`).
- Tag231 faulted at `pc=0x40064` before the dispatcher and had no dispatcher
  checkpoint.
- Tag232 traced to `0xda000077` immediately before the dispatcher call; it
  failed before dispatcher entry.
- Tag233 added an entry stub and reached the dispatcher. It failed at stage
  `0x54000350`, detail `0x010723c4`: peer worker1 reported reason `0x50`,
  `panels != count` after `q38_compute_once()`. The run authenticated the
  model in 100.890229 s, tokenized the 18-token prompt in 13.185885 s, and
  loaded/uploaded 3,450 resident spans (2,740,498,432 bytes) in 224.100909 s.
  No prefill, decode, token commit, valid output, or rate was produced.

Tag233 logs and recovery evidence are archived in
`benchmarks/qwen35-work-tag233-20261118`. Its recovery restored stable041 at
generation2 `cold0 active_owner0 guard1` and removed the shared lock. The last
observed Haiku preflight had all 64 CPUs and full RAM, both 4B and 27B files
intact, no inference process, and stable041 idle.

Tag234 fixed the suspected scale-cache address for the Qwen35 peer and added
actual/expected panel counts on peer refusal. It instead failed earlier at
`StageBand` reason 14 (`q35_dispatch_matrix()` returned an unexpected worker
count); the root checkpoint remained empty, so it did not verify the scale
change or reach the peer count diagnostic. It authenticated in 100.234451 s,
tokenized the 18-token prompt in 13.322161 s, and loaded/uploaded 3,450 spans
(2,740,498,432 bytes) in 220.861532 s. No prefill, decode, token, valid output,
or rate was produced. See `benchmarks/qwen35-work-tag234-20261118`.

Tag234 recovery restored stable041 at generation 2 (`cold0 active_owner0
guard1`) with the shared lock absent. The 27B setup remains intact. The next
step is the tag235 receipt follow-up, described below. Do not assume the
tag233 scale-cache change was exercised. Two valid warm outputs, separate
load/prompt/decode timing, clean unload, and at least 100 decode tokens per
second remain the completion criteria.

Tag235 added a failure-only receipt in ROOT progress words and made the Work
owner prefer it over an empty or stale L1 checkpoint. Its guarded run again
started from stable041 generation 2 idle, with the old diagnostic exited,
shared lock absent, 64 CPUs/full RAM, and both model files intact. It
authenticated in 98.691868 s, tokenized the 18-token prompt in 13.515672 s,
and uploaded 3,450 spans (2,740,498,432 bytes) in 225.917047 s. The dispatcher
receipt `stage=0x54f002da detail=0x0000000a` maps to source line 730's padded
scale check, first rejected block 10. The run failed before prefill/token;
the dispatcher returned 0, `StageBand()` reported reason 14, and the candidate
was cold/owned. No output or decode rate was produced. Tag235's logs and the
recovery are in `benchmarks/qwen35-work-tag235-20261118`.

Recovery completed through the saved single-cycle relay procedure and pinned
SD-free Linux-to-Haiku path. Stable041 is again idle at generation 2 and the
shared lock is absent; the 27B setup is intact. Next capture the scale bits,
tensor type, and input width on that failure-only path, then diagnose the
scale-cache contents before choosing a behavioral fix. Do not erase the bad
scale tail in the dispatcher, since that would hide the reason it is wrong.

Tag236 added failure-only scale bits and packed tensor type/input width to
the receipt. Strict preflight passed from stable041 generation 2 idle, with
the prior diagnostic exited, shared lock absent, 64 CPUs/full RAM, and both
model files intact. Authentication took 100.712738 s, prompt tokenization
took 13.525346 s for 18 tokens, and model load/upload took 227.309557 s for
3,450 spans (2,740,498,432 bytes). It failed before prefill at
`stage=0x54f002df detail=0x0000000a context0=0xbf61fd40
context1=0x0d000a00`: the first padded scale slot (block10) for type13,
width2560 held a nonzero scale. No output or decode rate was produced.

The compiled StageInput has the expected six-word tail-zero loop; its
640-byte frame and nested 2,112-byte quantizer frame are far from the scale
cache at `0x94000` (ROOT stack starts at `0x70000`). This does not prove the
on-card store or exclude a later writer. Tag236 was recovered through the
single-cycle relay and pinned SD-free Linux-to-Haiku path after fixing two
stale tag35 checks in its recovery helper. Stable041 is idle at generation2,
the shared lock is absent, 64 CPUs/full RAM are available, and both model
files remain intact. See the archived build and run in
`benchmarks/qwen35-work-tag236-20261118` and the next-session handoff.

## November 18 rebootless Work recovery follow-up

Tag245 passed guarded preflight and loaded the verified 4B model (3,450 spans,
2,740,498,432 bytes). Authentication was 101.414734 s, prompt tokenization
12.996782 s, and model upload/load 225.714405 s. ROOT faulted before prefill
with zero commits at stage `0x54000102`, detail `0x77`. Peer1 reported the
Q8PF fault marker; tag245 correctly refused unsafe cleanup, leaving candidate
generation3 `cold1 active_owner1`. There was no valid output or decode rate.
Exact hardware logs and hashes are preserved under
[`qwen35-work-tag245-20261118/evidence`](benchmarks/qwen35-work-tag245-20261118/evidence).

Tag246 added rebootless fleet recovery for an explicit parked Q8PF peer fault.
The hardware run loaded the 4B model in `225.435951` s after `99.030119` s
authentication and `12.659907` s prompt tokenization, then faulted before
prefill with zero committed tokens. Its STOP proof rejected the Work peer's
park at `0x720a4` because it only recognized the generic `0x7209c` park; the
candidate remained quarantined (`cold1 active_owner1`) with no output or clean
unload. The logs are preserved in
[`qwen35-work-tag246-20261118/evidence`](benchmarks/qwen35-work-tag246-20261118/evidence).

Tag247 corrected the first park address, but its hardware run still returned
generic proof17 for Q8PF peers and remained quarantined. Tag248 added a second
sealed local branch for safe frontend PC samples and field-specific peer
status checks. On hardware, tag248 authenticated in `100.143765` s, tokenized
in `15.216576` s, and loaded all 3,450 spans (`2,740,498,432` bytes) in
`224.792770` s. ROOT then faulted before prefill with zero commits while
waiting on the worker119 NoC transfer (`stage=0x54000102 detail=0x77`).

Tag248 passed both safe-idle proofs, unloaded cleanly, and restored stable041
by hot reload at generation4 (`cold0 active_owner0 guard1`) without rebooting
the board; the shared lock is absent. This verifies driver recovery without
reboot. No valid output or decode rate was produced. The exact trial and
postflight evidence is in
[`qwen35-work-tag248-20261118/evidence`](benchmarks/qwen35-work-tag248-20261118/evidence).
The 27B model and both model files remain intact. Two valid warm outputs and
100 decode tokens/s remain the completion criteria.

Tag249 enabled the bounded-transfer timeout receipt but still faulted at
`stage=0x54000102 detail=0x77` on worker119 before prefill; the diagnostic
`0x58` timeout record was absent. It authenticated in `68.086098` s, tokenized
in `13.609946` s, and loaded the full model in `222.687418` s. It unloaded
cleanly and hot-reloaded stable041 at generation6 (`cold0 active_owner0`)
without reboot. Tag250 now preserves the transfer identity before the
doorbell so an early fault cannot erase it. Its offline cross-build passed
with warnings as errors; the guarded hardware trial is next. This diagnostic
build is not for performance measurement. No valid output or decode rate has
been measured. Tag249 evidence is in
[`qwen35-work-tag249-20261119/evidence`](benchmarks/qwen35-work-tag249-20261119/evidence).

## Haiku llama.cpp platform patch

The local `src/libs/llama.cpp` worktree also contains Haiku platform support
used by the Pioneer client build: Haiku cache/settings directories and
thread-affinity handling. The source delta is preserved as
[`llama-c060ca974c77-haiku-work.patch`](patches/llama-c060ca974c77-haiku-work.patch)
against the pinned submodule commit `c060ca974c773c7c3d17fd1b66dc9d312bc292c0`.
To reproduce that source state in a clean checkout, run:

```sh
git -C src/libs/llama.cpp apply tools/pioneer/patches/llama-c060ca974c77-haiku-work.patch
```

This patch records the existing platform delta; a clean-room rebuild of the
full `libllama.so` from it has not been qualified in this bring-up.
