# llama.cpp CPU performance on Haiku / Milk-V Pioneer

Recorded: 2026-09-08.

## Status

Latest: the uncapped-demand scheduler candidate passed three seeded
imbalance probes and the 100-migration regression test. Its complete
unpinned llama sweep exited 0: 32-worker generation 12.939 tokens/s,
64-worker generation 10.610 tokens/s, with no severe collapse in this run.
Linux comparison is also complete. Through 32 workers, matched scalar
performance is similar. Linux's unpinned 64-worker runs were variable;
explicit placement gave 9.444 tokens/s over 32-token generation, while
the latest unpinned Haiku sweep gave 10.610. These are limited trials, not
a general OS ranking. See the sections below for settings and caveats.

## Original baseline results

The first table below is the historical Haiku baseline, before the scheduler
changes. Current scheduler-candidate results and the Linux comparison are
recorded later in this document.

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

### Linux matched scalar comparison

Built the exact unmodified `c060ca974c773c7c3d17fd1b66dc9d312bc292c0`
source archive on the same Pioneer under Fedora Linux, kernel 6.1.31.
Native GCC 13.2.1 and CMake 3.27.4; Haiku used GCC 13.2.0 and CMake 3.31.8.
Both builds use shared libraries, Release/O3, CPU_GENERIC, and no OpenMP,
RVV, or XTheadVector. Linux explicitly uses `-march=rv64gc -mabi=lp64d`.
The comparison-only `linux-scalar-generic.cmake` sets CMake's processor
dispatch to `other` to select the same generic backend as Haiku. The actual
compiler remains native RISC-V Linux. No Linux source modifications.

Linux reports 64 online CPUs across four NUMA nodes, 131976904 KiB RAM.
No NUMA binding, worker affinity, governor change, or profiler is applied.
No cpufreq policy0 data was exposed by the inspected sysfs paths. This is
an application/OS comparison, not an isolation of kernel scheduling alone:
libc, compiler patch release, memory policy, and other OS behavior differ.

Source and model are in a separate staging directory on the R3SL SSD:
`/mnt/ssd/haiku-deploy/llama-linux-c060ca-N3stW2`. The Samsung disk and SD
are not written. Model filename is `Qwen3-0.6B-Q8_0.gguf.part`, but it is
the complete verified 639446688-byte model with SHA-256 `9465e63a...`.
Source archive SHA-256:
`ef1286b5bc643e2394957f655f05be468bbf505306cb5a9e2508ac2746957528`.

Reproduction: `build_linux_scalar.sh STAGING_DIRECTORY` verifies both
inputs and builds the benchmark and completion tool. Build exited 0.
Exact compile command, build log, and environment are saved in
`linux-cpu-compile-command-20260909.txt`, `linux-scalar-build-20260909.log`,
and `linux-environment-20260909.txt`.

```sh
comparison=/mnt/ssd/haiku-deploy/llama-linux-c060ca-N3stW2
"$comparison/build/bin/llama-bench" \
  -m "$comparison/Qwen3-0.6B-Q8_0.gguf.part" \
  -t 4,8,16,32,64 -p 128 -n 32 -b 64 -ub 64 -ngl 0 \
  -r 3 -o jsonl --progress
```

Raw results: `linux-scalar-full-20260909.{jsonl,stderr}`.

The Linux sweep completed all ten tests, exit 0. Comparison against the
uncapped-demand Haiku candidate (mean tokens/second, three repetitions):

| Workers | Haiku prompt | Linux prompt | Haiku generation | Linux generation |
| ---: | ---: | ---: | ---: | ---: |
| 4 | 4.603126 | 4.537783 | 3.148009 | 3.185387 |
| 8 | 8.827817 | 8.558674 | 6.155621 | 6.203492 |
| 16 | 15.538612 | 15.589630 | 11.366162 | 11.619844 |
| 32 | 23.997733 | 24.739234 | 12.939131 | 12.454231 |
| 64 | 35.590468 | 8.912730 | 10.610063 | 3.275249 |

