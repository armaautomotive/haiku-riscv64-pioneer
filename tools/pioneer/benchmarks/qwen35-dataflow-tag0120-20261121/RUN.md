# Qwen3.5 dataflow rewrite tag0120 hardware run

Status: one guarded diagnostic was completed; no valid model output was
produced. This is failure evidence for the device-program rewrite, not a
throughput benchmark. The two-warm benchmark remains gated.

## Preflight and setup

The prior diagnostic process had exited before the card was touched. The
tag0119 recovery helper had hot-restored stable driver55 at generation2, and
the stable driver reported idle before tag0120 was staged. The model files
were present at their expected sizes: Qwen3.5-4B `2740937888` bytes and the
preserved Qwen3.8-27B `18973870528` bytes. The candidate and client matched
the tag0120 build hashes recorded in [`BUILD.md`](BUILD.md). The verified
hardware handoff was Fedora Linux on the existing NVMe to Haiku on that NVMe
through kexec. No SD-card workflow was used.

## One-run diagnostic

- Authentication: `99.767714` s
- Prompt tokenization: `12.726700` s
- Model load/residency: `337.246020` s; `2740498432` resident bytes
- Prompt processing: failed after `0.612460` s
- Decode: `0` s; no token produced, so tokens/s is unavailable

The first core0 embedding read failed. NoC0 completion counter index36 read
`0x00029066`, expected `0x00029067`; completion mask was `0xfe`, busy
initiator mask was `0x0`, and outstanding transaction-ID mask was
`0x00000001`. The read request receipt was address `0x04185380`, route
`0x312`, `3392` bytes. The failed request was not retried and no counter was
cleared.

Generation returned result31 at stage4, state2, recovery1, output0. The
recovery audit returned result13; owner result8; core0 reported worker state4,
failure7, phase5; audit stage6/core0/detail1508. Normal close failed with
result15 at the stop audit. The client exited, but the guard reports candidate
tag `2026112100010120`, generation3, `cold1 active_owner1`. The exclusive
recovery lock and stable55 restore backup were retained. Do not reload or issue
more accelerator work while the latch and active owner remain. No read retry,
counter clear, Tensix reset, or host reboot was performed after the failure.

Both model files were rechecked and remain intact at their expected sizes.
No valid output, decode rate, warm-run result, or clean unload exists. The
100 decode tokens/s goal remains unmet. The in-place recovery state machine
must be implemented and validated before another model diagnostic. Current
source research and the SD-free kexec path are recorded in
[`QWEN35_TTMETALIUM_REWRITE_STATUS.md`](../../QWEN35_TTMETALIUM_REWRITE_STATUS.md).
