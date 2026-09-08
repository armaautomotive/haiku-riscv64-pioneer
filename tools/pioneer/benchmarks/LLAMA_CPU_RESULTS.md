# llama.cpp CPU performance on Haiku / Milk-V Pioneer

Recorded: 2026-09-08.

## Status

All ten test results have been recorded. The benchmark exited successfully
with status 0. Each test includes three measured repetitions.

## Results

Throughput is tokens per second, reported as mean +/- sample standard
deviation across three repetitions. Higher is better.

| Worker threads | Prompt processing (128 tokens) | Generation (32 tokens) |
| ---: | ---: | ---: |
| 4 | 4.583 +/- 0.007 | 3.138 +/- 0.015 |
| 8 | 8.894 +/- 0.026 | 6.001 +/- 0.004 |
| 16 | 15.768 +/- 0.005 | 11.256 +/- 0.015 |
| 32 | 22.778 +/- 0.014 | 11.938 +/- 0.022 |
| 64 | 7.090 +/- 0.296 | 0.143406 +/- 0.000036 |

Scaling is strong through 16 threads. Generation gains little from 16 to
32 threads, while prompt processing continues to improve. At 64 threads,
prompt throughput regresses substantially. Generation at 64 threads is
approximately 83 times slower than at 32 threads: each 32-token repetition
took about 223.14 seconds. All three repetitions were similarly slow.
The cause has not been diagnosed;
these results alone do not distinguish scheduling, synchronization, memory
bandwidth, topology, or other bottlenecks.

These are short, single-sweep baseline tests, not long-context performance
claims or a Linux comparison. Worker counts do not change online CPU counts.

## Hardware and software

- Milk-V Pioneer / SOPHON SG2042; Haiku reports 64 CPUs.
- Haiku boots from Samsung's 111.8 GiB BFS partition,
  `/dev/disk/scsi/0/3/0/0`, mounted at `/boot`.
- Source revision: `c060ca974c773c7c3d17fd1b66dc9d312bc292c0`, with
  the three Haiku compatibility edits in
  [llama-c060ca-haiku.patch](../llama-c060ca-haiku.patch).
- Native GCC 13.2.0 Release build using the CPU_GENERIC backend.
- OpenMP and optional RISC-V vector extensions disabled.
- Native GCC target defaults verified as `rv64imafdc_zicsr_zifencei` and
  `lp64d`: hardware scalar floating-point support is enabled by default.
  CPU_GENERIC does not imply software floating-point emulation.
- The reported 1000 MHz CPU frequency is not independently verified.
- Haiku's clock reports 1970 after boot; JSONL `test_time` values are not
  reliable calendar timestamps. The report date is from the Mac session.

Model: Qwen3-0.6B Q8_0, 596,049,920 parameters; GGUF file size 639,446,688
bytes. Stored at `/boot/home/models/Qwen3-0.6B-Q8_0.gguf`.
Previously verified model SHA-256:
`9465e63a22add5354d9bb4b99e90117043c7124007664907259bd16d043bb031`.

## Reproduction

Runtime directory:
`/boot/home/develop/llama-c060ca974c77/build-pioneer/bin`.

```sh
llama-bench -m /boot/home/models/Qwen3-0.6B-Q8_0.gguf \
  -t 4,8,16,32,64 -p 128 -n 32 -b 64 -ub 64 -ngl 0 \
  -r 3 -o jsonl --progress
```

Warm-up enabled; prompt and generation measured separately. Batch and
microbatch sizes are 64, GPU layers zero, no explicit affinity or NUMA
policy. Other options retain this revision's defaults. The user was asked
to leave the machine idle during measurement.

Raw evidence:

- [Quiet-AHCI results](haiku-qwen3-0.6b-q8-quiet-ahci-20260908.jsonl)
- [Quiet-AHCI progress log](haiku-qwen3-0.6b-q8-quiet-ahci-20260908.stderr)

## AHCI logging change

The earlier run was stopped during its first prompt warm-up, with no timing
samples. The kernel was printing `ahci: sg_memcpy phyAddr` for each copied
segment, often 4096 bytes. Brief CPU samples showed the SCSI scheduler using
nearly a full core while llama workers received little CPU time.

Only that per-segment trace was removed. Error and startup diagnostics remain.
Samsung reboot and installed-package checksum were verified, and the new
boot no longer emitted that message. The kernel and EFI loader binaries
were unchanged. Updated system-package SHA-256:
`6a40c785862ab19913a7cf0b9ef64c90fd5998cda2c22c6636edc8f3463b348a`.

The quiet run produced the results above, but the interrupted noisy run
provides no completed throughput baseline: no numeric logging speedup can
be claimed from this comparison.

## Next experiments

### Initial scaling investigation

A short generation probe (`-t 32,64 -p 0 -n 8 -r 1 --poll 0`, otherwise
the same settings) failed to complete its first 32-thread warm-up before
the subsequent affinity experiment panicked the kernel. This is not a
completed throughput result. Disabling idle polling did not provide an
immediate remedy. Source inspection shows `--poll` controls waiting for
new graphs, not the spin barrier used within graph execution.

Read-only sampling confirmed all 64 CPUs enabled, but approximately three
cores busy while most were idle. Of the 32 benchmark threads, most were
in READY state, and a few RUNNING. All returned zero affinity masks,
which this scheduler treats as unrestricted. This points to runnable-work
distribution as a concrete area to investigate; it does not yet establish
the original 64-thread regression's root cause.

An experiment assigning those workers distinct CPUs through the existing
private affinity syscall triggered a kernel panic in
`_user_set_thread_affinity + 0xe0`, reading address zero. The source
unconditionally dereferences `thread->cpu` in that path. The trace is saved
in [haiku-affinity-panic-20260908.txt](haiku-affinity-panic-20260908.txt).
Do not repeat the `cpu_activity --pin` diagnostic on this kernel. No kernel
or llama source changes were made during this scaling investigation.
The machine is stopped in the kernel debugger and needs recovery before
further runtime tests. Neither probe supplies a valid new benchmark result.

Supporting samples: `haiku-scaling-poll0-top-20260908.txt` and
`haiku-scaling-cpu-activity-20260908.txt`. The CPU sampler was compiled
during the exploratory run, so this was diagnostic activity, not a clean
throughput comparison.

1. Investigate the confirmed high-thread-count regression, beginning with
   worker CPU activity, synchronization and polling behavior. Change one
   runtime setting at a time while retaining the current kernel.
2. Compare Linux using the same model, source revision and test parameters,
   recording compiler and backend differences.
3. Test a separate vector-enabled build only after validating compiler
   support and Haiku vector-register preservation across context switches.
   Pioneer uses the older RVV 0.7.1/T-Head vector extension, not RVV 1.0
   ([vendor specifications](https://milkv.io/docs/pioneer/overview)).
   This llama.cpp checkout includes `GGML_XTHEADVECTOR` support, but it is
   not enabled or validated in the current Haiku build.
4. Check numerical correctness before accepting optimized throughput;
   retain this scalar build as the reference.