Linux's 64-worker generation was highly variable: 0.587546, 6.80228,
and 2.43592 tokens/s (standard deviation 3.191252). Its average is not a
reliable estimate of best achievable Linux performance. Through 32 workers,
both systems are in broadly the same range. A lightweight `ps -L` snapshot
during the final test showed two workers last assigned to CPU 60; PSR is
last-CPU metadata, not simultaneous execution proof. Snapshot saved in
`linux-scalar64-thread-snapshot-20260909.txt`. This diagnostic may slightly
perturb that test and is not a root-cause diagnosis. Fresh-process and
explicit-affinity controls are required before a high-thread-count verdict.

Model storage differs (Linux staging R3SL SSD versus Samsung on Haiku);
these are measured inference repetitions after warm-up, not model-loading
or storage-speed benchmarks.

#### Linux 64-worker controls

All controls completed, exit 0; same native scalar binary and model:

| Test | Generation length | Mean tokens/s | Sample standard deviation |
| --- | ---: | ---: | ---: |
| Full-sweep unpinned | 32 | 3.275249 | 3.191252 |
| Fresh-process unpinned | 32 | 2.053593 | 2.173011 |
| Strict per-worker affinity | 32 | 9.443906 | 0.248929 |
| Strict per-worker affinity, longer run | 128 | 8.889666 | 0.711164 |

Fresh unpinned samples: 1.13239, 4.53546, 0.49293 tokens/s. Short pinned
samples: 9.20872, 9.41839, 9.70461. Longer pinned samples: 8.10674,
9.49566, 9.06660. Pinned tests add only `--cpu-mask ffffffffffffffff
--cpu-strict 1`; the longer control also uses `-n 128`. All use r3.
The longer test's /proc status snapshot verifies 64 distinct single-CPU
masks covering CPUs 0..63. The short test ended before its snapshot; its
empty mask file is not verification evidence. No affinity errors reported.

Raw controls: `linux-scalar64-repeat-20260909.{jsonl,stderr}`,
`linux-scalar64-pinned-20260909.{jsonl,stderr}`,
`linux-scalar64-pinned-long-20260909.{jsonl,stderr}`, and
`linux-scalar64-pinned-long-masks-20260909.txt`.

Interpretation: placement sensitivity also affects this Linux scalar
workload. Do not compare only its slow unpinned outliers with Haiku and
claim a universal Haiku speedup. The current Haiku scheduler is competitive
in these limited matched-backend tests. Neither OS demonstrates better
generation throughput at 64 workers than at 32 under the tested settings.
Further work should distinguish NUMA placement, synchronization overhead,
and vector-kernel performance; this data alone does not isolate them.
The optimized/native RISC-V backend and vector extensions were deliberately
not benchmarked. Both benchmark binaries are available in Linux's staging
directory, but were not installed system-wide. No benchmark remains active.

### Scheduler reproduction and profiler failure

Ran the original unpinned 4/8/16/32/64 sequence using the matched baseline
backend. Nine rows completed; 64-worker generation did not finish.
`haiku-scheduler-repro-20260908.{jsonl,stderr}` is incomplete and must not
be treated as a completed sweep. A two-second scheduler-profiler smoke
capture ran during the early part of this sweep, so it is diagnostic data,
not an entirely uninstrumented throughput baseline.

Added a bounded 16 MiB in-memory capture utility using the existing system
profiler. Its initial two-second smoke test exited 0 with 4915 switch
events. A second capture during 64-worker warm-up panicked in
`SystemProfiler::_AllocateBuffer` on CPU 12, thread 2027. Store page fault
at `0xffffffc0208018b8`, PC `0xffffffc0021588ce`, interrupts disabled;
printed leaf PTE `0x7000000127ab68e7`. The entry appears valid/writable,
but that alone does not establish the mapping/translation failure's cause.
Full serial evidence: `haiku-scheduler-profiler-panic-20260908.txt`.
The second capture produced no usable switch data. Worker mask snapshot
exists, but does not reveal actual CPU placement. Do not rerun the profiler
on this candidate merely to obtain performance data.

Static inspection identified a separate hypothesis: low-latency rebalance
compares capped core load with a worker's demand. A 100%-demand worker
cannot pass the migration threshold using a capped 100% source load and
20% margin, even when that core hosts two such workers. Runtime evidence
is still needed. No scheduler policy change has been made for this theory.

Prepared `scheduler_balance_probe.c`: seed two workers on CPU 0, leave
CPU 63 without a probe worker, warm up for one second, then release all
affinity masks and sample each worker's CPU for four seconds. This avoids
the kernel profiler and is a diagnostic workload, not a llama benchmark.

