# Pioneer llama.cpp CPU baseline

## Haiku run started 2026-09-08

Samsung BFS boot verified over SSH: `/boot` is
`/dev/disk/scsi/0/3/0/0`, 111.8 GiB. No kernel changes for this run.

Runtime: `/boot/home/develop/llama-c060ca974c77/build-pioneer/bin/llama-bench`.
Source revision: `c060ca974c773c7c3d17fd1b66dc9d312bc292c0`, with the
three Haiku compatibility edits preserved in `../llama-c060ca-haiku.patch`.
Existing Release CPU_GENERIC build; optional RISC-V extensions and OpenMP
disabled. This is an initial baseline, not optimized RISC-V performance.

Model: `/boot/home/models/Qwen3-0.6B-Q8_0.gguf`.
Previously verified SHA-256:
`9465e63a22add5354d9bb4b99e90117043c7124007664907259bd16d043bb031`.

Arguments:

```sh
llama-bench -m /boot/home/models/Qwen3-0.6B-Q8_0.gguf \
  -t 4,8,16,32,64 -p 128 -n 32 -b 64 -ub 64 -ngl 0 \
  -r 3 -o jsonl --progress
```

Warm-up enabled. Prompt processing and generation are separate tests.
Thread counts are worker counts, not changes to the number of online CPUs.
No explicit CPU affinity or NUMA policy. Other options retain this revision's
defaults. User asked to leave the machine idle during the run.

Raw stdout and stderr are saved in the adjacent dated files. A partial
output file does not imply the full sweep completed successfully.
Linux comparison must use the same revision, model and test parameters;
record compiler/backend differences before attributing results to the OS.

### Initial run interrupted

The initial run was deliberately stopped with SIGTERM during the first
prompt warm-up; no JSONL timing samples were produced. Haiku remained
responsive over SSH. Two 2-second `top` samples showed the SCSI scheduler
using approximately 1.85-1.91 CPU seconds per interval and all four llama
workers using only a small fraction of a core each. The serial log was
actively emitting `ahci: sg_memcpy phyAddr` for 4096-byte segments.

The next comparison removes only that per-segment trace, retaining errors
and startup diagnostics. This is a suspected I/O/logging bottleneck, not
yet a measured explanation for all of the delay.