After recovery to the same SD image, this probe completed three times
(exit 0 each). In every run workers 0 and 63 sampled only CPU 0 throughout
the four-second unrestricted phase; the other 62 workers stayed on CPUs
1..62. No worker sampled CPU 63. All affinity calls returned B_OK.
The two sharing workers recorded roughly half as many samples each as a
worker on its own CPU; counts are observations, not calibrated CPU time.
Probe binary SHA-256 was verified on Haiku:
`b3ab11c4f35556663603bccff22c2f456deb694492c8df004a35382ed949d86c`.
Evidence: `haiku-scheduler-balance-probe-20260908.txt` and
`haiku-scheduler-balance-repeat-20260908.txt`.

This directly reproduces persistent imbalance after restrictions are
removed, independently of llama's barriers. It supports, but does not yet
prove, the capped-demand explanation. Prepared a kernel candidate adding
`CoreEntry::GetUncappedLoad()` and using it only for low-latency rebalance
comparisons. Existing capped utilization, heap keys, affinity checks,
power-saving policy, and 20% migration margin remain unchanged. Example:
two fully busy workers represent demand 2000, not capped 1000; against an
idle core, the existing threshold can then permit moving a 1000-demand
worker. Runtime validation at that point was pending.

#### Uncapped-demand candidate: deterministic test passed

Deployed only to the SD. Full payload readback matched
`01dc43b1e36671a53bf23be46587f503ed61af4c55b73ca50bbe51769bd9ed41`.
Rollback on Linux:
`/mnt/ssd/haiku-deploy/haiku-pioneer-bfs-before-20260909T040948Z.img.gz`.
The active `/boot/system/kernel_riscv64` hash matches the packaged candidate:
`3fe93a8af0fddadfa226ced3abf504e7b1e1ad4d6baa523849e005819101e110`.
The unstripped build artifact has a different hash (`ab47572f...`);
compare deployed kernels against the packaged artifact, not that file.

The affinity smoke test passed all 100 migrations and sleep/wakeup checks.
All three seeded-imbalance probes completed successfully. In runs 1 and 2,
worker 63 moved from CPU 0 to CPU 63; in run 3, worker 0 moved instead.
Only about 1460-1610 early samples remained on the shared CPU before the
moving worker accumulated about 1.43 million samples on CPU 63. Before
this change, both workers stayed on CPU 0 for the entire four-second
unrestricted phase in all three runs. This is a confirmed improvement in
the deterministic scheduler test, not yet a llama throughput claim.
Evidence: `haiku-scheduler-demand-probes-20260908.txt`.

Started the original native scalar llama runtime, with no affinity options
and no profiler, using `-t 4,8,16,32,64 -p 128 -n 32 -b 64 -ub 64 -ngl 0
-r 3 -o jsonl --progress`. Samsung is mounted read-only at `/Haiku1`.
Results: `haiku-scheduler-demand-full-20260908.{jsonl,stderr}`.

The sweep subsequently completed all ten tests, exit 0. Three measured
repetitions per row; mean +/- sample standard deviation, tokens/second:

| Workers | Prompt processing (128 tokens) | Generation (32 tokens) |
| ---: | ---: | ---: |
| 4 | 4.603126 +/- 0.007067 | 3.148009 +/- 0.007362 |
| 8 | 8.827817 +/- 0.004662 | 6.155621 +/- 0.011144 |
| 16 | 15.538612 +/- 0.015370 | 11.366162 +/- 0.034655 |
| 32 | 23.997733 +/- 0.003516 | 12.939131 +/- 0.058303 |
| 64 | 35.590468 +/- 0.557512 | 10.610063 +/- 0.448594 |

The deterministic imbalance is corrected and the severe collapse did not
occur in this sweep. This does not prove all intermittent scheduling
problems are eliminated. Earlier healthy unpinned runs were also fast;
do not present comparison against a prior slow outlier as a universal
speedup. Generation still favors 32 workers over 64. Prompt processing
benefits from 64. The profiler mapping fault remains unresolved and was
not exercised in this run. No benchmark remains running. Next: equivalent
Linux baseline before further optimization or distribution packaging.

### llama startup affinity experiment

Added a Haiku branch to `ggml_thread_apply_affinity`, using libroot's
private `_kern_set_thread_affinity` syscall. It validates CPU indices,
uses Haiku's uint32 bitmap layout, and reports failures. Priority handling
is unchanged. This is an opt-in llama change, not another kernel change.
The existing threadpool assigns distinct CPUs with `--cpu-strict 1`.

Kernel remains the SD trap-migration candidate (`5902003a...`). Samsung
remains read-only; the original runtime, model, and installed OS are
unchanged. Test libraries are in `/tmp/baseline` and `/tmp/affinity`.

To isolate compiler effects, both test libraries replace only
`ggml-cpu.c.o`, compiled using cross GCC 13.3.0 with the original Release,
CPU_GENERIC, scalar options. All other CPU backend objects and the base
library are copied from the native GCC 13.2.0 build. The original llama
executable and other libraries are reused. These are hybrid test builds,
not a freshly rebuilt distribution package.

Rebuild script: `build_llama_affinity_backend.sh OBJECT_DIRECTORY`.
The directory must contain `native-objects/` copied from
`build-pioneer/ggml/src/CMakeFiles/ggml-cpu.dir/ggml-cpu/`, and
`libggml-base.so.0.21.0` copied from `build-pioneer/bin/`.
The script generates separate baseline and affinity libraries. A standalone
source patch is saved as `../llama-c060ca-haiku-affinity.patch`.

Final backend SHA-256 values:

- Baseline: `f4be753ee1f9a4def96e2b48700c94d5b0dd13aaeee3c3d204bf2aeace794263`
- Affinity: `e91b5cb2c94d683f4413021a4d7dc4c0cfeefc471503ab26d4441392928ba762`

Initial pinned probes completed successfully: 32-token generation averaged
13.021658 +/- 0.401167 tokens/s; 128-token generation averaged
11.238023 +/- 1.488758 tokens/s, each with three repetitions. The longer
run's mask snapshot confirms all 64 workers have distinct single-bit
masks covering CPUs 0 through 63. These two probes used an earlier uint64
bitmap representation (identical bytes on this little-endian machine),
backend SHA-256 `fecc2957cb5938d0ddb3e0aa5baac01ef503639909ba38357b4b1d6aa2bbab87`.

Raw evidence: `haiku-llama-affinity64-20260908.{jsonl,stderr}`,
`haiku-llama-affinity64-long-20260908.{jsonl,stderr}`, and
`haiku-llama-affinity64-long-masks-20260908.txt`. The short run's snapshot
caught only the main thread after workers exited; use the longer snapshot
as the placement evidence.

Alternating baseline/affinity fresh-process trials are in
`haiku-llama-affinity-ab-20260908.{jsonl,stderr}`. Rows alternate baseline,
affinity for three rounds; stderr labels each trial. Both receive identical
CPU-mask options, but the baseline ignores them (the prior Haiku stub).
JSON records requested options, not verified placement.

All six alternating trials exited successfully. Generation throughput:

| Fresh-process round | Matched baseline (unpinned) | Startup affinity |
| ---: | ---: | ---: |
| 1 | 10.922765 | 12.629153 |
| 2 | 12.153996 | 12.330276 |
| 3 | 12.217611 | 12.202060 |

No affinity errors were reported. Neither variant reproduced the severe
collapse in this batch. This verifies the opt-in mechanism but does not
establish that it reliably prevents the intermittent scheduler problem.
No general scheduler fix or improved 32-to-64 scaling is claimed.

The final-library sequence `-t 32,64 -p 128 -n 32 -r 3` also completed
all four tests, exit 0, without affinity warnings:

| Workers | Prompt tokens/s | Generation tokens/s |
| ---: | ---: | ---: |
| 32 | 23.215656 +/- 0.270143 | 16.266823 +/- 0.094419 |
| 64 | 34.774933 +/- 0.026403 | 13.412439 +/- 0.252681 |

Evidence: `haiku-llama-affinity32-64-20260908.{jsonl,stderr}`. Its late
mask snapshot caught only the main thread; the earlier long-run snapshot
remains the complete 64-worker placement evidence. This sequence does not
include the earlier 4/8/16-worker phases. Generation still favors 32
workers, while prompt processing benefits from 64. Startup affinity is a
working diagnostic/opt-in control, not proof of a general scheduler fix.
No benchmark remains running after these trials. Further repetitions and
correctness checks are needed before distribution integration.

```sh
runtime=/Haiku1/home/develop/llama-c060ca974c77/build-pioneer/bin
LIBRARY_PATH=/tmp/affinity:$runtime:/boot/system/lib \
  "$runtime/llama-bench" -m /Haiku1/home/models/Qwen3-0.6B-Q8_0.gguf \
  -t 64 -p 0 -n 32 -b 64 -ub 64 -ngl 0 -r 1 -o jsonl --progress \
  --cpu-mask ffffffffffffffff --cpu-strict 1
```

### Isolated 64-thread runs: placement variability

On the same migration-candidate SD boot, isolated generation with
`-t 64 -p 0 -n 32 --poll 50` completed at 0.296190 tokens/s (one
repetition, 108.038867 seconds). During the slow phase, CPU 8 was nearly
idle; two workers each consumed about one CPU-second per two-second
sample while the remaining workers consumed nearly two. Total CPU use
was approximately 98%. This is consistent with two workers sharing one
CPU while another CPU is idle, causing barrier delays; direct per-thread
CPU placement was not captured in that sample.

A second isolated run, now with three repetitions, completed at
12.785758 +/- 0.412390 tokens/s. An external pinning attempt during its
warm-up returned B_BUSY on the first thread, so no affinity was changed.
The retry produced no thread records because the benchmark had finished.
This is NOT a pinned result. The large run-to-run difference means token
count alone does not explain the slowdown. Controlled startup affinity is
the next diagnostic, before any claim of a general scheduling fix.

Evidence:

- `haiku-migration-isolated64-20260908.jsonl`
- `haiku-migration-isolated64-activity-20260908.txt`
- `haiku-migration-isolated64-repeat-20260908.jsonl`

Both benchmark processes exited 0 and have finished.

### Trap-migration candidate: preliminary improvement

The SD candidate restores kernel trap state on context switch and removes
blanket pinning for the user trap lifetime, retaining other kernel pinning.
The affinity smoke test passed 100 migrations between CPUs 0 and 63, with
placement checks after sleeping/waking and an initial suspended-thread
affinity change.

A short `-t 32,64 -p 0 -n 8 -r 1 --poll 50` generation probe exited 0:

| Threads | Generation tokens/s (one repetition, 8 tokens) |
| ---: | ---: |
| 32 | 14.613447 |
| 64 | 13.773844 |

The severe 64-thread collapse is absent in this probe. This is not yet
a like-for-like speedup measurement: the original used 32 generated tokens
and three repetitions. Also, the candidate boots from SD, with Samsung
mounted read-only at /Haiku1; the runtime and model are unchanged on Samsung.
LIBRARY_PATH includes the relocated build bin directory and /boot/system/lib.
The initial launch without this corrected search path exited 3; it was
not a computation failure. A CPU sample ran too late to establish active
worker distribution during this short probe.

Raw results: [migration probe](haiku-trap-migration-probe-pathfixed-20260908.jsonl).
The full original 4/8/16/32/64-thread settings completed all ten tests
with exit 0. Raw output: `haiku-trap-migration-full-20260908.{jsonl,stderr}`.

| Threads | Prompt tokens/s (mean +/- SD) | Generation tokens/s (mean +/- SD) |
| ---: | ---: | ---: |
| 4 | 4.553071 +/- 0.043294 | 3.178920 +/- 0.001457 |
| 8 | 8.758219 +/- 0.000701 | 6.099343 +/- 0.002957 |
| 16 | 15.593373 +/- 0.005259 | 11.144254 +/- 0.016266 |
| 32 | 28.181762 +/- 0.030087 | 15.824186 +/- 0.040242 |
| 64 | 38.931319 +/- 0.040316 | 0.260900 +/- 0.000052 |

The longer run does NOT confirm that 64-thread generation is fixed.
Prompt processing at 64 threads improved from 7.09 to 38.93 tokens/s,
and 32-thread generation improved from 11.94 to 15.82 tokens/s. However,
64-thread generation remains severely slow (0.261 tokens/s), despite the
successful 8-token probe. Investigate token-count and run-history effects,
and profile the slow phase before drawing a root-cause conclusion.
The different boot volume remains a comparison caveat. The best generation
throughput measured in this full sweep is at 32 threads.

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
