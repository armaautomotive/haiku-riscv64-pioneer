# Native build status

Latest checkpoint (October18,00:20 UTC / October17 Vancouver): Haiku is
running, SSH10.0.0.247 responds, P100A installed, both SD cards out. SD-free
handoff verified64 CPUs and137,034,936,320 total usable bytes (127.62GiB).
Tested serialized NIC SHA0b83ceed is retained. Experimental storage45
SHAf7d80213 is now installed/LOADED after the twice-UNLOADED installation
guard. Qualified43 SHA9e098d02 is preserved outside driver paths. Native
seven-bank1..4-page DMA sample passed ALL56 uploads/112 independent PIO
pages including untouched neighbours,784 KNOWN NoC requests,clean END;
no UNKNOWN.573,440B upload calls took0.365018s (not a full-model benchmark).
No live model/GPU job remains. Restore43 only after a NEW cold-recovered
Haiku boot with its card driver still UNLOADED,using the exact trial helper
`pioneer-qwen38-bulk-20261017-a/switch-unloaded45.sh --restore43`.
NEVER replace a loaded driver. Assistant/firmware/default boot/dirty submodule
are unchanged. Requested-fast27B P100A inference remains UNACHIEVED.

The default-off HAIKU_PIONEER_RTL_SERIALIZE variant addresses a source-level
locking mismatch: send/ioctl/tick use Giant, MPSAFE interrupt bypasses it.
It serializes that interrupt, directly rings the existing TX doorbell after
descriptor sync, and avoids recursive Giant on system-error reset. Actual
production selection/compatibility/helper tests pass ASan/UBSan, preserving
default/foreign profiles. Candidate0b83ceed reproduces across two builds.
Exact Linux installer passed read-only-source private COW rehearsal, then
one-file application/readback. Qualified prior NICfe6273c9 is preserved
outside driver directories. Native nine FRESH unthrottled64MiB transfers
passed ALL hashes:603,979,776 bytes, ZERO retries/interface resets/GPU work,
zero reported NIC errors/drops. One2s ping timed out under load; SSH/data
continued, and post-load/final three-ping checks passed. This is bounded
native qualification, NOT proof of the old stall's cause or long-term cure.
The public source listener closed. Diagnostic900-tick limit is bounded.

IMPORTANT: use the separate Linux handoff helper
`/mnt/ssd/pioneer-haiku-kexec-20261013/pioneer_linux_boot_haiku_network-serialized-1.sh`
(SHA0bfe2647) with `prepare|boot SESSION 64 memory128-spin-idle6-nosd-control`.
Its companion verifier SHA32b924b7 pins NIC0b83ceed. The preserved ORIGINAL
spin-idle-runtime-6 helper expectsfe6273c9 and now correctly FAILS CLOSED.
Kernel/bundle/64-CPU/full-RAM profile are unchanged; see exact pins below.

Requested Qwen3.8-27B Q4_K_M18,973,870,528B passes WHOLE native SHA
`c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747`.
Three authenticated leaves repaired ONLY the unpublished assembly after
entire-old-leaf checks; whole SHA passed BEFORE final-name publication.
851 layouts/8,162,688 finite boundary values passed native inspection.
Original bad parts/repair inputs remain preserved. CPU-only one-token smoke
passed with32 threads/zero offload:196.82s for ONE unwarmed token. This is
NOT text quality, steady speed, or a P100A performance result.

Experimental storage-only44 ca6c7fdf/clientd5b2a742 passed the native sample:
exclusive seven-bank owner, repeated/foreign BEGIN and legacy refusals,
write ALL14 distinct first/last4KiB pages then independently read all14,
56 known NoC requests, clean END/PCI restore/host release, NO UNKNOWN.
This is NOT full21GiB capacity qualification, compute, or model inference.
After synced shutdown/cold recovery, exact restore helper SHA5e0ec5a6 passed
private COW rehearsal/application; final native readback is qualified43
SHA9e098d02. Temporary44 backup is preserved outside driver directories.
Both offline checkfs runs reported0 missing/duplicate blocks and unchanged
17,665 freeable blocks; no filesystem repair was performed.

OFFLINE exact GGUF reader/lossless tiling/seven-bank packed blueprint,
hybrid math, packed dots, activation quantizers and four local RV32 images
pass host tests but are NOT selected by driver/backend/Assistant. Fast
mixed Tensix kernels, bulk model upload, activation integration, full graph
and whole-model quality/steady-speed qualification remain. Qualified43's
last fixed Qwen3-0.6B Q8_0 result remains7.428707..7.557277tok/s, not fast27B.
See the current
[P100A status](TENSTORRENT_PORT.md#status) and the
[Assistant integration record](../../src/apps/assistant/README.md#p100a-integration-verification-on-pioneer-2026-10-15).
See [the current P100A bring-up record](TENSTORRENT_PORT.md) for exact artifacts,
rollback evidence and the remaining startup/cache gate. Older entries below
are chronological checkpoints, not the current hardware state.

Samsung boot and automatic key-authenticated SSH were verified. The launcher
needs `sshd -f /dev/null` because the offline-installed OpenSSH package's
default `.settings/ssh/sshd_config` path was missing.

Installed locally through pkgman, without replacing the kernel:
GCC and gcc_syslibs_devel 13.2.0_2023_08_10-2, binutils 2.41-1,
Make 4.3-1, GMP 6.2.1-3, MPFR 4.2.0-3, MPC 1.2.1-2, and the
locally built haiku_devel package. A C++17 std::thread smoke test passed.
Native repository downloads returned "Operation not supported"; packages
were fetched from the official build-packages archive on the Mac and
their transfer hashes verified before installation.

Samsung staging: `/boot/home/develop/pioneer-native-tools`.
Mac staging: `/private/tmp/pioneer-native-tools.p2RE9f`.
CMake 3.31.8 source was extracted and bootstrap started with four jobs,
prefix `/boot/home/config/non-packaged`, OpenSSL and testing disabled.
C and C++ compiler detection passed; bootstrap completion is NOT verified.
The system clock was corrected from 1970 to current UTC during preparation.

llama.cpp clean local revision:
`c060ca974c773c7c3d17fd1b66dc9d312bc292c0`.
Its archive transfer was interrupted; do not trust the partial remote archive.
No llama.cpp compile or inference test has completed.

During concurrent CMake preparation and a rate-limited SCP source transfer,
the kernel panicked in thread 204 `/dev/net/rtl8125/1 consumer`, CPU 59.
Fault PC `ffffffc0021b040e` (`list_remove_item + 0x0e`), store fault VA
`ffffffc024d73038`. P330 reported physical page `19b9f000`, flags `7030`,
area 21620, slab base `ffffffc024800000`, size `800000`, protection `30`.
This repeats the earlier network-consumer fault signature; workload correlation
does not establish its underlying cause. The kernel-stack dirty-bit change
does not resolve this separate observed failure.
Trace preserved at
`/private/tmp/pioneer-native-tools.p2RE9f/serial-network-panic-20260907.log`.
Board left in KDL for inspection, not restarted.

## Follow-up candidate: shared kernel RAM translation visibility

After user-authorized shutdown, Linux was booted with SD removed, then the
32 GB SD (serial `0x0000e752`) was reinserted. Source audit found that Map()
queued synchronous remote invalidation only for kernel stacks, not newly
mapped slab chunks. Slab uses CACHE_TYPE_NULL, whose generic Fault() rejects
faults with B_BAD_ADDRESS even when a subsequent PTE query sees a valid page.
This supports a stale-translation hypothesis but does not prove it.

Candidate extends the existing invalidation queue to T-Head shared kernel
RAM with default/write-back memory type after bootstrap. Device mappings,
CPU count, and dirty-bit policy are unchanged. Kernel build and image build
passed; packaged and unstripped kernel .text contents compare equal.
Kernel SHA-256: `232d0778e59c985d0ca879dd3fd2f2d6202ab6421953c8ce119dbdf0f3a4f0fe`.
Package SHA-256: `9228c446db11392824a99a9dedd08b6ceee92b35a226cd2f215ed76445304966`.
SD payload: `haiku-pioneer-bfs-shared-ram-tlb.img`.
Payload SHA-256: `29e1600f2b1f732b4e29bc90b82411d5a8f5f2d13cb4928c6696e593bac9517e`.
SD rollback stamp: `20260907T210157Z`.
Samsung and firmware remain unchanged. Runtime validation is pending.

P338 SD boot passed desktop/input/SSH checks. First 36,801,423-byte source
transfer completed and matched SHA-256
`ef1286b5bc643e2394957f655f05be468bbf505306cb5a9e2508ac2746957528`.
Two earlier SCP connection attempts closed before payload transfer while SSH
remained responsive. The second full transfer attempt then reproduced the
kernel panic without a concurrent CMake build. Thread 199 RTL8125 consumer,
CPU 2, PC `ffffffc0021b0414` (`list_remove_item + 0x14`), fault address
`ffffffc01af190e0`, P330 physical page `1d66e000`, flags `7030`, area 11343,
slab base `ffffffc01a800000`, size `800000`, protection `30`, CACHE_TYPE_NULL.
The broader Map() flush is not sufficient to fix this failure. Do not promote
it to Samsung as a proven fix. The second test file may be incomplete.
Trace: `/private/tmp/pioneer-native-tools.p2RE9f/serial-p338-repeat-panic-20260907.log`.

## P339 trap-time snapshot

Restored Map() to checkpoint policy after P338 failed the repeat transfer.
Added a diagnostic before enabling interrupts for kernel store faults when
the kernel mapping query already reports present, writable, accessed, dirty.
It prints the CPU, kernel query, trap registers, and live SATP page-table walk.
No retry or fault suppression was added. This distinguishes active-root state
from the later P330 query, which occurs after VM fault handling can schedule.

Build passed; SD deployment full readback verified. Runtime test pending.
Kernel SHA-256: `935d7b94fc06b85e2dd883594167702eee91454e0e478174d0be23c06957fd08`.
Package SHA-256: `3ebe0bb2d29b64dac9ef354e8c4f16cd4e39c6d90e10450bf24f34aeb638be84`.
Payload `haiku-pioneer-bfs-fault-snapshot.img` SHA-256:
`fb6f8ed8562736bfaa0a9484e39073cec0ac1240f86c81a6a3a78282991abdde`.
Rollback stamp: `20260907T211848Z`. Samsung and firmware unchanged.
Linux remains running pending a clean shutdown for the SD test.

P339 boot test and one user-authorized cold retry both stopped at
`P214:SR0 reset all begin bits 0x0`, before boot-volume mount or graphics.
No P339 snapshot or panic was printed. First serial offset 23594187;
retry offset 23736030, last log size 23877871. User reported black screen.
This is a repeatable boot regression relative to P338 in these tests, not
evidence of the network fault being fixed. Do not repeat power cycles without
a new diagnostic or controlled build comparison. Board remains at the stall.

Control prepared after user-authorized Linux boot: restored the exact archived
P337 payload `haiku-pioneer-bfs-stack-dirty.img` to SD, SHA-256
`1bb85fa481389e436bc83eb8dc5126ee8d98290ac941016b81fd58b9bd231014`.
Full readback verified. Diagnostic-image rollback stamp `20260907T213605Z`.
No rebuild was used for this control. Samsung and firmware unchanged.
Linux remains running; control boot is pending. P339 source changes remain
in the worktree for investigation, but are not in the restored SD payload.

Control boot at serial offset 23943223 passed the prior SD-reset stall,
mounted SD at 94.44s, enabled secondary CPUs at 109.31s, and accepted SSH.
No panic observed at this checkpoint. This contrasts with two P339 stalls;
it does not establish whether diagnostic execution or another binary/layout
difference caused them. Do not use P339 for further network testing until
that regression is isolated. Current running system is SD P337, not Samsung.

## P340 prepared, not deployed

Replaced P339 with a noinline raw trap diagnostic: before enabling interrupts
for an unhandled kernel-address store fault, print a marker, CPU, saved trap
registers, and live SATP page-table walk. No VM reference acquisition or
translation-map query in this new helper; existing fault repair remains intact.
This is a diagnostic simplification, not proof of the P339 stall's cause.
Build passed, packaged/unstripped .text compared equal.
Kernel SHA-256: `0b4d2e8d43ea751b1ece25cafdfc6a395a75cb7f5ba2615361bc90a0555ea9aa`.
Package SHA-256: `70725d785ddf882809e97abb242d4e79ef8449a12164c84a7ea5a54c3d8ca63b`.
Payload `haiku-pioneer-bfs-raw-fault-snapshot.img` SHA-256:
`ebe751fe5ee83c21446537bc20fde82b9b6c787786abc5157d3c1dc39c136599`.
Requires Linux SD deployment. Samsung is unchanged.

P340 deployed with full readback verification, rollback `20260907T215429Z`.
Boot serial offset 24270285 passed the P339 SD-reset stall but panicked
during user-space startup before SSH or any transfer test. Thread 157
media_server, CPU 11, object_cache_alloc+0x4c at PC `ffffffc0021adc2e`,
store VA `ffffffc008052a80`. Snapshot before enabling interrupts:
SATP `800000000000dae8`, L2 `700000002f635801`,
L1 `70000000013ab821`, L0 `70000000036b98e7`.
Leaf decodes as valid/R/W/global/A/D with physical page `dae6000`, matching
the subsequent P330 query (flags `7030`, slab area 7446, base
`ffffffc008000000`, size `800000`, CACHE_TYPE_NULL).
Thus the valid mapping was visible in the live faulting hart's page-table
walk, not just a later kernel query after potential migration. This does not
exclude stale translation or concurrent A/D changes, nor establish that
all earlier network faults have the same cause. Fault occurs outside RTL8125.
Trace: `/private/tmp/pioneer-native-tools.p2RE9f/serial-p340-startup-panic-20260907.log`.
No runtime fix validated. Samsung remains P337 with tools and SSH preserved.

## P341 bounded translation retry experiment

Added a T-Head-only retry for supervisor stores to live Sv39 level-0 kernel
RAM mappings already valid, readable, writable, global, accessed and dirty.
It executes local SFENCE.VMA without changing the PTE. Device/user mappings
are excluded. Identical consecutive fault keys and a 16-retry per-CPU boot
cap fall through to normal fault handling. This is diagnostic, not a final
production policy or proof of the underlying translation-coherency cause.

Build passed; SD payload full readback verified. Firmware and Samsung kernel
remain unchanged (Samsung P337). Rollback stamp `20260907T224053Z`.
Kernel SHA-256:
`db082a789069bc14e405b862a8bf38163335be27718ddfe5eb52867e12a23077`.
Package SHA-256:
`e37b0f44f2957631d246d1ccf9a85865819a498f917ef669a8bd1c5183b61ac9`.
Payload `haiku-pioneer-bfs-bounded-store-retry.img` SHA-256:
`8f622d13326baa116a9470675ff6ab79469b2e0b6f1255d67faed06b2e241966`.

Boot serial offset 24550600: SD mounted at 95.38s, secondary CPUs enabled
at 110.45s, SSH operational. Samsung mounted at `/Haiku1` for transfer tests.
First three 36,801,423-byte source transfers passed SHA-256 verification:
`ef1286b5bc643e2394957f655f05be468bbf505306cb5a9e2508ac2746957528`.
During further repeated transfers, P341 logged two retries on CPU 38:
store VA `ffffffc01bf381e8`, PC `ffffffc0021b040e`,
leaf `70000000082710e7`; and VA `ffffffc01bf3d368`,
PC `ffffffc0021adc2e`, leaf `700000000c6408e7`.
Transfers continued with matching checksums after these events. This exercises
the recovery path at the earlier list_remove_item/object_cache_alloc fault
sites and supports (but does not prove) stale local translation as a cause.
Only the first two retries per CPU are logged, so log count is not total count.
All eight transfers completed with matching checksums and no observed panic.
Passes 4-8 reused `repeat.tar.gz` to bound disk usage. Candidate staging for
a native-build test on Samsung has started in
`/Haiku1/pioneer-update-p341-20260907`; activation and reboot not yet performed.

Samsung activation subsequently completed: backup hash matches P337
`7dd3646661bceaec5992858b9cf8871e2a6b29b2f6899395c8d605535bc378e8`;
active package readback matches the P341 package hash above. Only the kernel
package was replaced; apps/settings were preserved. Samsung was synced and
successfully unmounted, then normal `shutdown` requested on SD Haiku.
Await physical safe-to-power-off confirmation before relay cycling. Next
step is serial-menu Samsung boot and resumption of the native CMake build.
No Samsung P341 boot or native build result is verified yet.

After user safe confirmation, Samsung P341 boot succeeded (serial start
25289974). Select Samsung's latest activation state, then Escape from the
volume list; main menu must show 111.79 GiB before Continue. Samsung mounted
as `/boot` on `/dev/disk/scsi/0/3/0/0`; all five boot identity checks matched.
SSH package checksum equals P341, native GCC 13.2.0 remains available.
Four-job CMake bootstrap resumed with the original flags. Completion pending.

Native bootstrap passed compiler/feature checks and is compiling CMake sources
(latest observed: cmGeneratorTarget_TransitiveProperty.cxx). SSH exec session
90837 owns the ongoing build. No P341 retry or panic logged in this SSD boot
at this checkpoint. Haiku UTC clock corrected to 2026-09-07 23:05:00 after
the boot reset it to 1970; early bootstrap objects may need a timestamp rebuild.
Verified archive pass1 was successfully extracted into the new directory
`/boot/home/develop/llama-c060ca974c77`. No llama.cpp compile yet.
Next: finish CMake bootstrap, make/install, then baseline llama.cpp configure
with optional RISC-V vector/half/prefetch/pause extensions and OpenMP off.
Do not assume bootstrap or full CMake build has finished from this checkpoint.

The original bootstrap compiled its objects but failed linking libuv's socket
references (sendmsg, getsockopt, etc.). Verified those exports in libnetwork.so.
Retry with `LDFLAGS=-lnetwork ./bootstrap` and the same flags linked successfully
and entered full CMake configuration (SSH session 56034). No kernel panic or
P341 retry observed during this SSD run at that point.
Using Bootstrap.cmk/cmake for an early llama.cpp configure failed because the
bootstrap executable lacks add_compile_options; use the full CMake executable
after building it, not a llama.cpp source workaround. Preliminary cache is at
`/boot/home/develop/llama-c060ca974c77/build-pioneer`.

Corrected bootstrap completed with exit 0: configuration 642.1s, generation
11.9s. Full native `make -j4` started in the CMake source directory. No full
build/install result yet. `-lnetwork` is the only build-setting correction;
no CMake or llama.cpp source patch and no new kernel/boot change was made.

Full make later stopped at 13%: KWSys QueryHaikuInfo unconditionally referenced
x86 cpuid_info fields. Applied the narrow source patch preserved as
`tools/pioneer/cmake-3.31.8-haiku-riscv.patch`: initialize logical count from
Haiku system_info and guard the CPUID identity block for i386/x86_64.
Generic memory/frequency queries and existing x86 behavior remain intact.
Reverse dry-run against patched CMake 3.31.8 source passed. Native cmsys
compiled/linked successfully after deployment; cmlibuv also built. Resumed
full `make -j4` session 99166 reached 24%, still running, no complete CMake
or llama.cpp build yet. No new kernel/boot change in this iteration.

Full CMake link later failed on missing libuv platform symbols. Added a Haiku
source-selection block using existing no-fsevents, no-proctitle, posix-hrtime,
and posix-poll sources, linking network. This is a POSIX polling fallback,
not native filesystem notification support. Saved in the same CMake patch;
corrected earlier patch context and verified reverse dry-run for both files.
Native make completed all cmake/cpack/ctest targets with exit 0; make install
also passed. Installed cmake and ctest report version 3.31.8 under
`/boot/home/config/non-packaged/bin`.

Full installed CMake successfully configured llama.cpp build-pioneer.
Haiku reports CMAKE_SYSTEM_PROCESSOR=other, so GGML chose CPU_GENERIC.
Optional vector/half/prefetch/pause extensions, OpenMP, OpenSSL, server, app,
UI, examples and tests disabled for this initial build. Tools remain enabled.
Commit explicitly set to c060ca974c77 because Git is not installed on target.
Initial llama-cli target request failed: this revision gates CLI on server.
Started four-job build of llama-completion and llama-bench instead. Runtime/inference not
yet verified. Processor detection needs follow-up before performance tuning.

First llama build compiled/linked ggml-base, ggml-cpu, ggml and libllama.so,
then failed in common/arg.cpp on missing sys/syslimits.h. Added a Haiku-only
limits.h include branch in the local llama submodule and deployed that file.
Haiku headers define PATH_MAX and NAME_MAX there. Diff whitespace check passed.
Resumed native session 8335 passes the prior include error, but arg.cpp object
completion is still pending. Another file, common/common.cpp, fails its cache
and configuration-directory platform branches with '#error Unknown architecture'
(lines 1065 and 1111). This is OS-directory selection, not CPU instruction
support. Full completion/benchmark binaries and inference remain unverified.

Added Haiku native directory lookup in common.cpp using find_directory for
B_USER_CACHE_DIRECTORY and B_USER_SETTINGS_DIRECTORY, retaining LLAMA_CACHE
override. Applied the matching limits.h fix in download.cpp. All three source
edits live in the Mac's llama submodule and were copied to the Samsung source
tree; they are not committed. Native compilation passed for the fixed files.
llama-completion linked successfully and --version exited 0:
0.2.0-dev, build 0, commit c060ca974c77, GNU 13.2.0, Haiku other.
Session 29322 is still building llama-bench.cpp at this checkpoint.
No model inference or benchmark execution verified yet.

## First verified inference: Qwen3-0.6B Q8_0

Both llama-completion and llama-bench builds completed with exit 0.
Downloaded the official Qwen/Qwen3-0.6B-GGUF model on the Mac and copied it
via rate-limited SCP to Samsung. Mac and Samsung SHA-256 matched the publisher:
`9465e63a22add5354d9bb4b99e90117043c7124007664907259bd16d043bb031`.
Model: `/boot/home/models/Qwen3-0.6B-Q8_0.gguf`.
Source: https://huggingface.co/Qwen/Qwen3-0.6B-GGUF/blob/main/Qwen3-0.6B-Q8_0.gguf

Native llama-completion exited 0 using four generation/batch threads,
512 context, batch/ubatch 64, zero GPU layers, 32 predicted tokens,
temperature 0, seed 1, no-conversation, simple-io and perf enabled.
Prompt: "The capital of France is". Output began:
"Paris, and the capital of Italy is Rome. The capital of Spain is Madrid."
Reported prompt evaluation: 1193.43 ms / 5 tokens, 4.19 tokens/s.
Reported generation: 10119.35 ms / 31 runs, 3.06 tokens/s.
Reported load time: 10525.34 ms. These are single-run smoke-test timings,
not repeated benchmark results. CPU_GENERIC backend; system reports 64 CPUs.
No P341 retry or kernel panic logged in this SSD boot through this test.
Model loader warned about token 128247 '</s>' type and overrode it; run
continued successfully. Tools remain in the development build directory,
not installed system-wide; models are separate under /boot/home/models.

Interactive launcher installed as `/boot/home/config/non-packaged/bin/llama-chat`;
Mac source `tools/pioneer/llama-chat.sh`. Uses completion runtime in development
tree, Qwen model above, four threads, 2048 context, conversation/Jinja/simple-io.
Interactive test reached the prompt, answered "Hello!", returned to the prompt,
and exited 0 on Ctrl+D. User can run `llama-chat` in Haiku Terminal.
User requested overnight shutdown. Normal sync/shutdown requested; await their
safe-to-power-off confirmation before the relay off action. Leave SD inserted.
Next session: boot Samsung via serial volume/latest-state selection, verify
SSH and persistent model/launcher, then continue packaging/performance work.

The exact current llama.cpp delta is preserved as
`tools/pioneer/llama-c060ca-haiku.patch`. Both Pioneer source patches use
zero-context insertion hunks; apply them with `git apply --unidiff-zero`.
The llama patch was applied to a clean c060ca974 tree and reproduced all
three locally modified files byte-for-byte.

## 2026-09-08: quiet AHCI transfer path

Samsung boot verified via SSH at `/boot`, device `/dev/disk/scsi/0/3/0/0`.
The initial llama-bench 4/8/16/32/64-thread sweep was stopped during its
first prompt warm-up without producing a timing sample. Live serial output
was continuously printing `ahci: sg_memcpy phyAddr` and brief top samples
showed the SCSI scheduler consuming nearly one core while llama workers
received little CPU time. See `benchmarks/README.md` for test parameters.

Removed only the per-segment TRACE call in AHCI util.cpp's sg_memcpy loop.
Errors and startup diagnostics remain; no DMA, PCI, SMP or VM logic changed.
Build script --image completed with exit 0. Generated image still reports
the existing missing boot.scr warning; no generated firmware partition was
deployed. AHCI binary no longer contains the removed trace string.
Kernel checksum remains
`db082a789069bc14e405b862a8bf38163335be27718ddfe5eb52867e12a23077`;
EFI loader checksum remains
`477ccfeff4d309e8549d68fdd232f836d29e100e4858513005f077a64586bf05`.

Updated system package SHA-256:
`6a40c785862ab19913a7cf0b9ef64c90fd5998cda2c22c6636edc8f3463b348a`.
Deployed directly through Haiku to Samsung's
`/boot/system/packages/haiku-r1~beta6_hrev99999-1-riscv64.hpkg`;
sync and installed-file SHA-256 verification passed.
First SCP disconnected after 64 KiB; retry at 8192 kbit/s completed and
verified before replacement. Previous package backup:
`/boot/pioneer-update-quiet-ahci-20260908/previous-haiku.hpkg`, verified
`e37b0f44f2957631d246d1ccf9a85865819a498f917ef669a8bd1c5183b61ac9`.
SD, firmware, apps, models and settings were not replaced.

Reboot and quiet-driver runtime verification remain pending. Ask user to
save work before normal Haiku shutdown, then wait for safe confirmation
before relay off/on. Keep SD inserted and select Samsung/latest state.
Repeat the same benchmark into a new output file after verifying boot;
do not treat the interrupted noisy run as a completed baseline.

Quiet-AHCI Samsung boot subsequently verified after user safe confirmation.
Serial start offset 37112326. Selected 111.79 GiB/latest state; SSH verified
Samsung `/boot` and package SHA-256 6a40c785862ab19913a7cf0b9ef64c90fd5998cda2c22c6636edc8f3463b348a.
No `sg_memcpy phyAddr` messages found in this boot's serial output.
Restarted the identical benchmark into
`benchmarks/haiku-qwen3-0.6b-q8-quiet-ahci-20260908.{jsonl,stderr}`.
Session 25741 is running; first prompt warm-up reached, no timing samples
yet at this checkpoint. Do not reboot or start another benchmark on top of it.

Quiet-AHCI benchmark subsequently completed all ten tests with exit 0.
64-thread generation: 0.143406 +/- 0.000036 tokens/s; mean 223.143459
seconds for 32 tokens across three repetitions. Approximately 83 times
slower than 32-thread generation (11.938151 tokens/s). Full results are
saved in `benchmarks/LLAMA_CPU_RESULTS.md`. Session 25741 is complete;
no benchmark remains running from this sweep. Cause of scaling regression
is not established; next work is runtime profiling/polling experiments,
not unrelated kernel or driver changes.

Scaling investigation: --poll 0 short probe stalled in its first 32-thread
warm-up. CPU sampler reports all 64 enabled but about three busy; most
llama threads READY with unrestricted masks. External per-worker pinning
through _kern_set_thread_affinity caused a null-address kernel panic in
_user_set_thread_affinity+0xe0, thread 848 cpu_activity, CPU 55. Board is
currently stopped at kdebug; do not interpret pending SSH sessions as live
progress. Saved serial tail and diagnostic samples under benchmarks/.
No kernel or llama source edits in this investigation. Do not rerun the
--pin diagnostic before reviewing/fixing the affinity syscall. Review
thread.cpp's unconditional thread->cpu dereference and scheduling/locking
semantics; a null guard alone does not establish correct remote migration.

## Affinity scheduler candidate, 2026-09-08

Implemented scheduler_set_thread_affinity under the scheduler/thread locks:
READY threads are dequeued/re-enqueued, RUNNING threads request rescheduling
on their actual CPU, and sleeping threads retain the mask for their next
wakeup. Reject conflicting masks during temporary CPU pinning with B_BUSY.
The syscall checks permissions/idle threads and releases the thread lock
before checking for local rescheduling. Affinity reads use scheduler_lock.
No changes to llama, PCI, DMA, GPU, or general scheduler balancing policy.

Image build exit 0; whitespace check passed. Runtime validation pending.
Kernel SHA-256: 436c35f47e1c301ee7792b3100fc8368e725ecf143b7cf83c29da241ae23d52b.
Package SHA-256: 584e55fc831215ab80ac8f6e5f6ef8d6ad00db9ac2292664fe8ff4eabe0b54c0.
SD payload SHA-256: d4231e076337f5ee9d71e86885f20b1088b7579f11cd26b05f187095616fefea.
SD /dev/mmcblk1 (serial 0x0000e752) p2 deployed and full readback verified.
Rollback: /mnt/ssd/haiku-deploy/haiku-pioneer-bfs-before-20260908T222102Z.img.gz.
Samsung remains on quiet-AHCI package 6a40c785...; firmware unchanged.
Linux clean shutdown requested and completed, then relay on issued for SD
boot. Serial start offset 37516388. No menu watcher: allow default SD boot.
Do not assume this candidate fixes performance until runtime tests pass.

SD candidate boot verified over SSH: /boot=/dev/disk/mmc/0/1 (300 MiB),
installed package hash 584e55fc... matches. Samsung mounted read-only at
/Haiku1. Saved Samsung cpu_activity binary has invalid ELF header (begins
4a1dce17), while llama-bench begins valid ELF magic. Cause of the damaged
diagnostic file not established; do not use it. Rebuilt cpu_activity on
Mac with cross compiler and staged it under /tmp on SD Haiku instead.

Disposable sleeping-thread affinity test succeeded with mask 1 and no panic.
Running shell-loop test returned B_BUSY on five spaced retries; test process
was stopped afterward. This candidate is not yet a validated migration fix.
STrap pins the entire user trap lifetime, including blocking syscalls and
timer-driven rescheduling, to preserve hart-local stvec/sscratch assumptions.
That pinning is a plausible cause of poor runnable-work distribution.
Do not remove the pin without reviewing arch context-switch/trap-return
state restoration. No new llama benchmark started in this SD boot.
SD Haiku remains running; Samsung still read-only and unchanged.

## Trap-migration candidate prepared, not deployed

arch_context_switch now explicitly sets stvec=SVec and clears sscratch
before loading the incoming kernel context. SVecURet already reconstructs
the user trap entry and kernel stack on the executing hart before sret.
Removed the blanket pin/unpin around the user STrap lifetime; other kernel
pinning remains. This targets the trap-state rationale for the earlier
pinning workaround without changing general scheduler balancing policy.

Image build exited 0; objdump confirmed the new CSR writes; diff check
passed. Added affinity_smoke.c (outside llama) to test suspended-thread
affinity followed by 100 self-migrations between CPU 0 and the last CPU,
checking placement before/after sleep. Cross-compiled with -Wall -Wextra
-Werror; /private/tmp/pioneer-affinity-smoke is ready, not run yet.

Candidate kernel SHA-256:
5902003af9a378625b17e9345f04dc7eca7b09c8458e7512764a497b6199ec46.
Candidate SD payload: /Volumes/HaikuBuildLocal/generated.riscv64/haiku-pioneer-bfs-trap-migration.img
SHA-256: bb81f152e2c3859aa8e47aa0a71c12ff70a1cf799468ba6df7c9878a9dad26a4.
EFI loader unchanged. Current SD still has affinity-only candidate
584e55fc... package. Need user save/shutdown/safe sequence, SD removal for
Linux boot, then deploy with rollback. No performance or runtime success
claimed for this migration candidate. Samsung remains unchanged.

Trap-migration payload subsequently deployed to verified SD serial
0x0000e752 via Linux, with complete 300 MiB readback matching bb81f152... .
Rollback: /mnt/ssd/haiku-deploy/haiku-pioneer-bfs-before-20260908T232120Z.img.gz.
Firmware and Samsung unchanged. Linux clean shutdown completed; relay on
issued with SD inserted. Serial boot offset 37883662. Default SD boot;
runtime migration and performance tests remain pending.

Migration SD boot confirmed. Fresh affinity_smoke binary transfer SHA-256
ca0bfec9bcfc88d85638ce0d2a4431d938240fc0b223b7b5b1f3dc58298b8cbf
verified; test exited 0, PASS for suspended affinity plus 100 migrations
between CPUs 0/63 and sleep/wakeup placement checks. Samsung mounted
read-only at /Haiku1. Existing llama runtime needs command-local
LIBRARY_PATH=/Haiku1/home/develop/llama-c060ca974c77/build-pioneer/bin:/boot/system/lib.
Without it, runtime_loader cannot locate libraries (exit 3).

Short generation probe -t 32,64 -p 0 -n 8 -r 1 --poll 50 exited 0:
32 threads 14.613447 tokens/s; 64 threads 13.773844 tokens/s. This is
preliminary, not directly comparable to the longer original benchmark.
Full original 4..64 settings restarted as session 80132, results in
benchmarks/haiku-trap-migration-full-20260908.{jsonl,stderr}. Do not start
another benchmark or reboot while that run is active. Samsung unchanged.

Session 80132 subsequently completed all ten tests, exit 0. Full results
updated in benchmarks/LLAMA_CPU_RESULTS.md. 64-thread prompt processing
38.931319 tokens/s; 32-thread generation 15.824186; 64-thread generation
only 0.260900. The longer test contradicts treating the successful short
probe as a complete fix. Investigate token count/run history and collect
CPU/thread samples during the slow phase. No benchmark remains running
from this sweep. SD Haiku still running, Samsung read-only and unchanged.

Isolated 64-thread generation: one 32-token repetition ran at 0.296190
tokens/s. CPU 8 was almost idle; two workers each received about half a
core, remaining workers near full cores. A second isolated run with three
repetitions ran at 12.785758 +/- 0.412390 tokens/s. Pinning team 926 was
attempted during warm-up but rejected B_BUSY on the first thread; retry
enumerated zero threads after completion. No pinning was applied. Renamed
second run raw files to haiku-migration-isolated64-repeat-20260908 to avoid
mislabeling. Both runs exited 0; no benchmark currently running. Next:
controlled affinity at worker startup to test placement/synchronization.
See benchmarks/LLAMA_CPU_RESULTS.md; root cause not yet proven.

## 2026-09-08: llama startup-affinity experiment

Added 38-line Haiku affinity branch in llama ggml-cpu.c, using private
libroot _kern_set_thread_affinity with CPU-index validation and uint32
bitmap layout. Existing strict-mask worker assignment is reused. Priority
handling, math kernels, kernel code, and installed Samsung runtime were
not changed in this experiment. Separate patch:
llama-c060ca-haiku-affinity.patch. Reproduction helper:
benchmarks/build_llama_affinity_backend.sh. No commit or push performed.

Built matched hybrid baseline/affinity CPU backends: only ggml-cpu.c.o
cross-compiled with GCC 13.3; remaining native GCC 13.2 backend objects and
runtime reused. Libraries staged in Haiku /tmp/baseline and /tmp/affinity,
transfer hashes verified. Samsung remains read-only at /Haiku1; SD Haiku
still runs the existing trap-migration kernel. No reboot or SD change.

Initial pinned probes: 13.021658 tok/s for 32 tokens, 11.238023 for 128,
three repetitions each. Complete mask snapshot confirms 64 distinct
single-CPU masks. Three fresh-process alternating pairs all completed:
baseline 10.922765 / 12.153996 / 12.217611 tok/s;
affinity 12.629153 / 12.330276 / 12.202060 tok/s.
Neither side reproduced the severe collapse in these paired trials.

Final-library 32->64 sequence, prompt128/generation32, r3: prompt
23.215656 / 34.774933 tok/s; generation 16.266823 / 13.412439.
All four results complete, exit 0. No affinity warnings. Affinity works,
but reliable elimination of the intermittent collapse remains unproven;
64-worker generation still trails 32. All sessions completed; nothing
benchmarking now. Full provenance, hashes, raw results, and caveats saved
in benchmarks/LLAMA_CPU_RESULTS.md. Next: repeated controlled trials and
targeted scheduler placement diagnostics, not unrelated hardware changes.

## 2026-09-08: deterministic scheduler imbalance and candidate

Original unpinned full sweep completed nine rows before the scheduler
profiler panicked during 64-thread warm-up. The bounded profiler smoke
test had worked earlier. Panic is storePageFault at 0xffffffc0208018b8,
SystemProfiler::_AllocateBuffer+0x4c, CPU12/thread2027, interrupts disabled,
printed leaf 0x7000000127ab68e7. Root cause of this mapping/translation
failure is not proven. Do not rerun profiler on this image. Saved serial
trace benchmarks/haiku-scheduler-profiler-panic-20260908.txt; incomplete
sweep haiku-scheduler-repro-20260908.jsonl, empty switch capture not valid
placement data. Added bounded scheduler_capture.cpp and trace analyzer;
analyzer rejects empty/inconsistent captures.

Recovered via relay off/on, same SD image, serial offset38348055. SSH
returned. Samsung was read-only before panic and has not been written.
No deployment or firmware change during recovery.

Safer scheduler_balance_probe.c seeds two of 64 busy workers on CPU0,
none on CPU63, warms for1sec, removes affinity, then samples for4sec.
All three runs exited0 with B_OK affinity calls. Workers0/63 stayed only
on CPU0, all others only on CPUs1..62; CPU63 received no probe worker.
This is direct placement evidence independent of llama and profiler.
Raw results in benchmarks/haiku-scheduler-balance-{probe,repeat}-20260908.txt.

Prepared narrow kernel candidate: scheduler_cpu.h adds GetUncappedLoad;
low_latency.cpp rebalance uses uncapped demand in its two comparisons.
Other utilization consumers, heap keys, power-saving mode, margin, and
affinity handling unchanged. Existing affinity/trap-migration fixes remain.
Candidate build session18343, log /private/tmp/pioneer-scheduler-demand-build.log,
payload /Volumes/HaikuBuildLocal/generated.riscv64/haiku-pioneer-bfs-scheduler-demand.img.
Not deployed or runtime validated yet. Next: SD-only deployment through
Linux after user save/safe/manual SD swap, then identical imbalance probe
and unpinned llama tests. Preserve Samsung installation.

Scheduler-demand image build subsequently completed, exit0. Payload
SHA-256 01dc43b1e36671a53bf23be46587f503ed61af4c55b73ca50bbe51769bd9ed41;
kernel SHA-256 ab47572fdb79946e2f0087cc33ccf517f0c7b87824fd15600317bcdaf512f186.
EFI unchanged (477ccfeff4d309e8549d68fdd232f836d29e100e4858513005f077a64586bf05).
Diff check passed. Awaiting safe shutdown/manual SD swap for SD-only test.
Recovered Haiku is currently running; no benchmark is active on the board.

Scheduler-demand payload deployed via Linux to verified SD /dev/mmcblk1
serial0x0000e752 (31299993600 bytes), p2 only. Backup:
/mnt/ssd/haiku-deploy/haiku-pioneer-bfs-before-20260909T040948Z.img.gz.
Complete 300MiB readback matched01dc43b1... . Firmware and Samsung untouched.
Linux shutdown -h now completed; waited10sec, then relay on (no relay off).
Candidate SD boot serial offset38714189. Runtime test pending.

Candidate boot reached SSH. Active packaged kernel SHA-256 is
3fe93a8af0fddadfa226ced3abf504e7b1e1ad4d6baa523849e005819101e110,
matching packages_build/regular/hpkg_-haiku.hpkg/contents/kernel_riscv64.
The earlier ab47572f... value is the unstripped build artifact, not the
deployed file. Affinity smoke PASS (100 migrations plus wakeups).
All three deterministic probes now move one of the CPU0 pair to CPU63;
the three pre-fix probes did not. Raw output:
benchmarks/haiku-scheduler-demand-probes-20260908.txt. Kernel fix validated
for this seeded imbalance; broad stability/llama results still pending.

Samsung mounted read-only at /Haiku1. Original native scalar llama full
4/8/16/32/64 sweep running as session12692, no affinity or profiler.
benchmarks/haiku-scheduler-demand-full-20260908.{jsonl,stderr}.
Do not reboot or start a competing workload until that run finishes.

Session12692 subsequently completed all ten rows, exit0. No benchmark
remains running. Full results added to benchmarks/LLAMA_CPU_RESULTS.md.
Prompt/generation tok/s by workers:
4:4.603126/3.148009; 8:8.827817/6.155621;16:15.538612/11.366162;
32:23.997733/12.939131;64:35.590468/10.610063 (r3 each).
No severe collapse in this unpinned, unprofiled sweep. Deterministic
imbalance fix confirmed; broad stability and Linux comparison remain.
Current state: candidate SD Haiku running, Samsung read-only at /Haiku1,
SD inserted. No firmware/Samsung writes or git commit/push. Next Linux
comparison requires user safe shutdown/manual SD removal. Do not rerun
the system profiler until its separate mapping fault is understood.

## 2026-09-09: Linux scalar comparison in progress

User removed SD while Haiku was still running. SSH confirmed Haiku;
sync/shutdown blocked on removed-card write timeouts. Saved results are on
Mac and Samsung was read-only. Relay off/on used to recover and boot Linux.
Do not assume the SD filesystem was cleanly unmounted; redeploy/check it
before relying on that test image again. SD remains removed.

Linux SSH verified: Fedora riscv64 kernel6.1.31, GCC13.2.1, CMake3.27.4,
64 CPUs, four NUMA nodes. Environment saved in
benchmarks/linux-environment-20260909.txt. New isolated staging directory:
/mnt/ssd/haiku-deploy/llama-linux-c060ca-N3stW2.
Transferred exact c060ca974c773c7c3d17fd1b66dc9d312bc292c0 git archive and
same Qwen3-0.6B-Q8_0 model from Mac, both hashes verified by build script.
Model retains .gguf.part filename but is the complete verified639446688-byte
file, SHA9465e63a... . No Samsung/SD writes or system-package installation.

benchmarks/build_linux_scalar.sh and linux-scalar-generic.cmake reproduce
build: native /usr/bin/gcc/g++, Release, rv64gc/lp64d, CPU_GENERIC, shared
libs, OpenMP/RVV/XTheadVector off. CMAKE_PROJECT_INCLUDE overrides only
processor dispatch to match Haiku's generic backend; actual compilers are
native RISC-V Linux. Source archive is unmodified upstream revision;
Haiku-only compatibility edits are not needed on Linux. GCC version/libc/
OS/NUMA differences must be recorded in comparisons.
Build session73569, log benchmarks/linux-scalar-build-20260909.log,
targets llama-bench and llama-completion, parallel32. Benchmark not started
yet. Linux remains running. Next: verify build flags, finish build, run
same unpinned4/8/16/32/64 sweep and save comparison in LLAMA_CPU_RESULTS.md.

Linux build session73569 finished all261 Ninja steps, exit0. Exact CPU
compiler command saved in benchmarks/linux-cpu-compile-command-20260909.txt,
confirming CPU_GENERIC, rv64gc/lp64d, O3, no OpenMP/vector flags. Full scalar
sweep started as session82346, outputs
benchmarks/linux-scalar-full-20260909.{jsonl,stderr}. No concurrent workload
or profiler. Old blocked Haiku shutdown SSH client53412 was identified and
closed locally; its session74908 ended255, not a clean Haiku shutdown.
Linux remains running; SD out; Samsung untouched. Await benchmark completion.

Linux sweep session82346 completed all10 rows, exit0. Through32 workers,
performance closely tracks Haiku. At64 Linux unpinned prompt8.912730 tok/s,
generation3.275249 +/-3.191252 (samples0.587546/6.80228/2.43592).
Fresh unpinned64 r3 session22821 completed2.053593 +/-2.173011.
Pinned64 r3 session43530 completed9.443906 +/-0.248929. Short mask snapshot
missed finished process (session18864 exit1); no placement proof in that file.
Long pinned64 n128/r3 session1460 completed8.889666 +/-0.711164; snapshot
session98094 verified64 distinct single-CPU masks0..63. All benchmark
processes exited0. No benchmark or build remains running.

Full comparison and caveats in benchmarks/LLAMA_CPU_RESULTS.md; raw Linux
results linux-scalar-full/64-repeat/64-pinned/64-pinned-long-20260909.
Do not claim general Haiku superiority from Linux's slow unpinned outliers.
Current scalar performance is broadly competitive; neither tested setup
scales generation better at64 than32. Next optimization candidates are
NUMA/worker placement and vector backend, with separate correctness checks.
Current machine state: Linux running, SD out, Samsung unchanged/unmounted.
No system-wide Linux install, kernel change, or git commit/push performed.

## Native assistant prototype (after Linux scalar comparison)

User approved temporarily switching from performance work to a native local
chat application, later global Ctrl+Space and permission-checked MCP tools.
Added src/apps/assistant (HaikuAssistant Jam target, app resources, README),
tools/pioneer/build_assistant.sh and tools/pioneer/haiku-assistant.sh.
No llama.cpp, kernel, boot, image, or Samsung changes for this task.

Prototype: native conversation/input window, background posix_spawn of existing
llama-completion, separate output/diagnostic pipes, streamed snapshots, Stop,
bounded recent conversation context, explicit Qwen3 non-thinking prompt.
Model reloads each turn; no global shortcut, resident backend, MCP, or automatic
system package inclusion yet. Full limitations and hardware checklist in README.

Cross-build passes -Wall -Wextra -Werror using SDK system headers. Resource-bearing
RISC-V binary /private/tmp/HaikuAssistant SHA256:
dcf03b26ff79e2cba922d0621b2e16633a6167ed0a112fe64b5869bb416beaa3.
Dependencies libbe.so, libstdc++.so.6, libroot.so. Shell syntax and diff checks
pass; missing-app launcher error checked. Linux matching-model command smoke
test session69583 exited0 with a short greeting. Native UI/cancellation/relaunch
still need Haiku testing; Linux test is not proof of native GUI correctness.

Linux SSH was verified running. SD remains out per last user report. No reboot
or deploy performed. SD was previously removed before a clean Haiku shutdown;
check/redeploy it before relying on it for the next boot. Samsung unchanged.

Night shutdown requested by user: verified Linux over SSH and issued
sudo -n shutdown -h now successfully (session30716 exit0). No relay off/on
or reset used. Resume with assistant hardware testing; source and cross-built
binary are saved on the Mac. SD last reported out; confirm before next boot.

## Assistant resident-service update, 2026-09-15

Linux boot verified with /Users/arma/.ssh/known_hosts.old (existing saved key).
SD 0x0000e752 inserted; firmware bbff6ac04... and EFI 477ccfef... unchanged.
Clean Linux shutdown then ten-second wait and relay on. Space watcher fired,
but this boot ultimately selected SD, not Samsung; /boot is /dev/disk/mmc/0/1.
Haiku host fingerprint verified against generated.pioneer/ssh public key:
SHA256:8eyC1ZQLmJwM6moJpEMuj5CFOcR9B672b8TNsosuFSs.
Known-host file restored at /private/tmp/pioneer-haiku-verified-known-hosts.
Samsung mounted read-only at /Haiku1 (/dev/disk/scsi/0/3/0/0).

Original chat GUI installed on SD and user confirmed it works. User requested
resident inference plus OS-wide shortcut. Implemented native BApplication
service using installed llama shared libraries, message IPC, bounded requests,
fresh per-request KV/sampler state, cancellation and persistent model/context.
GUI starts service on first launch; service survives GUI exit. No automatic
boot preload yet. Ctrl+Space input filter asynchronously launches/toggles GUI.
No kernel, firmware, llama source, Samsung, or existing Shortcuts changes.

Cross-build outputs and installed hashes:
HaikuAssistant: 7cd49d963b25124ea37c3e1909b0cf5c5531d19735ab3987118b0db3acbbf3c7
HaikuAssistantService: 902b64264444db81bb76c4c85fc02e7ef0a8bcdb0e3cde775b1aea0dd715c82b
HaikuAssistantShortcut: c0d57845e49da39c642d677ecfde2df98901ad2fb9d489ac5576304cdef548c3
Control executable installed as /boot/home/config/non-packaged/bin/assistantctl.
Native libraries copied for cross-linking to /private/tmp/pioneer-assistant-libs.
Service running from /tmp/HaikuAssistantService, team598; log /tmp/assistant-service.log.
Permanent copy installed under non-packaged/servers/HaikuAssistantService.

Two short asks, cancellation, and another successful ask all left team598,
ready=1 busy=0 model_loads=1. Sessions27570/19991 completed0. listimage confirms
shortcut image6817 loaded into input_server team211. No input restart.
Old GUI team546 still running as of last check. User asked to close it before
using Ctrl+Space to launch new version; physical shortcut/new GUI confirmation
pending. Backup non-packaged/apps/HaikuAssistant.before-resident-20260915.
See src/apps/assistant/README.md for current limitations and packaging work.

User confirmed updated GUI/shortcut work and approved Samsung deployment.
Verified Samsung /Haiku1 is /dev/disk/scsi/0/3/0/0, 111.8GiB. Stopped idle
service598, unmounted normally and remounted writable. No target assistant
files existed; deployment refused overwrites and added only five files under
/Haiku1/home/config/non-packaged: apps/HaikuAssistant,
servers/HaikuAssistantService, add-ons/input_server/filters/HaikuAssistantShortcut,
bin/assistantctl, bin/haiku-assistant. First three hashes match above.
Control SHA e98fe0a428c821700dc75d24271fa1dcc009d15a2b23074ed0f16519563a3997.
Launcher SHA bf92f61a5cde61ecc3c55b44092c73b802f05893109519db4def10bf62fb162c
(tools/pioneer/haiku-assistant.sh, Samsung /boot paths, not SD launcher).
All destination hashes verified. Synced, unmounted and remounted Samsung
read-only (device9). Restarted SD-installed service as team877. No kernel,
bootloader, runtime/model or existing user settings changed. No reboot.
Samsung-boot validation remains pending. Proposed system-tools/MCP service
has not been implemented; this deployment contains only the working chat suite.

## Native system-tools service, 2026-09-15

User requested MCP server, then clarified "or service". Implemented native
HaikuAssistantTools background BApplication and haiku-tools CLI. This is not
MCP/JSON-RPC and not yet connected to Qwen's automatic tool selection. Source:
src/apps/assistant/ToolsService.cpp, ToolsControl.cpp, ToolsProtocol.h,
ToolsService.rdef, TOOLS.md; Jam/build_assistant.sh targets added.

Tools: system_info, list_apps, list_windows, launch_app (Terminal/StyledEdit/
DeskCalc/WebPositive only, no args), move_window, resize_window, focus_window.
Mutations require native one-shot approval, default Deny, 60-second expiry.
Random nonce binds response, one pending action, exact target revalidation,
on-screen bounds, immovable/unresizable checks; no shell or filesystem tools.
Request/list limits and bounded IPC replies. Local consent gate, not sandbox.

Read-only native tests passed, reporting64 CPUs and136289968128 bytes total.
Unknown shell tool, /bin/sh launch and nonexistent-window requests rejected.
Initial approval test exposed BMessage copying strips synchronous reply flags;
fixed using DetachCurrentMessage with exactly-once reply/delete after consent.
Approved DeskCalc launch succeeded, observed team985/token24. User confirmed
seeing permission prompt. Move request to100,100 was denied or expired and
list_windows confirmed unchanged frame1020,691,1242,832. Thus approved launch
and denied/expired mutation tested; successful move/resize/focus still untested.

Final service SHA6f80a9404cb3b1aca52812af8db2571f95ad3e9659cd76050e77035602538192.
CLI SHA0bbc801f01151352f201151935e9cc9b1b23af54d288b98f1566bce27c938af2.
Installed SD paths non-packaged/servers/HaikuAssistantTools, bin/haiku-tools.
Previous test build backup /boot/home/HaikuAssistantTools-before-bounded-replies.
Service log /tmp/assistant-tools.log. Final build read-only/allowlist smoke tests
rerun after activation. Build and diff/shell checks passed. Samsung not changed
by this tools-service work; earlier chat/service/shortcut deployment remains.
Current OS still SD Haiku; Samsung /Haiku1 read-only. No reboot/kernel/firmware
change, git commit or push. Next: connect structured chat tool requests/results
to native IPC while preserving approval; actual MCP adapter remains optional.

Final tools service is team1031. Process inspection confirmed deployment shell
and sync had exited; session57535 remains open because CLI auto-launch via
BRoster inherited the SSH descriptors. Service is functional, not a hung sync.
For clean remote management, start the service explicitly with stdin=/dev/null
and stdout/stderr redirected to /tmp/assistant-tools.log before using CLI.

## Chat tool bridge integration, 2026-09-15

Added ToolBridge strict JSON parser and ToolSession native orchestration shared
by GUI and ToolFlowTest. Limits: one tool per generation, three per turn,
bounded result/prompt sizes, watchdog and cancellation. Model does not supply
approval; native consent remains mandatory for desktop mutations. Read-only
results are visible before the model summary. No MCP transport, shell tools,
kernel, boot or llama-source changes.

Cross-build and all 17 parser cases passed on SD Haiku. Initial end-to-end
CPU/RAM test produced invented plain text instead of a tool call; this was
NOT a successful integration test. Added explicit examples and tightened test
success criteria. Second run issued a native request and received a result;
final model answer is still being checked. Current GUI remains old version.

Updated SD model service SHA38bf01b8f75b9ae4a99b46d5615023eb8930fd6c85317d1f215ec1a1bf844e1d
preserves special tool delimiters. Tools service
SHAe95d21f7aa6519d1f10a460017a92b2829a105dd9aacb39b176971bc1b5ca65f
echoes request IDs and supports same-caller cancellation. Backups in /boot/home:
HaikuAssistantService.before-tool-flow, HaikuAssistantTools.before-tool-flow.
Model service team1218, ready=1, model_loads=1 throughout the two test requests.
Samsung stays read-only and unchanged. New GUI staged as
/tmp/HaikuAssistant.tool-flow, not yet activated.

Completion: second end-to-end run returned [Tool: system_info], actual64 CPUs,
136289968128 bytes, then a model answer. Qwen incorrectly converted bytes to
13.63 GB; native output now formats126.93 GiB itself (direct query verified).
Tool selection/summary reliability remains limited; do not claim arbitrary
queries are reliable based on this single success. No mutation model test yet.

Final GUI SHAda09ddb62a0ac979dfe4e17e70625cd8a4340a44656965ec8f4ef9892d74192e
installed SD atomically; old open GUI not killed. User must close/reopen via
Ctrl+Space. Backup /boot/home/HaikuAssistant.before-tool-flow. Final tools
SHA877f8e5dff03256d10eeef6628ee1e32614af14e9fafec9f3406980c747296f8
installed and queried. Checksums matched, final parser tests all passed.
Latest test harness uses session tool_calls metadata instead of trusting a text
marker; end-to-end run preceded that instrumentation-only update. Watchdog
initialization failure now fails closed. Model remains team1218 ready1 busy0
model_loads1. Samsung unchanged, no reboot or kernel changes.

Follow-up source adds a persisted CPU-thread setting. It defaults to the lesser
of 32 or the available CPU count, can be changed in the GUI or with
`assistantctl threads N`, and is captured per request so the next request uses
the new value without reloading the model. Pioneer deployment/testing of this
settings follow-up remains pending. Final warning-as-error cross-build hashes:
GUI dfbcd69a805fb722cfab18b1b1092d05a28f97009b53e821761c1b5217d82388;
service d7329b2a8418d3e4d44e1e918997b357ddb7129b13ee6ad5c8a987b6e1304aa0;
shortcut c0d57845e49da39c642d677ecfde2df98901ad2fb9d489ac5576304cdef548c3;
tools 877f8e5dff03256d10eeef6628ee1e32614af14e9fafec9f3406980c747296f8;
tools control 0bbc801f01151352f201151935e9cc9b1b23af54d288b98f1566bce27c938af2.

CPU-settings deployment verified on SD: installed GUI/service hashes match the
above; assistantctl SHA8a6da17119160b687c81dcf10d07eebcda8ba0ab5c1065d8d177c423ae5f3d58.
New service defaulted32/max64. Counts0 and65 rejected. Short inference at4,
then32 succeeded on the same team1583/model_loads1; status confirmed each
last_request_threads count. Saved32 survived service restart: team1731 ready1,
busy0, cpu_threads32. Settings file /boot/home/config/settings/HaikuAssistant_settings.
Backups /boot/home/HaikuAssistant.before-cpu-settings,
HaikuAssistantService.before-cpu-settings, assistantctl.before-cpu-settings.
Old open GUI was not killed; close/reopen to see CPU threads / Apply controls.
GUI interaction still needs user confirmation. Samsung untouched, no reboot.

## 2026-10-12: read-only runtime recovery and bounded smoke tests

Following the verified SD/xHCI recovery, identified the Samsung by its
120,034,123,776-byte capacity and existing AHCI device path. Mounted only
`/dev/disk/scsi/0/3/0/0` at `/Haiku1` with `mount -ro -t bfs`. `df` verified
BFS, 111.8 GiB total, 82.1 GiB free, and flags `QAM-P--` (no writable flag).
The Samsung system package still matches the September 8 quiet-AHCI record:
`6a40c785862ab19913a7cf0b9ef64c90fd5998cda2c22c6636edc8f3463b348a`.
No Samsung files, system packages, firmware, boot settings or partitions were
changed. The working SD remains the boot volume, with P100A removed.

The missing assistant library/model paths were unavailable because Samsung
was not mounted. The saved runtime and model are present under
`/Haiku1/home/develop/llama-c060ca974c77/build-pioneer/bin` and
`/Haiku1/home/models/Qwen3-0.6B-Q8_0.gguf`. The complete 639,446,688-byte
model hashes to the recorded reference:
`9465e63a22add5354d9bb4b99e90117043c7124007664907259bd16d043bb031`.
Runtime reports commit `c060ca974c77`, GNU 13.2.0, Haiku other. The Samsung
CMake executable runs and reports 3.31.8; GCC 13.2.0 and Make 4.3 package
archives are present. GCC, G++, Make and CMake were not found in the current
SD shell's PATH. A read-only BFS mount does not activate Samsung's development
packages in the SD packagefs; no compiler was installed in this session.

Haiku `sysinfo` reports 64 CPUs and 136,289,968,128 bytes total RAM
(126.93 GiB). Ran six CPU-only llama-bench cases: prompt 16 tokens and
generation 8 tokens, each with 4, 32 and 64 workers. Parameters were batch
and microbatch 32, GPU layers zero, one repetition, no warmup, offline mode,
JSONL output and no explicit CPU affinity. The 4-worker command and combined
32/64-worker command both exited zero under 90-second deadlines. All six
JSONL rows identify CPU backend and positive completion times. These tiny,
single-repetition checks are not sustained-load, all-RAM, write-I/O or
performance-comparison validation.

Checksummed the first 64 MiB of the model on Haiku, then streamed the same
bounded read through SSH and checksummed it on the Mac without saving its
contents. Both commands exited zero and transferred/read exactly 67,108,864
bytes. Both SHA-256 results were
`3616867eba00d6d0aaeb3beaf32c0aadc88a92e5a2bcb3a007ff57e5c7375c4f`.
This validates that read/transfer sample, not full storage or network stress.

Installed assistant GUI, service and control hashes still match the September
15 CPU-settings deployment above. Started the existing service with the saved
runtime in command-local `LIBRARY_PATH`, logging only to
`/tmp/pioneer-assistant-smoke-20261012.log`. It loaded the fallback Samsung
model as team 713, `ready=1 busy=0 model_loads=1`, CPU threads 32/max 64.
Cancellation returned `Stopped. Model remains loaded.`; a subsequent request
returned `Hello! How can I assist you today?`. Same team, readiness and one
model load were retained after all requests. No settings or executable changed.
The service is left resident and Samsung must remain mounted while it is used.
New-boot GUI/shortcut interaction has not been physically reconfirmed.

Answer-quality caveat: the service returned `2` for "Reply with only the
number: what is 2 plus 2?", despite reporting successful execution. Direct
completion with the same ChatML/non-thinking prompt also returned `2`, using
temperature zero and seed 1234. A service follow-up correctly answered
3 plus 5 as 8. Thus the first arithmetic test failed semantically; successful
IPC/inference execution does not imply correct answers. The direct comparison
points away from an IPC-only explanation, but does not isolate prompt, model,
runtime/numerical or hardware causes. No answer-quality fix was attempted.

Ignored raw evidence is under `generated.pioneer/stability-20261012/`:
`cpu-smoke-{4,32-64}.{jsonl,stderr}`, `model-prefix-*`,
`assistant-{smoke,followup,cancel-recovery}.*`, `completion-arithmetic.*`,
`final-state.*` and `smoke-serial.log`. Serial slice 1260522..1380324 contained
no panic, assertion or P340/store/load-fault diagnostic during these checks.
Desktop/input/network services and SSH remained running. No reboot occurred.
Next hardware milestone is a controlled Samsung boot after the user saves
work; keep the SD inserted for its firmware/loader and preserve existing
rollback packages. Do not replace Samsung's whole package with the SD package merely
to copy the xHCI fix; audit any future Samsung driver update independently.

### SSD-selection attempt and SD-free boot direction

The user saved work and approved shutdown/restart. Issued `sync; shutdown`,
waited twenty seconds and confirmed SSH connection refusal, then relay off
(seven seconds), ten-second wait and relay on (0.75 seconds). The SD remained
inserted and P100A removed. No package or firmware was changed.

A bounded watcher saw `[Bds]Stop Hotkey Service!` after fresh serial byte
offset 1383772 and sent one Space through the existing screen session. The
loader did not enter the boot menu; it selected and mounted
`/dev/disk/mmc/0/1` again. This is not a Samsung boot test. The user reported
Haiku was up, and serial reached graphical userland. Several pinned SSH
connections to 10.0.0.247 timed out; two ICMP requests also timed out. The
cached ARP entry still matches the recorded Pioneer MAC, but does not prove
current reachability. Early P340 store-fault diagnostics appeared in this boot;
no fatal panic line was found in the inspected capture. Do not classify this
boot as network-validated. No second restart or forced power-off was performed.
Evidence: ignored `stability-20261012/ssd-selection-attempt-1-boot.log`.

`console_check_boot_keys()` polls EFI input only three times, with 100 ms
stalls, before the loader welcome message. The serial reader's log flush
interval can delay a marker watcher relative to that window. Timing is a
hypothesis, not an established explanation for the missed Space. Do not
send volume-selection keys unless the menu is actually observed.

The user proposed SD-free, remotely selectable booting for unattended port
development. The saved October 12 no-SD Linux log establishes the relevant
constraint: SPI loads OpenSBI, Linux 6.1.47 and an initrd; that LinuxBoot
environment kexecs Fedora 6.1.31 from NVMe. Both Linux entries report
`efi: UEFI not found`. Thus copying an EFI executable to NVMe or relying on
EFI BootNext is not sufficient for the currently installed SPI boot path.
Haiku's deployed EFI loader requires live EFI services for disk reads.

Proposed non-flashing direction, not implemented or hardware-validated:
retain the known SPI/Linux boot as recovery, add a kexec-compatible Pioneer
Haiku loader/adapter, preload verified boot resources from disk, and select
the root volume by stable identity. Consume a one-shot Haiku request before
handoff; a host-side timeout and relay restart should then return to default
Linux rather than repeatedly boot the failed candidate. A second proven
Haiku build can be retained independently. The existing non-EFI RISC-V
loader has a hart-ID/device-tree entry point, but its disk path is VirtIO
and its console/startup assumptions are not a validated Pioneer kexec port.
Start with a serial-only handoff probe, then RAM boot resources, then real
Samsung identity/mount and secondary-CPU validation. CPU state, reserved
memory and device/DMA quiescence must be checked; a menu-selection function
alone does not solve this bootstrap work. Linux's published entry contract:
https://docs.kernel.org/arch/riscv/boot.html
SPI replacement is a separate option requiring recovery planning and explicit
approval, not part of this investigation. Current board remains on SD Haiku.

## 2026-10-13: SD-free kexec entry probe candidate

Implemented `tools/pioneer/kexec_probe/` and separate build/read-only preflight
scripts as the first bootstrap milestone. This is a freestanding serial-only
RISC-V Linux Image, not a Haiku kernel, disk loader or working boot selector.
It receives the incoming hart/FDT, captures entry CSRs, validates bounded FDT
header/physical-RAM limits and reads SBI Base/HSM status. It neither starts nor
stops harts and makes no disk, PCI, flash or persistent boot-setting changes.
A private supervisor trap stack reports unexpected exceptions. Every result
deliberately halts and requires the planned relay recovery to default Linux.

Host ASan/UBSan tests passed production FDT checks and diagnostic control flow
with mocked physical-address/SBI/console calls. Six Python test groups passed
Image header rejection checks. The existing GCC 13.3.0 cross toolchain built
the RV64 IMAC soft-float candidate with warnings as errors, no runtime imports
or retained ELF relocations. Raw bytes are identical when linked at 0x2200000
and 0x80200000. The fixed-linked debug ELF is not a kexec target. Raw artifact:
`generated.pioneer/kexec-probe-20261013/build/PioneerKexecProbe.Image`, 24,576
bytes, SHA-256
`78de12a2de7f8bdf9ccdd1864bb822b505c0e8cb64c94d9646b6aada7da930c9`.
No emulator or hardware execution is established by these local checks.

The user confirmed Haiku work saved, then used Deskbar shutdown because pinned
SSH was still unreachable. After the user confirmed shutdown finished/safe to
power off, the seven-second relay off hold completed. SD removal remains gated
on physical confirmation that fans/LEDs are off. Native Linux preflight, actual
installed kexec Image support, handoff diagnostics and recovery are pending.
The SD, Samsung system package, SPI and Linux boot configuration are unchanged.

### First native attempt and diagnostic follow-up

The user confirmed fans/LEDs off and SD removed. Relay on booted the unchanged
SPI/LinuxBoot path into Fedora 6.1.31, with pinned Linux SSH, NVMe root, all 64
CPUs online and no MMC storage. Read-only preflight passed CONFIG_KEXEC and
CONFIG_HOTPLUG_CPU, live FDT and unoccupied/enabled load interface. Samsung
remained unmounted; only verified R3SL `/dev/sdb2` at `/mnt/ssd` was used for
private staging in `/mnt/ssd/pioneer-kexec-probe-20261013`.

Installed kexec-tools 2.0.26 handles only `elf-riscv`. Built unmodified upstream
commit `0fd0d8e9ea975696fdc05ad267a65f3efd415612` natively with existing GCC
13.2.1/autoconf, no system install or additional packages. Source archive
SHA-256 `28011794cdc4320b2440bcdbb723242d6c413bbfd0ede3f6bae0dac60d47c75a`;
private 2.0.32.git executable
`06aa5c6fa6e78bdf92abefb4e7f4b0accadfe6212a02c700905392f8d8056748`.
Saved original live FDT SHA-256
`e9264e1696ae23bdde4a2a20c0831215b4ea8967d2dd90ccba5a910aaa9befdd`.

Version 1 load with `-c -l -d -t image-riscv`, saved DTB and memory bounds
0x2200000..0xbfffffff failed EINVAL with kexec_loaded still zero. Debug showed
entry 0x2200000, FDT segment 0xbfff5000 and syscall flags 0xf30010. Current
userspace adds KEXEC_DEBUG (0x10) for `-d`; Linux 6.1 rejects unknown flags.
Retry without `-d` exited zero and kexec_loaded became one. Used normal
`systemctl kexec`, not direct `kexec -e`. SSH went away, but no new serial bytes
or probe marker appeared after offset 1729522 within the bounded observation.
No entry/hart/console success is claimed. Relay off/wait/on returned to pinned
NVMe Linux and kexec_loaded zero. The retained journal does not contain this
attempt's final shutdown record; recovery shows journal/EFI-FAT warnings that
were also present before the test, not a diagnosed new corruption from it.

Prepared version 2 diagnostic candidate: direct UART32 polled output instead
of legacy SBI putchar, with all original read-only SBI state queries retained.
Live FDT confirms the exact UART range, shift 2 and width 4. No clock, baud,
FIFO or interrupt-register changes are made. Divisor-bank selection and bounded
poll timeout halt without PASS. UART host mock tests, existing sanitizer tests
and byte-identical relink checks pass. Candidate is 28,672 bytes, SHA-256
`b57754d066aaaaa96ad285faff00a9efd30d5e4b24351b7d37f52f5d865f70e7`, in
`generated.pioneer/kexec-probe-20261013/build-uart/PioneerKexecProbe.Image`.
This console change is not yet a proven explanation or fix for the silent test.

Version 2's reviewed load-only operation also exited zero/kexec_loaded one.
Normal `systemctl kexec` at 17:41:14 UTC was observed in a live host-side journal
stream, including logind's accepted kexec request. The stream ended when SSH
closed; it does not prove final filesystem unmount, SMP shutdown or payload
entry. No serial bytes followed offset 1793524 through 17:44:39 UTC, so this
attempt is also inconclusive, not PASS. Began the planned relay recovery.
Next control is the existing Linux kernel/initrd with temporary serial-enabled
arguments, not a saved boot-entry or firmware change.

Version 2 recovery returned pinned NVMe Linux with kexec_loaded zero. For a
Linux-to-Linux control, decompressed the existing `/boot/vmlinuz-6.1.31` into
private staging and copied its existing initramfs without modifying originals.
The decompressed file is a RISC-V Linux Image with a PE/EFI header, not ELF;
its Image header has version 0.2 and text offset 2 MiB. It can use the same
private `image-riscv` loader without invoking EFI services. Artifact identities:
compressed kernel `eae39971f09c29b94d7fc00de22241cb92dabf8ce960be226686cb1911b271e3`;
decompressed 31,954,432-byte kernel
`b77ee6f003e0ffc74444880b93d9a9d903bad550ef9ca5b0b232b07a7a78fbef`;
initramfs `2abfdf6268cdaa9beab1297525d6812c821e27bc72677e80fb5cf6db8c7c5747`.
Saved current FDT matches the earlier live-FDT hash. Reused running command
line and appended `console=ttyS0,115200 keep_bootcon ignore_loglevel loglevel=8`
only to the staged in-memory DTB. No saved boot arguments were changed.
Control load-only returned zero/kexec_loaded one, then normal systemctl kexec
was requested at 17:50:55 UTC after fresh serial offset 1857526. This tests the
same Fedora handoff path using its known Linux payload; it is not a Haiku boot.

The Linux control produced zero serial bytes through 17:54:00 UTC and did not
return pinned SSH within the observation window. Therefore the current failure
is not isolated to probe code: the shared load/shutdown/relocation path remains
unvalidated even with the known Linux payload. This does not identify the cause
or prove which final shutdown/entry stage ran. Began relay off/wait/on recovery;
do not implement automatic Haiku selection atop this unproven handoff. The next
diagnostic needs outgoing Linux kernel serial visibility, not more silent probe
retries, and must preserve the current cold-boot recovery path.

Fresh-log comparison identifies an additional control variable: SPI/LinuxBoot
6.1.47 boots with `maxcpus=1` and reports one online CPU before its successful
handoff to Fedora; Fedora 6.1.31 subsequently brings up 64. Thus secondary-CPU
shutdown is a testable hypothesis, not yet an established cause. A temporary
native Linux CPU-hotplug check is possible without saved boot-setting changes;
any failed check must recover/restore the CPU mask before further handoff work.

Native hotplug check offlined logical CPU 1 and restored the full 0-63 mask.
The reported IRQ60/61 affinity warnings are Radeon/HDMI on the first PCIe
controller; their interrupt counts were already on CPU 0. Disk/network use
the separate controller. The guarded `pioneer_linux_cpu_quiesce.sh` then
offlined CPUs 1-63, retaining CPU 0, and exited zero. Pinned SSH and a fresh
read-only preflight worked in that state. This proves the tested runtime
hotplug operations, not final kernel shutdown or independent HSM status.
The helper changes only runtime CPU-online controls, requires native known
board/NVMe/no-SD and an unoccupied kexec interface, checks the original full
mask, bounds individual operations and attempts mask restoration on errors.
A stuck kernel operation can still require relay recovery.

Loaded identical version 2 probe/FDT in the same memory bounds, then requested
normal systemctl kexec at 18:04:32 UTC with only CPU 0 online. Fresh serial
offset is 1921528. This is the single-CPU hypothesis check, not a new payload.

The single-CPU attempt also produced zero serial bytes through 18:08:19 UTC.
Therefore pre-quiescing the secondary CPUs alone is not a proven workaround;
the final entry/console state remains unobserved. Began the planned relay
recovery, which should restore the default full CPU mask. Next check is the
native Linux UART's actual line state/output, since version 2 preserves baud
settings rather than initializing a known line configuration itself.

Cold recovery returned pinned NVMe Linux with CPUs 0-63 and kexec_loaded zero.
Verified `/sys/class/tty/ttyS0/device` is the exact `7040000000.serial` node.
Native `stty` reports 9600 baud, despite the host's 115200 capture. This is a
runtime termios observation, not proof of the pre-open physical line state or
the cause of previous silence. Explicit runtime stty 115200/raw/no-flow-control,
with HUPCL disabled, followed by a Linux TX marker reached the host capture:
`PIONEER_LINUX_UART_READY v1`. No saved serial/boot configuration changed.

The CPU helper's complete restore path was also tested: quiesce to CPU 0,
restore to 0-63, then quiesce again, all successful. After the final quiesce,
`PIONEER_LINUX_UART_READY v1 cpu=0` also reached the host. Loaded the unchanged
version 2 probe/FDT in the same bounds, then normal systemctl kexec at
18:16:23 UTC after fresh offset 1986942. This calibration does not itself prove
UART survival through shutdown or payload entry; actual probe markers remain
the success criterion.

The calibrated single-CPU attempt still produced zero new serial bytes through
18:21:05 UTC. Neither a probe BEGIN/PASS/FAIL nor a supervisor-trap marker was
observed. This is not a successful handoff; calibrated Linux TX and runtime
CPU-hotplug success do not establish the state after final kernel shutdown.
Began the approved relay recovery to normal Linux. Do not repeat silent probes
or build automatic Haiku selection on an assumed entry success. Next diagnostic
is outgoing-kernel serial visibility so the final driver/SMP/relocation stages
can be distinguished. Original Linux boot entries and SPI remain unchanged.

Final recovery verified pinned SSH on Fedora 6.1.31, root `/dev/nvme0n1p3`,
CPUs 0-63 and kexec_loaded zero. The SD stays removed. Samsung remains
unmounted and the known Linux cold-boot path is retained. All host sanitizer,
format/UART and Image-header tests still pass; hardware entry remains unproven.
Read-only boot-entry inspection: `/boot/extlinux/extlinux.conf` default f38MVP
matches kernel/initrd/root identities and already specifies console=ttyS0,115200;
its SHA-256 is `53c0da6d26b93ac6e6a152c8146296325a9c25f54409ef7e6dcfc83e3b1fad9c`.
Actual Fedora command line instead ends console=tty1, with the early UART
bootconsole disabled on registration. Thus do not assume editing a serial
console token in that file is sufficient or that its arguments are used
unchanged by SPI/LinuxBoot. `/boot/grub.cfg` is an older 6.1.22/mmc entry;
`/boot/grub2/grub.cfg` and `/boot/efi/EFI/fedora/grub.cfg` are absent. None of
these files was changed. A temporary, backed-up keep_bootcon experiment is a
possible next logging diagnostic, not an implemented selector or proven fix.

### Approved temporary boot-logging experiment

The user approved a temporary Linux boot-logging change with backup/rollback.
Rechecked pinned Linux SSH, NVMe root, CPU mask 0-63, kexec_loaded zero and the
unchanged extlinux hash. Created an exact metadata-preserving backup at
`/mnt/ssd/pioneer-kexec-probe-20261013/logging-change-1/extlinux.conf.before`.
Patched a local copy using apply_patch and audited the full diff: only the
f38MVP append line gains `keep_bootcon ignore_loglevel loglevel=8`. Other
entries, default choice, kernel, initrd, FDT, root and existing arguments are
unchanged. Linux 6.1 documents keep_bootcon as retaining its boot console.

Candidate SHA-256
`97f7f3085719f6a9e9dbf79a47c6778fd320fafaf82a57d81d99ba38de835d70`,
918 bytes, root:root mode 664. Installed via a same-directory temporary copy
with preserved attributes, hash/mode checks, sync and atomic rename. At
20:37:32 UTC requested clean shutdown; fresh serial baseline is 2052292.
No kexec image was loaded for this cold boot. The original must be atomically
restored and hash-verified as soon as pinned SSH returns, before any new probe.
Whether SPI/LinuxBoot passes through these logging arguments remains a test,
not an assumed property of the candidate file.

The cold test did pass the three new arguments into Fedora: serial captured
the full command line including keep_bootcon/ignore_loglevel/loglevel=8 and
continued past the old bootconsole cutoff. Last visible line is at 3.358654
seconds, after UART0-3 driver registration. Pinned SSH did not return during
the bounded cold-boot checks. This logging boot is not healthy; whether it
exposed or caused the stop is not yet isolated. The logging candidate remains
installed until recovery can restore the exact original backup. No SPI or
Samsung change occurred. Began a bounded LinuxBoot recovery interruption;
never send rollback shell commands until an actual recovery prompt is observed.

Recovery interruption: watcher used a fresh offset 2142064, observed LinuxBoot
userspace initialization, waited two seconds and sent exactly one Ctrl-C via
the existing serial reader. No recovery prompt appeared; firmware continued
its normal handoff to Fedora with the temporary logging arguments. Again the
last visible Fedora line is after UART0-3 registration (3.360801 seconds).
Pinned SSH and two ICMP probes failed; ARP for the known address was incomplete.
No shell commands were injected and no additional boot-entry edit was made.

Issued the seven-second relay off hold at 20:57:11 UTC. Recovery is unfinished:
the candidate `97f7f308...` is still on NVMe `/boot/extlinux/extlinux.conf`.
The original `53c0da6d...` remains intact on the separate R3SL staging SSD at
`/mnt/ssd/pioneer-kexec-probe-20261013/logging-change-1/extlinux.conf.before`.
An independent recovery boot is required to mount the known NVMe boot
partition `/dev/nvme0n1p2`, restore that exact original via same-directory
temporary copy/atomic rename, preserve root:root mode 664 and sync/hash-check
it before a normal boot. Do not flash SPI, reformat storage or overwrite the
working Haiku SD as a recovery shortcut. Do not report Linux restored or
resume kexec probes while this rollback remains pending. User physical-display
confirmation and available recovery-media information are pending.

### Spare-SD recovery preparation

The user physically confirmed power is off and has only the working Haiku
SD, but offered a spare SD for Linux recovery. Leave the board off and the
Haiku SD unchanged. Asked the user to connect only the spare to this Mac,
state its capacity, and authorize erasing that exact spare; those details
and target identification are still pending. No removable-media write or
new Pioneer boot has occurred, and the NVMe logging rollback is still pending.

Downloaded the Fedora image currently linked by Milk-V's Pioneer download
page, `fedora-disk-gnome-workstation_livecd-f38-20231010-033114.n.0-fix.raw.xz`,
from public Google Drive file `1-tYHNXjX6oekNorOGlWb_Az2nVOoey6y`. This is
temporary recovery media, not an OS reinstall on NVMe or a firmware-flash
update. Compressed size is 2,617,107,208 bytes; XZ declares 13,555,990,528
uncompressed bytes, so verify the spare's actual capacity before imaging.
Local compressed SHA-256 is
`4c38d4a2c53e301fb3ae0221a0d9e82ab25f73a04d1d44a956b2daecc384e680`;
this is a locally computed digest, not a vendor-published authentication hash.
The full `xz --test` CRC64 integrity check completed successfully (exit 0).
The download's `.part` suffix was removed only after this validation.
Artifacts are under ignored `generated.pioneer/linux-recovery-20261013/`.
Before writing/booting recovery media, inspect its partition/boot configuration,
verify it selects the SD root rather than the modified NVMe entry, and identify
the authorized spare by fresh disk inventory. Verify imaging readback; do not
write the original Haiku card, internal Mac disks, NVMe, or SPI firmware.

Also reconstructed `extlinux.conf.original` locally from the audited
candidate by removing only the three experimental arguments. It is exactly
878 bytes and matches the original backup SHA-256
`53c0da6d26b93ac6e6a152c8146296325a9c25f54409ef7e6dcfc83e3b1fad9c`.
This provides a verified local rollback copy in addition to the intact R3SL
backup. It has not been installed. Restore and hash-verify the original on
the identified NVMe boot partition before any further kexec experiment.

The spare is now physically identified on this Mac as `/dev/disk6`, Built In
SDXC Reader, Secure Digital protocol, removable physical media, exactly
31,299,993,600 bytes. It is LSMSSD0, manufacturer 0xad, serial **0xeb42**,
not the working Haiku card's serial 0xe752. Its original single FAT32 volume
is `NO NAME`, UUID `80235346-F6F1-3CCB-AA9C-55108A5F0AE5`. The user explicitly
approved erasing this spare. The Mac classifies this built-in reader as
internal; an external-only disk inventory does not list it.

Uncompressed the untouched vendor image (13,555,990,528 bytes), SHA-256
`681edfbb89349e32111c2cac8b48b551f5e5205d8387a81ce3e7c16bb01879ed`.
Inspected its MBR/FAT and ext4 boot/root read-only. The default is Fedora
6.1.55 and its root UUID matches the included filesystem. Built upstream
e2fsprogs 1.47.3 tools privately under the ignored recovery directory; source
SHA-256 `857e6ef800feaa2bb4578fbc810214be5d3c88b072ea53c5384733a965737329`
matches kernel.org's published digest. No system-wide package install.

Prepared a separate copy-on-write artifact, `PioneerLinuxRecovery-20261013.raw`.
Gave it MBR disk ID `201785d7` and set both extlinux and fstab to root
`PARTUUID=201785d7-03`, avoiding reliance on device enumeration or a factory
filesystem UUID potentially duplicated on another disk. Kept the stock kernel,
initramfs, DTBs and FAT boot firmware. Added the existing owner's public key
for root, a first-included SSH configuration requiring public-key authentication
and disabling password/interactive authentication and forwarding, and a fresh
private recovery host key (root:root, 600). Stock sshd service was already
enabled. Recovery host fingerprint is
`SHA256:5k2RifcAMqINUw7EbEJEHt4ND1fAfdhcWz0Lzh9qJMk`; its separate pin file
is `generated.pioneer/linux-recovery-20261013/known_hosts.recovery`.
The exact rollback file is included at `/root/pioneer-extlinux.conf.original`.
Private keys, disk images and preparation helpers remain ignored, not pushed.

All injected files were readback-compared against their inputs without exposing
private key material. Both modified ext4 partitions passed full read-only
`e2fsck -fn` (exit 0). Prepared image SHA-256 is
`7251f9ae2848761db1768d6f9bf82fbcd523a7bc823ee7879b8a93516acde121`.
The one-shot imager's dry run verified this digest and the spare's serial,
protocol, capacity, physical/removable flags and original volume UUID. Requested
macOS administrator authorization to execute the explicitly approved write;
actual write/readback/ejection results are still pending. The Pioneer remains
off, its NVMe rollback is still pending, and no SPI or Haiku-SD write is planned.

Administrator authorization was accepted, but macOS privacy policy prevented
the privileged Python process from opening the script under Documents. This
first attempt never reached the card. Staged identical script/image copies in
private directory `/private/tmp/pioneer-linux-sd.KhhSuW` (directory 700, custom
image 600), avoiding a request for broader Documents access. Script SHA-256 is
`21f22e6307e7e55fd0496cf40fb4da1db07a6761dd2c1428f2b3dd5df2acad8d`.

The staged retry verified the card identity and prepared-image SHA, rechecked
the original spare UUID, successfully unmounted disk6, and rechecked its
identity. It then stopped at opening `/dev/rdisk6` with `Operation not permitted`.
No raw descriptor was opened, no original-header backup exists, and no SD
sector was written. The card is unmounted, not erased or imaged. Log copied to
`generated.pioneer/linux-recovery-20261013/image-spare-temporary-stage.log`.
Do not retry alternate device aliases or weaken macOS privacy settings silently.
The user can explicitly run the unchanged guarded script in their own Terminal:
`sudo /usr/bin/python3 /private/tmp/pioneer-linux-sd.KhhSuW/image-spare.py --apply`.
It independently checks the original spare identity and image again before
writing and verifies the complete image readback before ejection. If Terminal
also reports a privacy block, stop and arrange explicit OS permission or an
authorized GUI media writer. Linux recovery has not booted; NVMe rollback is
still pending, and the Pioneer must remain off until media is ready.

The user ran the unchanged guarded imager in their own Terminal. It verified
the image/card, wrote all 13,555,990,528 bytes, and completed a full readback
(158 seconds), but the final SHA comparison failed: `SD readback SHA-256
mismatch; DO NOT BOOT`. The spare's original filesystem has therefore been
replaced; it was not ejected and is not yet approved as recovery media. The
original first-1-MiB header backup now exists in the private staging directory;
this is not a full backup of the erased filesystem. No Pioneer/NVMe/Haiku-SD
write occurred.

Read-only investigation identified the same spare serial 0xeb42 with the
expected new three-partition layout. macOS had automatically mounted its new
127.9 MB FAT boot volume and created `.Spotlight-V100` and `.fseventsd`, both
timestamped during imaging. Those directories do not exist in the source FAT
image. The imager closed its raw write descriptor before rechecking identity
and reading back, leaving a remount race. This is a likely explanation for
the whole-image mismatch, not yet proof that all payload data is intact or
that the media is healthy. Do not ignore the failed checksum or simply boot.

Unmounted disk6 successfully to stop indexing changes. Direct raw reads from
the tool process are permission-denied, so the user must capture a read-only
image from their authorized Terminal:
`sudo dd if=/dev/rdisk6 bs=4m count=3232 > /private/tmp/pioneer-linux-sd.KhhSuW/spare-readback.raw`.
This reads exactly 13,555,990,528 bytes and does not write the SD. Sufficient
temporary storage is available. Next compare all bytes outside the FAT
partition against the prepared image and verify every original FAT boot file
plus filesystem structure; only then decide whether the mismatch is benign
Mac metadata or corruption. Extracted the 12 original FAT files read-only
into ignored `generated.pioneer/linux-recovery-20261013/fat-original/` for
independent payload comparisons. The Pioneer remains off; NVMe rollback is
still pending. Never rerun the one-shot imager blindly or delete its guard
backup to force a second write.

### Recovery SD payload verification completed

The user captured exactly 13,555,990,528 bytes in 154.72 seconds with the
read-only dd command. Protected this captured image with mode 600. Its SHA-256
is `21a56241499397d010679e5c35159cce6b2c681172974eb539810a6ebc441513`.
An independent local audit compared all captured bytes with the pinned prepared
source and inspected the FAT filesystem through mtools, without opening a
device for writing. Results:

- Prefix/MBR, Linux boot partition, entire Linux root partition and trailing
  image bytes match **exactly**. No differences exist outside the FAT partition.
- All **12 original FAT boot files** match byte-for-byte by SHA-256, including
  ZSBL, FIP, OpenSBI, LinuxBoot kernel/initrd, DTBs and GRUB EFI loader.
- Every extra FAT directory/file is under `.Spotlight-V100` or `.fseventsd`;
  there are no missing original entries or unexplained extra boot files.
- Read-only `fsck_msdos -n` on the captured FAT partition and `e2fsck -fn` on
  both captured ext4 partitions all completed with exit 0, without repairs.

Thus the whole-image digest mismatch is explained by Mac filesystem/indexing
changes confined to FAT, not altered Linux or original boot payloads. This is
a passed **payload/filesystem** audit, not an assertion that the whole card
still has the prepared raw SHA or that it has undergone long-term media testing.
Detailed ignored report: `generated.pioneer/linux-recovery-20261013/spare-readback-audit.json`;
audit helper: `audit-spare-readback.py` in the same directory.

Rechecked the exact spare identity (serial 0xeb42, disk6, 31,299,993,600 bytes)
and successfully ejected it with diskutil. The recovery SD is now ready for
its first boot test; no second media write was needed. Asked the user to move
only this spare into the powered-off Pioneer and confirm insertion, leaving
the working Haiku SD separate. Do not power on before that physical confirmation.
Once recovery Linux is running, verify the expected host key, SD PARTUUID
`201785d7-03`, kernel/root/device identities, then restore the known NVMe boot
entry using the exact original backup with hash/metadata/sync checks. The
Pioneer remains off and that NVMe rollback is still pending.

### Original NVMe boot entry restored from recovery Linux

After the user confirmed insertion while powered off, the short power-on
pulse successfully booted the spare SD. Fresh serial baseline: 2233184.
Both LinuxBoot and Fedora use 6.1.55; final command line selects
`root=PARTUUID=201785d7-03`, and the running root is `/dev/mmcblk1p3` ext4.
The pre-pinned recovery SSH host key and existing owner's public key both
worked at 10.0.0.247. No serial password login or SSH trust bypass was needed.

Read-only disk inventory identified NVMe `FORESEE XP1000F001T`, exactly
1,024,209,543,168 bytes. Its 511,705,088-byte ext4 boot partition is
`/dev/nvme0n1p2`, UUID `d9518f6d-f175-4331-83de-cb10ef6d8885`, PARTUUID
`4d3e74a9-02`. Neither SATA disk was mounted in this inventory. The guarded
rollback helper verified all these identities and the recovery SD root, then
mounted the NVMe boot partition read-only with journal replay suppressed.
Candidate `97f7f308...`, 918 bytes, root:root mode 664, matched exactly.
The SD rollback copy was the expected original `53c0da6d...`, 878 bytes.
The full diff confirms only `keep_bootcon ignore_loglevel loglevel=8` are removed.

Read-only preflight passed, followed by the authorized `--apply` rollback.
Preserved the logging candidate on the recovery SD at
`/root/pioneer-nvme-rollback-20261013/extlinux.conf.logging-candidate`.
Restored via same-directory temporary file, SHA/ownership/mode checks, sync
and atomic rename; then unmounted and remounted read-only and verified again.
The NVMe entry is now exactly SHA-256
`53c0da6d26b93ac6e6a152c8146296325a9c25f54409ef7e6dcfc83e3b1fad9c`,
878 bytes, root:root mode 664. This **file rollback is complete**. No kernel,
initramfs, DTB, SPI firmware or Samsung content was replaced. The cause of the
logging boot stall is still not isolated. Detailed ignored artifacts include
`restore-nvme-boot-entry.sh`, `nvme-boot-restore-apply.log` and
`recovery-sd-boot1.log` under the recovery directory.

Requested clean shutdown only after checking the NVMe partition was unmounted.
Serial confirms root remounted read-only, all filesystems/devices detached and
`reboot: Power down`. The user physically confirmed fans/LEDs off and removed
the recovery SD. No extra forced-off pulse was necessary. Issued the short
power-on pulse for a normal SD-free NVMe cold-boot test at 00:27:44 UTC on
October 14 (October 13 local). Fresh serial baseline is 2403054.

The SD-free cold boot **passed** the original pinned Linux SSH check as milkv
at 10.0.0.247. Running kernel is 6.1.31, root is `/dev/nvme0n1p3` ext4,
`/boot` is `/dev/nvme0n1p2` ext4, all CPUs 0-63 are online and kexec_loaded
is zero. The live command line has none of the three experimental logging
options. `/boot/extlinux/extlinux.conf` again matches the complete original
SHA-256 `53c0da6d26b93ac6e6a152c8146296325a9c25f54409ef7e6dcfc83e3b1fad9c`,
878 bytes, uid/gid 0:0 and mode 664. The validation command completed with
`NVME COLD-BOOT VALIDATION PASS`. SPI LinuxBoot remains 6.1.47 and the normal
Fedora early UART cutoff at approximately 0.03 seconds is unchanged; SSH
runtime verification, not serial silence, establishes this successful boot.

**NVMe Linux recovery is complete and verified for this cold boot.** The board
is left powered on in its normal Linux installation, with SD removed. Keep
the spare Linux recovery SD and working Haiku SD separate; insert/remove them
only with board power off. No new kexec payload was loaded or executed during
this recovery. Hardware entry remains unproven, and the unattended SD-free
Haiku/Linux selector remains unimplemented. Do not reuse the one-shot Mac
imager or start another logging experiment without addressing its remount race
and preserving this now-working recovery path.

### Late-userspace kexec trace prepared (not yet installed/armed)

User approved resuming SD-free boot work. Rechecked the restored baseline:
kernel 6.1.31, NVMe root, CPUs 0-63, kexec_loaded zero and the original extlinux
SHA `53c0da6d...`. Root-visible journal history has no retained October 13
failed-handoff shutdown record; pstore directories are empty. Installed
systemd is 253.2. Its normal kexec target ends through systemd-shutdown and
the installed `/usr/sbin/kexec -e`. Inspected matching upstream v253 shutdown
source: system-shutdown hooks run after userspace/filesystem teardown and
before the final kexec path. Existing hooks are mdadm and fwupd. Read-only
SQLite inspection found `/var/lib/fwupd/pending.db` exists but its history
table has zero rows; its name alone must not be treated as a pending update.
`/proc/mdstat` lists no arrays. No activation command or firmware write ran.

Copied the installed public System.map and existing staged decompressed
kernel for local disassembly, not live kernel-memory access. Its handoff
code matches the older relocation/CPU shutdown structure; this does not
isolate the silent failure or justify a kernel patch. No boot-entry or
kernel replacement was made.

Implemented `tools/pioneer/kexec_trace/` plus native build and host test
scripts. The native userspace helper ignores normal poweroff/reboot/halt,
requires an explicitly created root-owned 0600 single-link arm file in /run,
checks riscv64/exact UART binding and the expected kexec_loaded state, and
emits distinct self-test/late-shutdown markers carrying a fresh token. It
only sets the verified native UART to 115200 raw/no-flow-control; it never
loads an image or reboots. Nonblocking output is limited to one second/512
calls, drain polling to one second, and a process alarm covers three seconds.
These are userspace bounds, not protection against a wedged kernel operation.
The volatile arm token disappears on cold reboot, leaving an installed
helper inactive even if cleanup has not yet been possible.

ASan/UBSan host tests passed for token policy, both marker forms, partial
writes, EINTR/EAGAIN, timeout/iteration/clock bounds and invalid write results.
All existing kexec probe tests and six Image-header test groups still pass.
Native build succeeded using existing GCC with explicit RV64GC/LP64D (no
RVV 1.0). Private binary:
`/mnt/ssd/pioneer-kexec-probe-20261013/shutdown-trace-1/build/PioneerKexecTrace`,
SHA-256 `5010270d91c420346a0a3cedf0ad6a6feb7dddce2ffbac9fdcb6e56a16b7b063`.
Native normal-action no-op and unarmed kexec/self-test guard checks passed;
none opened the UART. The first aggregate shell command stopped at an
over-conservative database-absence check **after** reporting guard tests pass;
subsequent read-only schema inspection established the history is empty.

The helper remains **uninstalled and unarmed**. No image has been loaded and
no reboot/CPU-hotplug operation has occurred in this resumed work. Requested
explicit saved-Linux-work approval for one controlled test with a temporary
late-shutdown hook, fresh visible self-test token, normal systemctl kexec,
bounded serial observation, relay recovery, complete baseline revalidation
and hash-checked hook cleanup. Approval is pending; do not treat the question's
preselected answer or these host/native guard tests as hardware authorization
or actual late-shutdown/probe success. The restored extlinux hash is unchanged.

### Late-shutdown trace observed; payload entry still unproven

The user explicitly approved installing the temporary trace and one controlled
kexec test, confirming Linux work was saved. This supersedes the pending
approval recorded above. Read-only native preflight passed on Linux 6.1.31:
NVMe root, no MMC device, CPUs 0-63, enabled/unoccupied classic kexec, exact
live UART layout and the unchanged original extlinux SHA. Relay status was
available without a power pulse. Neither the hook nor arm directory existed.

Installed the reviewed native binary as root:root 0755 at
`/usr/lib/systemd/system-shutdown/pioneer-kexec-trace`, with SHA-256
`5010270d91c420346a0a3cedf0ad6a6feb7dddce2ffbac9fdcb6e56a16b7b063`.
Created a root:root 0700 volatile directory and single-link 0600 33-byte arm
file. The self-test exited successfully with kexec_loaded still zero. From
fresh serial offset 2468404, the capture contains exactly its 108-byte marker
with token `8c4e8bfa910d189309d0aca26d51e1c4` and `stage=self_test action=none`.

Saved a fresh live FDT and compared it to `/sys/firmware/fdt` as root; SHA-256
is `e9264e1696ae23bdde4a2a20c0831215b4ea8967d2dd90ccba5a910aaa9befdd`.
An initial non-root cmp was permission-denied, not a data mismatch; the root
comparison passed. Local fdtget and native live properties confirm UART
0x7040000000, range 0x1000, shift 2, width 4 and the existing four 32 GiB RAM
nodes. No FDT edits were made. Reverified unchanged v2 Image SHA
`b57754d066aaaaa96ad285faff00a9efd30d5e4b24351b7d37f52f5d865f70e7`
and private loader SHA
`06aa5c6fa6e78bdf92abefb4e7f4b0accadfe6212a02c700905392f8d8056748`.
The guarded classic `image-riscv` load used memory limits 0x2200000 through
0xbfffffff, the fresh DTB and no debug flag. It succeeded with kexec_loaded=1;
the no-command-line warning is expected for this non-Linux entry probe.

At 02:16:57 UTC October 14 (October 13 local), requested normal
`sudo systemctl kexec`, retaining all 64 online CPUs. Fresh handoff offset
2468512 produced exactly one 113-byte serial record:

```
PIONEER_KEXEC_TRACE v1 stage=late_shutdown action=kexec token=8c4e8bfa910d189309d0aca26d51e1c4 kexec_loaded=1
```

No probe BEGIN, END or TRAP appeared by 02:18:08 UTC, over 70 seconds after
the shutdown request, and one bounded pinned-SSH attempt timed out. Thus
systemd's late userspace hook phase is observed. Completion of every hook,
the final kexec syscall/device/SMP/relocation path and payload execution are
**not** established. The remaining silence can still be a shutdown/handoff
failure or lost post-shutdown UART visibility; do not label it a proven
payload or CPU-shutdown bug. This is not a successful Haiku boot or selector.

Performed the approved seven-second relay off hold, ten-second wait, then
0.75-second on pulse (02:18:47 UTC). Fresh recovery serial offset 2468625
shows the unchanged SPI/LinuxBoot path and normal Fedora 6.1.31 early boot.
Pinned SSH baseline validation completed at 02:20:25 UTC: root
`/dev/nvme0n1p3` ext4, `/boot` `/dev/nvme0n1p2` ext4, CPUs 0-63,
kexec_loaded=0, no MMC device and preserved Samsung partition unmounted.
The command line is the original one without experimental logging flags.
Extlinux remains exactly SHA-256
`53c0da6d26b93ac6e6a152c8146296325a9c25f54409ef7e6dcfc83e3b1fad9c`,
878 bytes, root:root mode 664. Verified the installed hook's complete hash,
owner/mode and single-link status, then removed only that hook. The volatile
arm directory was already absent after cold boot. Cleanup checks passed.

Ignored evidence and guarded one-test scripts are under
`generated.pioneer/kexec-probe-20261013/shutdown-trace-1/`: `self-test.serial.log`,
`handoff.serial.log`, `load.log`, `kexec-request.log`, `live-fdt.dtb`,
`recovery.serial.log` and `recovery-validation.log`. Host ASan/UBSan trace,
probe format/flow/UART tests and all six Image-header test groups passed again.
The board is left powered on in normal NVMe Linux; both SD cards remain out.
No saved boot configuration, kernel, firmware or Haiku/Samsung content changed.
Further controlled hardware tests require a new reviewed plan/approval; first
investigate the remaining hook/final-handoff boundary without another reboot.

### UART runtime-power diagnosis and first observed payload entry PASS

The user requested continuation until the solution works or a physical SD
change is required, authorizing continued controlled tests/recovery. Read-only
checks found matching native Linux 6.1.31 headers, KPROBES/MODULE_UNLOAD and
fixed handoff symbols. Installed executor `/usr/sbin/kexec` remains SHA
`64b403d2c1c0f0d63581b902477a9945ba187c8b1ab18ae733c69a48dc3b0950`,
95,624 bytes, root:root 755. Dracut restore requires the volatile
`/run/initramfs/.need_shutdown` gate, absent now alongside its shutdown binary;
its saved initramfs contains a kexec-capable shutdown script. Runtime absence
does not prove which path an earlier shutdown took. Inspection ran no hooks,
restore/activation commands or executor.

Prepared a removable GPL kernel module with nine fixed function-entry kprobes,
default self-test-only mode, no image/reboot operations and no saved-register
edits. It never installs/autoloads itself. Matching headers supply symbol
versions/vermagic and RV64 IMAC flags, no FP/vector code. Native GCC is 13.2.1
versus the kernel's 13.1.1; build warned about this difference. Fixed an initial
module-local name collision with a kernel header. Host sanitizer tests pass
token policy, UART TX/drain iteration bounds, divisor-bank refusal and invalid
inputs. Actual armed handoff checkpoint testing remains undone.

The first native self-test returned EIO, leaving no module/probe loaded.
Added hit/register reporting and rebuilt; private module SHA is
`213953ff8e1600bcebbd0f65791d40dd8556e27124f4b3030e1b932b76b3deec`.
The actual breakpoint ran (`hits=1`) but transmission failed with UART
runtime_status=suspended, LCR=0x13 and LSR=0. A matching userspace marker had
passed immediately beforehand and then closed ttyS0. Holding that same tty
open made the kernel self-test pass with runtime_status=active, LSR=0x60.
Setting only `/sys/devices/platform/soc/7040000000.serial/power/control` to on
then passed without an open tty. Each successful self-test-only module was
unloaded; **no real handoff probes were armed**. Unsigned out-of-tree testing
tainted the running kernel as expected, with no signature/security-policy
change. The following cold boot cleared the taint.

This demonstrates a serial-output failure from UART runtime suspension;
baud configuration plus a marker followed by close does not keep it visible.
With only UART runtime policy on, loaded the exact unchanged v2 Image and
fresh pinned FDT using the existing private classic loader and memory bounds.
Outgoing CPUs remained 0-63 and the module was absent. Normal systemctl kexec
ran at 03:42:45 UTC October 14 (October 13 local), fresh serial offset 2532916.
The matching late marker (`9564c3fa8d41e9ae8340fd18542b7e2b`) is followed by:

```
PIONEER_KEXEC_PROBE v2 BEGIN
hart=0x0000000000000002
fdt=0x00000000bfff5000
satp=0x0000000000000000
sie=0x0000000000000000
image_start=0x0000000002200000
image_end=0x0000000002207000
fdt_header=OK (full tree not parsed)
sbi_harts_started=0x0000000000000001
sbi_harts_stopped=0x000000000000003f
PIONEER_KEXEC_PROBE v2 END status=PASS (entry/header/SBI state only)
```

This is the **first actual payload entry PASS**, validating this probe's
entry/header/SBI checks and SBI-reported hart states, not independent DMA,
cache, full-FDT validation or what happened in all earlier silent attempts.
It deliberately halted. Approved relay off seven seconds, wait ten, on 0.75
(03:44:12 UTC) returned normal pinned NVMe Linux. Baseline validation and
owned-hook removal passed at 03:46:33 UTC: kernel 6.1.31, expected root/boot,
CPUs 0-63, kexec_loaded=0, original extlinux SHA/metadata, no MMC, Samsung
unmounted, volatile arm absent. UART reset to auto, no module loaded, taint=0
and installed executor unchanged. No boot, firmware or Haiku media edits.

Added `pioneer_linux_uart_keepalive.sh status|enable|restore`. It claims only
the recorded UART's original auto policy with an empty kexec slot, records
root-owned 0600 state in /run, verifies on/active and restores/removes only
owned state. Native enable/restore passed auto/suspended -> on/active ->
auto/suspended. No persistent boot/service change is added. Optional module
source/build/README and host tests are saved; armed checkpoints were not
needed for the successful unchanged probe. Existing trace/probe sanitizer
tests and all six Image-header groups also pass.

Ignored evidence is in `generated.pioneer/kexec-probe-20261013/kernel-trace-1/`,
notably `self-test-1.serial.log`, `held-tty-self-test.serial.log`,
`self-tests.serial.log`, `untraced-handoff.serial.log` and `recovery-1.serial.log`.

### Full existing Linux-to-Linux handoff passed, without relay recovery

Used the guarded UART helper and a fresh visible self-test marker with token
`0257c485981a5ed2655f7cbf79cec15f`. Reverified the existing decompressed Linux
Image SHA `b77ee6f003e0ffc74444880b93d9a9d903bad550ef9ca5b0b232b07a7a78fbef`,
existing initramfs SHA `2abfdf6268cdaa9beab1297525d6812c821e27bc72677e80fb5cf6db8c7c5747`,
source compressed kernel/initramfs, fresh live FDT, loader, hook and restored
boot-entry identities. Accepted load used the same classic Image loader and
memory bounds plus the **exact original running command line**. No previous
keep_bootcon/ignore_loglevel/loglevel options were appended. No module loaded.

Requested normal systemctl kexec at 03:49:35 UTC, fresh serial offset 2598200.
Capture contains its late marker and directly the new Linux 6.1.31 boot, with
no intervening ZSBL/OpenSBI/LinuxBoot cold-boot banner. Early UART cut-off at
tty1 registration is normal for these unchanged arguments. Pinned SSH returned
with boot ID `2343aa26-2b05-4216-9a61-62b7d4b5b160`, different from outgoing
`78a9d5aa-73f7-4e7d-a020-912c9d03efdb`. Kernel/root/boot and all CPUs are correct.
No relay pulse was used for this control. Thus a **full Linux handoff** is now
observed, not just the tiny entry probe.

Complete baseline validation and hash-checked temporary-hook removal passed
at 03:52:40 UTC: kexec_loaded=0, CPUs 0-63, no MMC, Samsung unmounted, original
command line and original 878-byte/root:root/664 extlinux SHA unchanged. UART
policy is auto, owned keepalive/arm state absent, no module loaded, taint=0
and installed executor unchanged. Ignored control evidence/scripts are in
`generated.pioneer/kexec-probe-20261013/linux-control-uart-on-1/`, including
`self-test.serial.log`, `load.log`, `handoff.serial.log` and
`validation-cleanup.log`. Linux remains powered on and reachable with both SD
cards out. Haiku RAM-resource loading, root identity and unattended selector
are still unimplemented; do not call this a Haiku boot.

### Haiku non-EFI kexec RAM-header/root smoke passed

Found the existing complete 4 GiB SD filesystem snapshot locally; SHA-256
again matches `987165a790a12170c75554a9a2833e4c8c14b9d8a698559c9e3932fd94940d0f`.
Restricted the local raw snapshot to mode 600. No SD change was required.
Transferred a compressed stream into a new private R3SL staging directory,
expanded it to exactly 4,294,967,296 bytes and verified the complete native
disk hash. The native copy is milkv:milkv 600 beneath a 700 directory. Raw
filesystem copies may contain private account data and must never be pushed.
An initial staging mkdir without sudo was permission-denied, leaving no
destination; created only the exact new project directory with sudo/chown
before retrying. No existing media/image was overwritten.

Added an explicit `HAIKU_PIONEER_KEXEC_PROBE` build profile for the existing
non-EFI loader. Its 64-byte Linux Image header enters assembly startup,
captures incoming CSRs, creates a private stack/trap handler and enables the
FPU before C++ code. The smoke path validates bounded physical FDT memory,
full FDT structure, board and exact UART layout before output. It requires
the explicit chosen property value 1, validates 32/64-bit initrd bounds, then
checks BFS superblock geometry and root inode bounds/magic/type. It never
uses emulator HTIF, heap/constructors, PCI/storage drivers or starts Haiku.
Output is polled/bounded and preserves the prepared UART settings.

The shared non-EFI linker entry now names assembly `_riscv_start` rather than
the C `_start`, and small BSS/common data are included in the clear range.
Renamed the input stack section to `.stack` so wildcard BSS clearing cannot
erase the active stack; it remains outside `_end`. Generic builds have no
Linux header or smoke entry unless explicitly selected. EFI source paths and
deployed loaders are unchanged. Generic compilation, not runtime emulator
behavior, was checked after these startup changes.

Host ASan/UBSan bounds tests passed. Testing against the real snapshot caught
an incorrect assumption before hardware: BFS blocks_per_ag counts **bitmap**
blocks, not data blocks. Corrected it against production Volume.cpp and added
the group-capacity/count checks. Real-image verification now passes with
block size 2048 and root byte offset 1,073,741,824. Existing probe tests and
all seven Image-header groups pass; the header verifier retains its default
128 KiB cap and accepts a larger smoke image only with an explicit size cap.

Existing source hashes matched the mirror before syncing only the affected
files. Build first stopped for missing mirror Git revision, then succeeded
using the existing hrev99999 local-development convention/cross-tool PATH.
`pioneer_kexec_loader_build.sh` verifies mirror equality, forces the two
profile-dependent sources, saves/pads the private Image, checks fixed base,
unresolved symbols and retained relocations, then restores generic build
objects with the macro off and normal base 0x1000000, including on failure.
Both private-build and generic-profile rebuild passed; the latter has
`_riscv_start=0x1000000` and no `pioneer_kexec_start` symbol. Existing package
notices and RWX loader-segment warning remain. No package or kernel was built.

Tested private Image: 1,413,120 bytes, SHA-256
`69d2f889c9a1d28913c5b0d0c73f148aa63f94687eed2d0e6399ce0d72aaca90`.
After the Linux control boot, fresh live FDT has SHA
`a447ce7127fa4c626a5a0f63a423e6ba303657b8751dcfa795e2bc65f472a94c`;
it legitimately differs from the earlier cold-boot FDT because its initrd and
kexec chosen properties changed. Verified a complete textual diff of a
private test copy: only `haiku,pioneer-kexec-probe = <1>` is added. That input
copy SHA is `d34acb35d9a3073ae0e0aa9eaab0cee3a9b740db34bc9d62d419d4782b73922b`.
No memory, CPU, PCI or UART node changed. The loader subsequently supplies the
actual new initrd/FDT placement. Native gates and complete snapshot rehash
passed before arming; fresh visible marker token is
`fc7be564428ff7626804afb577686ccf`. Used the guarded UART on policy, no module,
all 64 CPUs and exact private loader. Root-visible first System RAM begins
0x2200000, matching this fixed-linked Image. Accepted load allowed RAM through
0x1fffffffff to hold the complete 4 GiB initrd without using the 3..4 GiB hole.

Normal systemctl kexec at 04:22:27 UTC October 14 (October 13 local), fresh
serial offset 2603770, produced the matching late marker and:

```
HAIKU_KEXEC_LOADER v1 BEGIN (RAM-resource smoke only)
hart=0x0000000000000003
incoming_satp=0x0000000000000000
incoming_sstatus=0x0000000200000120
incoming_sie=0x0000000000000000
fdt=0x0000001effff5000
initrd_start=0x0000001f00000000
initrd_size=0x0000000100000000
bfs_block_size=0x0000000000000800
bfs_num_blocks=0x0000000000200000
bfs_root_offset=0x0000000040000000
bfs_root_inode=OK
HAIKU_KEXEC_LOADER v1 END status=PASS (RAM header/root only; kernel not started)
```

The initrd starts at 124 GiB and the root-inode read is at 125 GiB. This
establishes these 64-bit RAM reads and metadata checks, not a full-memory
audit, RAM filesystem mount, kernel/module preload or Haiku boot. It halted
as designed. Approved relay off/wait/on (on at 04:23:42 UTC) restored default
NVMe Linux. Complete baseline/owned-hook cleanup passed at 04:27:11 UTC:
kernel/root/boot, CPUs 0-63, kexec_loaded=0, no MMC, Samsung unmounted and the
exact original extlinux hash/metadata. UART auto policy, absent volatile
keepalive/arm state, no module/taint and executor hash are separately checked.

Private build/test evidence is under
`generated.pioneer/haiku-kexec-loader-20261013/smoke-1/`; native snapshot and
test inputs are `/mnt/ssd/pioneer-haiku-kexec-20261013/smoke-1/`. None of the
raw filesystem or private image data is committed. Next is a separate
RAM-only filesystem/kernel-resource preload test, still stopping before
kernel execution and without touching the original SD/Samsung/boot settings.

### RAM filesystem, kernel and 54 boot images preloaded successfully

Extended the explicit probe profile with chosen mode 2. Modes 1 and 2 alone
are accepted, and both halt before kernel execution. After the early bounded
FDT/RAM/BFS checks, mode 2 clears BSS while preserving the separate active
stack, records the validated RAM disk, initializes constructors/FPU/heap and
uses the existing stage2 filesystem/package/kernel readers. A low allocation
pool ends before either a low FDT/initrd or 0xc0000000, with a 64 MiB minimum
margin beyond the loader stack. This is a boot allocator, not a reported
restriction on the machine's usable RAM. No kernel memory map is built yet.

Only this explicit profile routes loader debug/console to bounded UART
output, disables emulator fw-cfg/virtio/video hardware initialization, and
adds a read-only MemoryDisk instead of VirtIO disk probing. It retains the
already-validated FDT without platform hardware enumeration. Unexpected menus
halt with failure instead of awaiting unavailable keyboard input. The
platform_start_kernel guard validates a RISC-V ELF64 kernel and a bounded,
nonempty boot-image list, prints their summary, then halts before the normal
MMU/SMP/kernel path. It cannot execute the loaded kernel.

New host sanitizer fixture extracts that exact production guard, stubs the
stop operation and tests inactive behavior plus missing/wrong-class/wrong-
architecture kernels, empty image lists, successful preload and cyclic lists.
Every active case stops and none reaches the mocked normal kernel path.
Mode rejection/pool bounds and the real BFS snapshot tests also pass. All
existing UART/trace/probe tests and seven Image-header groups pass again.

The first generic-profile restore during this extension reported a no-op:
Jam compares whole-second timestamps and the source touch could occur in the
same second as freshly compiled objects. Fixed this before hardware by
waiting one second before each profile-dependent touch, forcing all seven
affected sources and verifying the generic ELF base/no-probe-symbol result.
The corrected private build and generic restore both rebuilt 12 targets and
passed. Shared output is normal base 0x1000000, no probe entry. Existing
package/architecture notices and RWX segment warning remain; no kernel or
package target was built. New source/mirror copies were equality-checked
against the previous committed versions before scoped synchronization.

Tested preload Image is 1,417,216 bytes, SHA-256
`b9146f111413e3dbd1967c638659309e807d97308cd0ec5ded32dde8caadfe37`.
Its fresh cold-boot FDT again matches `e9264e1696ae23bdde4a2a20c0831215b4ea8967d2dd90ccba5a910aaa9befdd`.
Complete private-copy diff adds only `haiku,pioneer-kexec-probe = <2>`;
input FDT SHA is `2db3daed29f04d3aa87bdde73275b3a215010d081eddc6588907e77dfeca297f`.
Rehashed the unchanged private 4 GiB snapshot completely before arming.
Normal native gates, fresh visible UART marker/token
`6bb62b1cb5fb4ac15a4b3b12177db321`, runtime on/active policy and accepted
load-only all passed. CPUs stayed 0-63, with no tracing module.

Normal systemctl kexec at 04:42:21 UTC October 14 (October 13 local), fresh
serial offset 2668506, printed the matching late marker, mode-2 entry and
successful high-RAM BFS header/root checks. Then the real Haiku boot loader
mounted the 4 GiB RAM device and reported:

```
load kernel kernel_riscv64...
Pioneer preload: kernel entry 0xffffffc00010a180
Pioneer preload: 54 preloaded boot images
HAIKU_KEXEC_LOADER v2 END status=PASS reason=kernel_and_boot_images_preloaded (kernel not started)
```

The activated-packages-file warning is a handled current-state fallback:
production PackageVolumeInfo::_InitState scans for the system HPKG when that
administrative file is absent. It did not stop preload. The private loader's
revision string is blank because this helper uses the unrevisioned ELF, not
a claim about the loaded kernel's revision. No physical disk was mounted by Haiku,
no hardware storage/graphics probing ran, and no kernel entry is established.

Approved relay off/wait/on (on at 04:49:25 UTC, recovery serial offset 2669977)
returned default Linux. Complete baseline and owned-hook cleanup passed at
04:51:22 UTC: kernel/root/boot, CPUs 0-63, empty kexec slot, original extlinux
hash/878-byte/root:root/664 metadata, no MMC and Samsung unmounted. UART auto,
no owned keepalive/arm state, no trace module, taint=0 and unchanged installed
executor SHA all passed. An approval-service stream disconnection prevented
one validation call from executing; the same-scope retry used the normal
approval path and completed, without bypassing authorization.

Ignored private evidence/scripts: `generated.pioneer/haiku-kexec-loader-20261013/preload-1/`;
native staged test inputs: `/mnt/ssd/pioneer-haiku-kexec-20261013/preload-1/`.
The shared snapshot is reused read-only from the previous private smoke
directory. Next is root-identity and non-EFI hart/MMU handoff review before
any real kernel execution. No firmware, saved boot setting, Haiku media or
Samsung content was replaced. Linux remains powered on and reachable.

Read-only root inventory identifies Samsung serial S14CNSAD300075H, whole
capacity 120,034,123,776 bytes, partition 1 start sector 64, size
120,033,640,448, BFS UUID 020147de08752551 and PARTUUID d89b0970-01. Mounted
only with Linux befs ro,nosuid,nodev,noexec to inspect the system-package hash,
then unmounted successfully. Samsung's package is
`haiku-r1~beta6_hrev99999-1-riscv64.hpkg`, SHA
`6a40c785862ab19913a7cf0b9ef64c90fd5998cda2c22c6636edc8f3463b348a`.
It differs from the saved SD package `c7e0d87f...` and current SD xHCI candidate
`edffc524...`; do not assume matching kernels/userspace merely because their
package version names match. Further read-only compatibility/input selection
is required before choosing a real root; no root package update is authorized
or needed by the completed preload experiment.

## October 13 local / October 14 UTC: native handoff preparation passes

Copied Samsung's exact system HPKG through a transient Linux befs read-only
mount into private R3SL staging, verified SHA-256, and unmounted the source.
Only its public Ed25519 host key was inspected (fingerprint matches the
existing pinned Haiku known_hosts file). An automatic safety review rejected
copying the OS package to Mac staging; no transfer workaround was used.
All bundle construction instead stayed on the Pioneer. No installed package
or user data was replaced.

The native bfs_shell requires privileged access for its host-file copy path:
the first unprivileged new-artifact population returned permission denied.
A fresh root-run 300 MiB regular staging image then passed all seven command
replies, checkfs, and roundtrip package SHA. Image SHA is
`3d909f2d0de60528496d1ee040a918a7bf6f1f65d6ddc16d9904c3a022e335c1`.
No physical filesystem was initialized. Samsung source package SHA remains
`6a40c785862ab19913a7cf0b9ef64c90fd5998cda2c22c6636edc8f3463b348a`.

Read five identity sectors from unmounted Samsung: offsets/sums
0/83a217d7, 512/b9226b3b, 32768/0, 33280/3a7c012c,
120034123264/0. Private chosen manifest version 1 describes whole-disk bytes
120034123776 and partition byte offset 32768, not RAM-image identity.
Full FDT diff adds only the manifest and explicit probe mode 3. Input DTB SHA
`2e37cc3cc573706874918a5016efcfecac1508c86bfd023b1bb79be4b391263e`;
unchanged live DTB SHA `e9264e1696ae23bdde4a2a20c0831215b4ea8967d2dd90ccba5a910aaa9befdd`.

First prepare-only Image (`fe5b5324daabf41a1e1f5df4e7fde72293e5de6e008f80a8ac76784749e3f55d`)
reached loader entry after normal shutdown but trapped with scause 7,
PC 0x220fc04, stval 0. Production disassembly identifies an unconditional
store through KMessageField::ElementAt's optional size-output pointer.
SetInt64 updating the already-registered partition offset exposed this bug;
it was not evidence of an invalid Samsung kernel. Added null checks for both
fixed and variable element sizes plus an ASan/UBSan production-helper test.
The isolated fix is committed/pushed as 339e16e4f5. Relay recovery and complete
baseline/owned-hook cleanup passed at 05:15:44 UTC, all 64 Linux CPUs online.

Corrected prepare-only Image SHA
`2b08699441caba97d0233a3bb1bd86a77e31aefea76bfd5a93c7d383a7d098f8`
(1,417,216 bytes) passed normal load/shutdown using fresh marker token
641f5055a23cfccc86905abd206849c5, serial offset 2801084, 05:16:53 UTC.
It read the 300 MiB BFS at 0x1fed400000 and FDT at 0x1fed3f5000, loaded
Samsung's ELF64 kernel entry 0xffffffc00010a018 plus 54 boot images, and
registered the real root manifest. Actual hart 3 resolved to PLIC context 7;
timebase 50 MHz, UART IRQ 112, T-Head MAEE enabled. It generated low-window
Sv39 tables (SATP 0x8000000000003b32), kernel stack 0xffffffc000746000,
RAM 0x2200000..0xc0000000, then printed:

```
HAIKU_KEXEC_LOADER v2 END status=PASS reason=kernel_handoff_prepared_SATP_not_switched (kernel not started)
```

No kernel executed in that successful trial. Relay off/wait/on recovery is
under way for the explicitly gated mode-4 candidate. The candidate adds HSM
state and critical-map validation, a local fence.i, canonical post-SATP UART
diagnostics, and a separate mode-4-only entry path. Production-body host tests
prove mode 3 halts, mode 4 enters only after all 64 HSM checks/map validation,
and bad HSM state halts without entry. Generic build restoration remains
mandatory. Both SD cards remain out, default Linux boot settings unchanged.

Recovery from the prepare-only pass completed with full baseline and exact
owned-hook cleanup at 05:21:37 UTC. The mode-4 Image is 1,421,312 bytes, SHA
`acdb1dcad872528e1a0e36bcbc1e950453aca103c5f0a4b1b8707c1f134fbdbe`;
mode-4 input DTB SHA `a2ecad2ef54300044054b62f5bce09c2eea944b9facc15dad517c026f3369349`.
Re-read all five root identity sectors before staging; unchanged. Fresh UART
marker token 6bb5ede21005377fad1f9c75aca44898 was visible, all input/baseline
checks passed, normal load succeeded and normal shutdown ran at 05:23:34 UTC,
serial offset 2872812. The incoming hart was 2 / PLIC context 5. All 64 HSM
state checks and critical mapping checks passed, then the loader switched
SATP and entered Samsung's actual Haiku kernel.

The kernel printed `_start`, CPU/per-CPU/platform/debug readiness, heap,
VM, low-resource, module and semaphore readiness. It then panicked at
arch_vm_init_end while registering the PLIC area at
0xffffffc00074b000..0xffffffc00474b000, before mounting any root filesystem.
This establishes real SD-free kernel execution, not yet a Haiku boot or
desktop. The kernel debugger responded to a read-only `help` request with
unknown-command (its full debugger command set is not initialized here).
Captured panic/stack trace privately and began relay fallback recovery.

Source audit: this kernel deliberately defers generic boot-range VM
reservations; sem_table uses B_ANY_KERNEL_ADDRESS before arch_vm_init_end
registers loader MMIO as areas. The native allocator packed the PLIC directly
after the boot stack, leaving it exposed to that early area allocation.
The next loader-only candidate moves device mappings to a separate window
at KERNEL_BASE+1 GiB with an overlap gate, leaving space for initial kernel
areas below it. This is a hypothesis-driven retry; no Samsung kernel or
package replacement is involved.

## October 13 local / October 14 UTC: SD-free Samsung boot reaches desktop services

The dedicated-MMIO-window candidate (same kernel/package, loader-only change)
is 1,421,312 bytes, SHA
`6188d850ba4fda0851031dd9213d2cd1e760276eddfa801a577d73ddc6850961`.
The previous kernel-entry failure recovered into the complete Linux baseline
and owned-hook cleanup at 05:29:56 UTC. Re-read Samsung identity sectors;
unchanged. Fresh marker add3a96ff84e0aa74dcd3702e4f6976f, verified image/FDT/
bundle, visible self-test, UART on/active and normal load/shutdown passed.
Execution began 05:31:06 UTC, fresh serial offset 2945223.

This time the kernel passed VM/area setup, identified Samsung with the exact
whole-disk size and all five checksum matches, selected partition byte offset
32768, and mounted BFS root `/dev/disk/scsi/0/3/0/0`. It loaded/resumed
launch_daemon and started net_server, debug_server, app_server, input_server,
Tracker, Deskbar and notification_server. Radeon initialized a 1600-pixel
framebuffer. There was no panic through the observed userspace run. Both SD
cards remained physically out throughout. This establishes SD-free Samsung
root/userspace startup; physical desktop/input confirmation remains separate.

The working initial adapter is still intentionally one CPU and the low RAM
window, not a full-memory/SMP validation. Remote access is unresolved: NICs
enumerated, active RTL8125 MAC ending ce reported 1 Gb/s link, and a DHCP
negotiator started, but 10.0.0.247 ICMP/pinned SSH timed out. The source-derived
IPv4 fallback 169.254.0.10 did not resolve via ARP. IPv6 link-local also did
not answer; source inspection then confirmed default IPv6 autoconfiguration
is disabled in this net_server. No LAN-wide scan or SSH trust bypass ran.
Mac packet-header capture was unavailable because sudo requires a password
and unprivileged BPF access is denied; no permission was weakened.

Captured the full private serial log, left the package unmodified, and began
relay fallback recovery at 05:39 UTC to inspect only the saved network/SSH
configuration through Linux befs read-only. Default boot settings, SPI,
recovery media and SD contents are unchanged. A normal Haiku boot can write
its own logs/settings; the relay recovery was not a claimed graceful shutdown
and future source inspection must account for possible pending BFS journal
state. No disk repair or offline write is authorized by that read-only audit.

Linux recovery returned all 64 CPUs, NVMe root/boot, empty kexec slot and
unchanged extlinux hash/metadata; the exact temporary hook was removed.
The first post-Haiku Linux clock reading briefly showed May 2023, so local
Mac UTC timestamps remain the reliable trial ordering evidence.

Automatic safety review rejected the planned readout of Samsung network/SSH
configuration, selected account metadata and filtered service logs. That
script was not executed and no alternate content-export path was used.
A separate lower-risk existence-only script was approved and completed:
system/settings/launch/pioneer-ssh, home/config/settings/ssh/start-sshd.sh,
home/config/settings/ssh/authorized_keys and system/var/log/syslog are regular
files; system/home wired interfaces files were absent or symlinks, as was
the probed system/settings/passwd path. No conclusions about account validity
or exact configuration contents follow from those existence checks.
The transient mount was explicitly read-only and successfully unmounted.

Next required authorization is the narrowly scoped configuration/filtered-log
inspection (no private keys, password hashes or general personal logs), not
an SD-card swap. No offline repair or source-disk write has been performed.
Linux is the reachable fallback while that permission is requested. The
successful SD-free kernel/root/desktop-service startup is retained in source,
private test artifacts and this record; remote-unattended operation is not
claimed complete.

## October 14: authorized service audit and reversible SSH diagnostics

The user explicitly approved displaying wired-network settings, non-secret
SSH startup commands, selected service-account presence and filtered DHCP/SSH
errors. The scoped read-only audit found the existing launcher intact and
the nonzero-UID sshd account present in `system/settings/etc/passwd` (not the
previously guessed `system/settings/passwd`). No private keys or password
fields were displayed. Historical syslog entries include successful DHCP
leases for the expected address, but lack timestamps and cannot establish
network success during the new kexec boot.

Replayed the journal and ran check-only checkfs on a private qcow2/NBD view
while the Samsung backing partition was block-read-only. No newer log data
appeared. Checkfs reported zero missing/duplicate allocations but 16,987
blocks that could be freed; this is not a claim of a pristine filesystem.
No repairs ran. The source was unmounted, original kernel write policy
restored, and temporary NBD attachment/module removed.

Release launch_daemon redirects job stdout/stderr to `/dev/null`. Added
opt-in serial checkpoints/configuration testing to the tracked SSH launcher,
plus a launch job with the registrar dependency. The explicit key-only SSH
options remain unchanged. Regression tests exercise existing/missing accounts,
UID-zero refusal, configuration and permission failures, default silent mode,
and bounded interface reporting. Existing handoff/KMessage/probe tests pass.

Rehearsed replacement of only the startup script and launch job on a fresh
copy-on-write view, verified readback, then installed those exact two files
on the identity-checked Samsung partition. Backups of both original regular
files are retained on the build disk under
`/mnt/ssd/pioneer-haiku-kexec-20261013/ssh-diag-1/installed/`.
The OS package hash remained `6a40c785...b348a`; keys and account databases
were not changed. Check-only allocation counts remained unchanged.

At 16:58 UTC prepared and started mode-4 trial `enter-3` with the same tested
loader and Samsung RAM bundle, a freshly read root identity manifest, FDT SHA
`4037ce8ed6f6c268d3a0e29094ddd1106040b4f7a800ed148071f037acfa24b1`,
and fresh marker `de0c2237582946ccacd21ff5131b5622`.
Visible self-test and normal late-shutdown markers passed. Serial capture
starts at byte 3350762. This trial is for live service diagnosis, not a new
cache/DMA fix or a claim of unattended remote operation.

Trial `enter-3` passed root identity/mount again. The new SSH diagnostics
passed account, permission and `sshd -t` checks; sshd explicitly reported
listening on IPv4/IPv6 port 22. The active wired port (MAC ending ce) reached
1 Gb/s link, but successive snapshots showed no IPv4 address, zero received
packets and five submitted transmit packets. Thus SSH startup is no longer
the unobserved variable; receive/DMA/interrupt behavior needs investigation.
Recovered Linux and removed the exact temporary shutdown hook; all baseline
checks passed. No SD insertion was needed.

Built an opt-in `HAIKU_PIONEER_RTL_DIAGNOSTICS=1` driver profile. It reports
16 initial ticks of ISR/IMR, interrupt count and ring descriptor metadata,
without packet contents, register writes or polling completions. Host tests
execute the production tick function under ASan/UBSan, checking bounded
output, null/index guards and diagnostics-off behavior. Cross build passed;
shared outputs were rebuilt with diagnostics off and verified to contain no
diagnostic marker. Private diagnostic driver SHA
`1bb1fc8f063651c8373b063133bd48f368f17129107d4c6e9c13e7d1995a5816`,
246,763 bytes. Native inspection confirmed zero unresolved strong imports
against Samsung's actual kernel (SHA `38d4a424...49a65b`). Native package
extraction reported unsupported Linux extended attributes after file data
extraction; these are private inspection files, not installation artifacts.

Rehearsed then installed one previously absent regular-file override at
`system/non-packaged/add-ons/kernel/drivers/dev/net/rtl8125`. The package and
its original driver remain unchanged. Check-only BFS allocation warnings
remain 16,987 freeable blocks, zero missing/duplicate blocks. The override
can be removed by its exact hash/path after recovery; no other override was
created. A fresh manifest and marker are required for its boot trial.

Trial `enter-4` (17:08:53 UTC, serial offset 3776564, marker
`9a2a2c1fa4844e528b08df8d7c82b8d4`) loaded the system non-packaged driver
but then initialized the packaged driver, yielding no diagnostic tick lines.
This is not valid evidence about its DMA/interrupt counters. Source inspection
found `legacy_drivers.cpp::get_priority()` returning on the general system
directory prefix before considering the nested non-packaged directory; both
therefore receive priority zero and the later packaged entry replaces the
diagnostic path. Fixed the production function to retain the more-specific
match and check directory-component boundaries. ASan/UBSan tests exercise
the actual function with packaged, nested non-packaged, user and lookalike
paths; the normal kernel cross build passed. This fix is not yet deployed
to Samsung's unchanged OS package, so the current diagnostic trial must use
the existing higher-priority user path instead, without any boot-preload link.

Moved the exact diagnostic override to
`home/config/non-packaged/add-ons/kernel/drivers/dev/net/rtl8125`; the former
system override path is absent. Trial `enter-5` at 17:14:30 UTC (serial byte
4208522, marker `98b2d369625e461bb7a0376da82634cf`) emitted the real diagnostic
ticks. NIC status changed from 0x4010 to 0x40b5, RX descriptors completed and
transmit producer advanced while the consumer stayed zero. All 16 ticks
reported zero driver interrupt calls. This narrows the failure to interrupt
delivery/service rather than simply assuming corrupt DHCP or missing SSH.
The first diagnostic's `listseg.ds_addr` fields were compatibility-layer
virtual addresses, not the hardware ring addresses; corrected subsequent
diagnostics to report `list_map->dm_segs[0].ds_addr` with null-map guards.

Prepared an explicit, default-off `HAIKU_PIONEER_RTL_MSI_ONLY=1` comparison
profile. It skips MSI-X and fails closed if conventional MSI cannot be
allocated; normal builds retain existing MSI-X-to-MSI fallback. Production
selection code and tick diagnostics pass ASan/UBSan host tests. Cross build
passed; shared output was rebuilt normally and contains no test marker.
Private MSI-only driver SHA
`fe6273c985de229e0c6d8fba9dcc27d7978cb9ab2b419670058ceb0198a13c00`,
250,547 bytes. Replaced only the exact prior diagnostic override, retained
its backup, verified readback and unchanged OS package. This is a controlled
comparison, not yet a proven MSI-X workaround.

Trial `enter-6` at 17:19:57 UTC (serial byte 4636021, marker
`0f6a175a186e49deaeb240ebc84e492c`) passed: MSI-only profile markers appeared,
driver interrupt counts advanced, DHCP assigned 10.0.0.247 and pinned-key SSH
as baron succeeded. The real root is Samsung BFS `/dev/disk/scsi/0/3/0/0`.
The loaded driver is the expected user non-packaged override; SHA matches
`fe6273c9...13c00`, OS package remains `6a40c785...b348a`. SSH inspection
confirmed net_server, Tracker and Deskbar, with bidirectional packet counters
and zero reported network errors. Both SD cards remain out. This establishes
an MSI workaround for the observed MSI-X interrupt failure; it does not prove
the underlying MSI-X hardware/bridge cause. No polling completion workaround
or cache-management experiment was needed.

The RAM handoff still intentionally exposes one CPU and 3,185,573,888 usable
bytes, not all 64 CPUs/128 GiB. Added `pioneer_linux_boot_haiku.sh` for the
recorded native artifacts: separate prepare/boot/cancel actions, fresh strict
root-identity manifest, exact image/bundle/driver/package hashes, SD absence,
root/boot/CPU/default-config gates, temporary owned UART policy and normal
systemd kexec shutdown. It never edits extlinux/SPI, writes Samsung, or moves
an SD card. Preparing stages an image while Linux remains running; cancel
unloads it and restores the UART policy. Private run directories retain
evidence; session names must be fresh and simple. Source checksum tests reject
short sectors and validate all 128 little-endian words (including repeated
rows via `od -v`). Native round-trip verification follows.

The full SD-free round trip passed. Requested normal Haiku shutdown via
pinned SSH (`sync; shutdown -s`), observed service termination/SSH closure,
allowed it to settle, then used the existing off/wait/on relay recovery.
No claim is made that this board physically powered itself off or rebooted.
BFS emitted existing index-lookup warnings during shutdown; no filesystem
repair was attempted. Default Linux returned all 64 CPUs, original NVMe
root/boot, unchanged extlinux hash/metadata and an empty kexec slot. Removed
only the exact old temporary diagnostic hook.

The tracked helper's native `prepare cancel-check-20261014` loaded the image
while Linux stayed running. Its `cancel` verified input hashes/root manifest,
unloaded the image and restored UART auto policy; independent checks confirmed
empty kexec, absent owned UART state and unmounted Samsung. Then prepared and
booted `roundtrip-20261014` through the same helper at approximately 17:30 UTC,
fresh serial byte 5122103, without installing another shutdown hook. Pinned
Haiku SSH returned again at 17:31 UTC. Samsung BFS root, MSI-only driver hash,
unchanged system package and bidirectional error-free wired packet counters
were verified again. Both SD cards remained out throughout.

Final board state: reachable Haiku, using the tested user MSI-only override;
no request for physical intervention. Normal Linux remains the cold-boot
fallback. Known limits: this RAM loader is single-CPU/low-memory, physical
desktop/input have not been independently confirmed for these new boots,
MSI-X delivery root cause is unresolved, and the driver-priority source fix
is cross-built but not installed into Samsung's package. Host probe, preload,
handoff, KMessage, xHCI cleanup, SSH, driver-priority, RTL selection/diagnostics
and boot-helper tests all pass. Shared cross-build outputs are normal profiles,
not the opt-in diagnostic driver. Unrelated llama.cpp submodule edits remain
untouched.

## October 14: SD-free SMP loader, two-core tests and remaining runtime gate

The user requested restoring all CPUs before P100A inference. Added a
default-off `/chosen/haiku,pioneer-cpu-count` property (power-of-two counts
through 64) to the private non-EFI loader. Without it, the tested single-hart
path remains unchanged. The selected set puts the actual Linux entry hart
first, not an assumed hart 0. It requires complete/unique FDT records for all
64 physical harts and their supervisor PLIC contexts, then allocates distinct
final stacks and static reserved entry descriptors before final mappings.

The new assembly trampoline checks SBI's MMU-off entry, expected hart and
T-Head MAEE state, installs a private early trap recorder, enables FP state,
loads all descriptor inputs before switching SATP, fences TLB/instructions,
touches its private stack and acknowledges final mappings. It calls the
normal kernel ABI with the logical CPU index. The boot hart starts only
selected stopped APs, with bounded entry waits and checked HSM return codes.
Probe modes 1-3 cannot start APs. Mapping validation covers every AP stack,
descriptor and trampoline. Late failure can mean partial AP entry; stop
diagnostics now describe the boot hart's MMU state, not falsely "no kernel".

ASan/UBSan tests execute the production startup body and topology policy,
cover all 64 choices of boot hart, every supported count, missing/duplicate
identities/contexts, bad descriptor/HSM state, start errors, entry traps and
timeouts. Existing preload/handoff/boot-helper/card-policy tests still pass.
The integrated loader cross-build passes and restores shared generic output.

Live sequence (all times UTC, no SD/firmware/default-boot change):

- Before the first SMP trial, the previously running single-core September 4
  Samsung kernel panicked at a user-mode execute fault at zero in sshd-session
  thread 616. The normal shutdown request did not complete; relay recovery
  was an unclean stop, not a confirmed shutdown. Do not attribute that earlier
  fault to the not-yet-run SMP candidate.
- `smp2-20261014-1`, serial boundary 7685191: loader refused the PLIC table
  before any AP or kernel entry. Actual inactive machine-mode slots use
  `ffffffff`; the supervisor slots use 9. The corrected parser preserves
  the inactive slots' numbering and never assigns them to a CPU. This was
  checked against both the saved and live Linux FDT; a regression test covers it.
- `smp2-20261014-2`, boundary 7753311: loader `smp-3` booted the unchanged
  Samsung kernel `38d4a424...49a65b` with logical CPUs 0/1 on harts 3/0.
  Both schedulers enabled, SSH worked and `sysinfo -cpu` reported 2. The
  pinned worker test then hit the old scheduler assertion
  `nextThreadData->Core() == core` in thread 381 on CPU 1 during snooze.
  This is **not** a passing SMP workload test. The test executable had not
  been flushed before that panic and was absent after recovery; future
  staging explicitly syncs before workload execution.
- The repository's later affinity/trap migration fix `a1b44844a6` was already
  SD-tested but is absent from the installed September 4 kernel. Built the
  current kernel, checked matching mirror sources and disassembled the
  context-switch CSR restoration. A native-only HPKG copy replaced only its
  kernel; path sets, retained root-opt-in ECAM, fresh BFS consistency and
  package round trip were verified. No kernel/package was installed on Samsung.
- `smp2-kernel1-20261014`, boundary 8011208: that RAM-only kernel booted both
  CPUs (boot hart 1, AP hart 0), reached userspace, and returned `sysinfo`=2
  over SSH. The pinned executable was missing, so **no worker test ran**.
  Subsequent network access stopped without a fatal panic in the inspected
  serial capture. Do not call this kernel or two-core mode stable. Normal
  SSH shutdown could not be confirmed; relay recovery was required again.

The next controlled comparison is all 64 CPUs with the same private kernel;
it is not validated yet. The 3 GiB RAM restriction remains separate. The
bounded userspace smoke test checks CPU enabled state, real placement,
arithmetic, private memory across sleeps, and the final CPU mask. Its newer
revision bounds joins and terminates only its own test threads on timeout.

Current experimental artifacts:

- Loader `smp-4/HaikuPioneerKexecProbe.Image`:
  `7f9788287e1851789ff41b098c910d613df51c4044256278f104b9b806082fcb`.
  Startup code is unchanged from the two-core-booted `smp-3` image; only the
  partial-entry stop wording changed. It has not yet been booted.
- Fixed kernel: `cd02cdb537a41641a1eb6442abd6cfd083614243b85a7841ebf824291e8013e3`.
- Private `smp-kernel-bundle-1/samsung-private.hpkg`:
  `71bb415fb2968c1119d6e177ed69213546104a5765990b6e3f15849570e21948`.
- Its RAM BFS: `a3e1015fb56a440d5ea75a538b590c59cebd1ce99183b176eaef2ac703010c28`.
- Bounded CPU test: `6bd45e4c49a3f49d31de0bc95aaaf9de4ed216045f4c290f84e822ab214701a6`.

The old single-core loader/bundle and all intermediate private candidates
remain available. All host tests/builds are evidence of source/artifact
checks, not proof of 64-core runtime correctness or P100A inference.

### All-core handoff and archived-kernel control, October 14

`smp64-kernel1-20261014`, serial boundary 8258118, booted loader `smp-4`
and RAM kernel `cd02cdb5...` with all 64 harts. CPU 0 was physical hart 3;
all secondary schedulers reached their enabled marker. The MSI-only RTL8125
override loaded (hash `fe6273c9...`), but networking became unreachable before
the bounded worker executable could be staged. The inspected serial tail
showed continuing xHCI timeouts, not a fatal panic. No CPU workload passed;
neither the 64-core nor earlier two-core candidate is validated. A bounded
SSH shutdown attempt failed with "No route to host"; relay recovery was
therefore unclean, not a claimed normal shutdown. NVMe Linux returned with
64 online CPUs and an empty kexec slot.

For an isolated control, copied the old SD checkpoint to a disposable local
image and extracted only its public OS package/kernel. Original image
`haiku-pioneer-bfs-trap-migration.img` still matches deployed SHA-256
`bb81f152e2c3859aa8e47aa0a71c12ff70a1cf799468ba6df7c9878a9dad26a4`.
Its packaged kernel is `660147f6ad79cdbe1d1f60b4abffeb55b9ed655d5617959820bf7ddfa62cd2d8`,
not the earlier recorded build-object hash `5902003a...`; package generation
revision-stamps a copy. Its `_haiku_revision` contains `hrev99999`, and
disassembly independently confirms the context-switch stvec/sscratch fix.
Use the exact deployed image and extracted-kernel hashes as the control's
identity; do not silently substitute the build-object hash.

The fresh native-only `smp-kernel-bundle-validated-1` replaces just that
kernel in the existing root-opt-in HPKG. Sorted path sets match; retained
ECAM hash is `23e65e2a...`; BFS checkfs and full HPKG roundtrip pass.
Control HPKG: `7e4ace3490f560bd1b3df70b2b56d72229cf24c6527492a84a59b336ca00c43b`.
Control RAM BFS: `5f2f13fd45946aaae8038f20facad6638574468801c49497085443558a13151a`.
The boot helper's optional, fixed `validated-control` profile pins that
bundle and binds the profile into each fresh session's input manifest.
Samsung's installed kernel/package, default Linux boot and firmware remain
unchanged. Session `smp64-validated1-20261014`, boundary 8492494, was prepared
and dispatched via normal `systemctl kexec`; its runtime result is pending.

The archived kernel's identity was also verified in memory: zeroing only its
128-byte `_haiku_revision` section reproduces **exactly** the original
`5902003a...` hash. The control reached all 64 per-CPU initialization records
but stopped after SG2042 MSI setup, before SSH. No passing workload follows
from this control. It lacks current KMessage/driver-priority fixes; the
stopping point alone does not establish which difference matters. SSH was
unreachable; relay recovery was unclean again. Default NVMe Linux recovered.

Prepared a current-source control with only low-latency rebalancing's load
comparison restored to its old capped values. This is a diagnostic experiment,
not a proven fix. Kernel `beff31ad047033dc0b76685aaaffe0f6fe53c94d07442efb1d09f9caea1cceac`;
fresh native-only HPKG `38ac0e19bd52dda8e65a383c3e9eb18a4160fd940a58002bdeacb26858cd2ced`;
RAM BFS `1b247841bdf4a5d5ecd7239103dee0c359f24c7fa3224f8bae7eb8f72115a626`.
The same package-path, retained-ECAM and BFS roundtrip checks passed.

Added an explicitly armed, one-shot serial userspace CPU test. The launcher
requires a fixed token and executable hash, consumes/flushed its arm before
work and never reboots or touches the accelerator. It does not modify SSH's
configuration or startup script. Host tests cover missing/invalid/symlink
arms, bad executable hash, previously consumed arm, success and failure.
The test executable now uses unbuffered output for serial visibility:
`cbabe84a18808d4610e56c829f48e01b798a9da9f51e9b86d932fc414df3e139`.
Staging session `smp-serial-stage1-20261014`, boundary 8615713, uses the
previously working single-core profile; deployment and 64-core test pending.

Single-core staging succeeded after diagnosing a Mac-side cached REJECT
route to the board's known IP. A scoped ping returned 3/3 packets, and pinned
SSH immediately returned CPU count 1. Therefore earlier SSH timeout/no-route
observations are **not by themselves proof** of a kernel/network deadlock;
future readiness checks must include this route/ARP condition. The earlier
serial observations and absence of workload results remain valid.

Staged and hash-verified the serial diagnostic on live Haiku, flushed it and
installed only the new `/boot/system/settings/launch/pioneer-smp-smoke` job.
Existing SSH settings/startup were untouched. Script hash:
`c7e3825a7a24e952f5e6699b9d6e2b5464956474d6d0e54f6af2f1cf56dc6f73`.
The job refuses a non-64-core staging boot without consuming its arm; on a
64-core boot it renames `armed` to `consumed-v1` and syncs before executing.
The fixed expected binary hash is embedded in the script. The test-only
files are under `/boot/home/pioneer-smp-20261014`; no package was installed.
Armed token `smp64-serial-v1` was synced. `shutdown -s` dispatched and closed
SSH normally; final shutdown completion must still be confirmed on serial.

### 64-core workload passes; subsequent runtime stall still under investigation

`smp64-capped1-20261014`, serial boundary 9127700, used loader `smp-4`
and capped-comparison kernel `beff31ad...` (boot hart 2). The armed serial
test ran and completed with status zero: all 64 enabled CPUs reported
correct pinning, sleep/wakeup placement, private-memory checks and checksum
`7f2dc078`; final mask `ffffffffffffffff`. Pinned SSH then reported 64 CPUs.
One direct repeat and five further saved repeats all passed: **seven passes**.
This validates basic all-core work, not full OS stability or all RAM.

Retired the temporary launch job by moving it to
`/boot/home/pioneer-smp-20261014/pioneer-smp-smoke.completed.launch` after its
hash check, and synced. Its consumed arm and binaries remain in that scoped
directory; existing SSH startup/configuration is unchanged. The tracked SMP
helper defaults to this workload-passing capped profile; the older uncapped
profile remains an explicit comparison. This is a conservative baseline,
not evidence that the capped comparison fixed earlier network problems.

A later SSH request intended to recheck the P100A timed out before any result
or new authentication appeared in the inspected serial tail. No card test
result can be claimed. ICMP then failed even with a valid, non-REJECT Mac
route, so the renewed runtime stall is not explained by the earlier route
cache alone. No new core start/DMA/firmware/flash operation was attempted.
Bounded SSH shutdown failed; relay recovery was unclean again.

Source review found an independent, concrete RISC-V context-switch bug:
setting the destination address-space CPU bit and immediately clearing the
source bit clears the **same map's** bit during same-team switches. The old
map was also cleared before SATP/local-TLB switch completion. VM Flush uses
this mask for user-page shootdown targets, so that bookkeeping can omit an
actively executing CPU. Corrected membership and ordering; production-body
ASan/UBSan host tests cover all CPU indices/counts, same/different maps,
atomic selection, destination-before-SATP and source-after-flush ordering.
This source proof is not yet proof of the network stall's cause.

Cross-built corrected kernel:
`d7aa573407757c25402793bb24c576f465982778966ce38eea26541936ae8c0c`.
Added a bounded userspace remapping test: controller on CPU 0, two workers
per CPU 1..63, 32 alternating fixed mappings of two independently resident
scratch-file pages. Workers park before each remap and verify after release;
no arbitrary/existing mappings or data are replaced. Worker joins and runtime
are bounded; the 8 KiB new scratch file is unlinked while held open.
Test binary: `32ae2ee7a1d3dc79ca4e711e65dbe57840ba9d71900d66fbe7d62c2e8cf3caf9`.
Neither corrected kernel nor VM test is hardware-validated yet. Linux recovery
is booting; P100A inference remains unimplemented, not a CPU-only substitute.

Corrected-kernel native bundle checks passed with unchanged package paths and
ECAM hash, clean private BFS check and exact HPKG roundtrip. RAM BFS:
`8a0110715013a322ace77f896a4ce00857c3b18ca4d7106edeac46a6fdfe5ced`;
HPKG: `ced1b4904d5ce9e79475f38953973f3add47d15cc84f6e5a98d53ea88e6817b0`.
Helper `pioneer_linux_boot_haiku_smp-active-map-1.sh` hash:
`99737a29a8eb85a136421ed55ec0febe3992bb822fd7ed400e357210176501c9`.
Fresh session `smp64-active-map1-20261014`, boundary 9443031, was dispatched
through normal Linux shutdown. Runtime result is pending. The VM executable
was rebuilt with phase/worker failure diagnostics (same workload), new hash
`26ccd9ca7c015675f6e59aaf1599e440a4a98a7068c8120d4cc98112d5dcf104`.

The corrected-kernel trial returned pinned SSH and CPU count 64. Its all-core
smoke passed, then the VM test passed all 32 rounds on all 126 workers, then
the all-core smoke passed again. Full CPU mask remained `ffffffffffffffff`.
This is hardware validation of these bounded workloads, not proof that every
runtime path is stable. The tracked helper now defaults to this stronger
active-map profile, while retaining all earlier pinned controls explicitly.

The subsequent P100A revalidation session authenticated but returned no
profile/health result before the machine stopped responding. The user also
confirmed the desktop and input were frozen, so this stall is not merely
an SSH failure. The source fix is retained as an independently demonstrated
bookkeeping correction, **not** claimed to resolve the whole-system freeze.
No P100A compute, core-start, DMA or firmware update result exists. A normal
shutdown attempt failed; relay recovery was again required.

Added diagnostic-only stage markers in probe discovery/init/open, profile
semaphore/module lookup, and the six existing read-only topology stages.
The profile CLI now prints unbuffered entry/open markers. Hardware policy,
ioctl layouts and all PCI/TLB/data restore operations are unchanged. All six
root/MMIO/ARC/telemetry/reset/L1 policy suites still pass. The plain Jam target
was not found; the correctly qualified `<driver>tenstorrent_pci_probe` build
then succeeded. Trace driver SHA:
`0d0946532efb494b264c0048c84dcd602030915f837b14020294f7ecbf1967c7`.
Trace profile CLI SHA:
`967da2b70f736923f36ac379696dbd0a1ef73ee2d3df4741531cd6e36566b657`.
Neither is deployed yet; original driver `f0ab0eba...` is the rollback target.
The next trial must determine whether discovery/loading or profile inspection
causes the freeze before any later card operation is retried.

### RAM cap diagnosis and guarded high-memory implementation, October 14

Session `smp64-p100a-trace1-20261014`, serial boundary 9734650, returned
64-core Haiku with active-map kernel `d7aa5734...`. The trace-only optional
probe was installed after an exact-hash rollback copy and sync; its original
`f0ab0eba...` binary remains at
`/boot/home/pioneer-p100a-20261014/smp-trace-driver-1/tenstorrent_pci_probe.before-trace`.
Read-only profile CLI `967da2b7...` completed all six stages, status zero,
ready=1, with root command 0000 and the unchanged five host windows. No MMIO
write/core start/DMA occurred. A subsequent health request failed during SSH
preauthentication with "Bad packet length 2139826825"; it never entered the
card script. ICMP still responded. Later pinned SSH worked again, so this
specific event is not evidence of a whole-system freeze or a card-test fault.

The user's RAM observation was confirmed directly: `sysinfo -cpu` returned
64 and `sysinfo -mem` returned maximum 3,185,573,888 bytes, exactly **3038 MiB**.
The kexec adapter uses `gMemBase=0x2200000` and its temporary boot allocation
ceiling 0xc0000000 as the kernel's only physical memory range. Thus 3 GiB
minus 34 MiB explains the display exactly. This is not a hardware limit.
Saved Linux FDT and its boot log show four contiguous 32 GiB banks, totaling
128 GiB, with the first 34 MiB excluded. The adapter's 3..4 GiB exclusion is
a conservative loader policy, **not** a missing-RAM hardware hole: the Linux
DMA32 range includes that interval.

Added a separately requested `haiku,pioneer-memory-gib` trial (8/16/32/64/128),
leaving the absent-property default unchanged. It requires the exact four
FDT banks, excludes rounded FDT reservation-table/static reserved-memory
intervals plus the live FDT and initrd, refuses unknown dynamic reservations,
and keeps the loader's contiguous allocator below its first reservation.
The kernel-visible ranges are independent of that low allocator. The linear
physical map spans their physical offsets while leaving excluded holes
unmapped; its entire virtual span is protected. A post-semaphore VM block
reserves a large map permanently, without replacing mappings or overlapping
the existing final-page overflow-protection area. Handoff checks cover each
usable range's first and last mapped bytes and retain the existing MAEE policy.

ASan/UBSan tests execute the production FDT parser, sparse-map construction
and large-map reservation against two saved real FDTs. They cover absence
of opt-in, all accepted ceilings, missing/disabled/malformed banks, unknown
dynamic reservations, page rounding, overlap, overflow, full fixed arrays,
reservation failure and unchanged small-map behavior. Existing loader,
handoff, preload, topology/HSM, context-switch and helper tests pass.
Private loader build and generic-profile restoration succeeded. Loader:
`609691a0500bb6d58b6dd08a47581b3d7c536916e5229606d28627a10f8c2dc0`.
Kernel:
`1dca50d6976fb4629f0eca35be7c81305408ec4a053882d37135c3ef2ca99c91`.
Neither higher-memory profile is hardware-validated yet. The old loader,
bundles and installed Samsung kernel/package are unchanged.

Review caught a top-of-address-space edge case before hardware: the generic
range insertion helper does not merge an exclusive end that wraps to zero.
The sparse mapper now registers its entire span **once**, not individual
subranges followed by an overlapping top-ended span. Its production-body
test asserts component mappings do not create reservations. Final loader
`memory-2` SHA:
`b5473e6d00a9bcf469fce648b4415f28eda2dbc2510bfc9ed8d61af9034e6e98`.
The earlier `memory-1` loader was staged but never executed.
Final kernel hash is unchanged. Private native HPKG:
`8fe3b460a99c95a4d988790e525484b1bc8b6aa834dc98ffff3d7e9021174a93`;
RAM BFS:
`98eebc001c7c869566a81ec445ee71798ec98be05d7585e18d416f8029c759cd`.
Kernel/ECAM extraction hashes, sorted package path sets, fresh private BFS
check and full HPKG roundtrip passed. The new helper's `memory8-control` and
`memory128-control` explicitly pair this loader with its matching kernel;
the default remains the old active-map 3038 MiB profile. Helper SHA:
`f1c63c21c1f5b5c02c44b1f3f7a1a2183557835b0abce15a5728f965d57f64ee`.

Added a bounded, private RAM-sample test with 64 pinned workers, two full
address-dependent patterns, sleep/wake validation, 62-second join deadline,
and test-team-only failure termination. It requires >3 GiB reported RAM and
the requested sample plus 1 GiB free headroom; permits only 64..4096 MiB.
It does not claim to audit every physical page. Cross-built test SHA:
`410d48f2cb49a1bf6b189b2dbdf20c4f710dba7f5ab3520c447db339f28ade6a`.

Requested `sync; shutdown -s` over authenticated Haiku SSH; serial showed
service termination but no final shutdown-complete marker. Finished with
the relay off hold, waited ten seconds after completion, then powered on
for default NVMe Linux. Do not describe this as a fully confirmed clean
shutdown. Next is a private 8 GiB trial before attempting the full span.

The first 8 GiB/64-core trial `smp64-memory8-1-20261014`, serial boundary
10015540, passed loader checks with two usable ranges and physical-map span
0x1fde00000. The low allocator correctly stopped at the first FDT reservation
(0x40000000), independently of kernel-visible RAM. Kernel startup completed
and Samsung root identity checks passed, but boot then stalled before SSH.
Serial showed a kernel store-page fault at VA 0xffffffc0016e7030, leaf PTE zero,
PC 0xffffffc0001ac0e0 (kernel-relative 0x12c0e0, ObjectCache::InitSlab), called
by HashedObjectCache::CreateSlab. The "before VM" trace label means before
the fault handler calls VM, not before VM initialization. Some root-scan
output followed the fault, so the trace alone is not a proven fatal panic.
ICMP did not respond and no RAM/CPU userspace workload ran. Do not claim
8 GiB validation, restored 128 GiB, or identify the root cause from this alone.
The 128 GiB trial has not been attempted.

Bounded pinned-SSH shutdown timed out. The relay off hold was therefore
recovery from an unresponsive experimental boot, not a clean OS shutdown.
No SD change, firmware update or P100A operation was involved. The default
NVMe Linux boot and the old 3038 MiB Haiku control remain the recovery paths.

Recovery was verified over pinned SSH at 22:59 UTC: Linux 6.1.31, CPUs 0-63,
root /dev/nvme0n1p3, kexec_loaded=0, Samsung unmounted, no MMC device, exact
original extlinux SHA `53c0da6d...`, and P100A command 0000. The machine is
left running Linux, not the failed experimental Haiku boot. Full host RAM
and P100A LLM execution remain pending; basic all-64 CPU and remapping passes
on the old low-RAM profile remain the strongest successful Haiku controls.

### High-memory isolation control, October 14

User requested restoration of the old full-RAM/CPU/desktop baseline before
P100A LLM work. The original full-memory fix `7be8406fed` remains present;
October 12's SD/UEFI record reports 64 CPUs and 136,289,968,128 bytes RAM
(126.93 GiB). This is stronger prior evidence than hardware enumeration in
Linux alone. The SD-free adapter bypasses that EFI map-building path.

Without changing any kernel or loader input, session
`smp2-memory8-isolate1-20261014`, boundary 10170193, selected two CPUs and
the same `memory8-control` profile. It booted userspace and pinned SSH.
`sysinfo` reported two CPUs and 8,500,535,296 bytes RAM (7.92 GiB).
The existing two-CPU affinity/work/sleep test passed mask 3. This control
does not establish why the previous 64-core boot stalled, but it separates
an ability to expose/use high memory from all-core runtime stability.

The private memory test now accepts an optional explicit expected count
(2/4/8/16/32/64), retaining its default 64-CPU requirement. Count must match
the running system; the allocation/headroom/deadline and test-team-only
failure boundaries are unchanged. New binary SHA:
`b1773fd1cdc23d1074c864ab8bd9b8b365cd08b1b699e44e02792cc33786b963`.
Staged as `/boot/home/pioneer-memory-20261014/pioneer_memory_smoke-v2`.
Both the 64 MiB and **4096 MiB** samples passed two complete address-dependent
patterns on two pinned workers, mask 3. Post-test SSH/sysinfo still worked;
free RAM returned to 8,124,702,720 bytes. This is a bounded private memory
sample, not an audit of every physical page or proof of 64-core stability.
Some recoverable kernel page-fault traces also occur in this successful run;
therefore the earlier fault trace alone must not be presented as a proven
root cause of its stall. Next: a full-span two-CPU control before another
all-core comparison. P100A compute/DMA/firmware operations remain untouched.

The successful two-CPU/8 GiB control also completed a normal `shutdown -rs`
into default NVMe Linux, without a relay pulse. This validates an ordinary
Haiku reboot in that control; it does not establish that shutdown/reboot
works when the kernel has stalled.

The first full-span two-CPU trial, `smp2-memory128-isolate1-20261014`, boundary
10434050, passed loader handoff checks but panicked in arch_vm_init_end:
"cannot reserve large physical memory map: -2147483643" (B_BAD_VALUE).
Its map began at 0xffffffe014e0b000. Source and disassembly show a concrete
conflict: vm_init's compiled 64-bit freed-heap poison guard reserves
0xffffffefdeadb000 (size 0x40000) before the later physical-map
reservation. That address lies inside a 128 GiB linear map but outside the
8 GiB map. This explains this particular deterministic reservation failure,
not the earlier intermittent all-core stall.

Moved the large-map reservation into vm_init, after bootstrap-area conversion
and kernel-argument areas but before overflow/poison guards. The final page
still belongs to the existing overflow protection. Optional poison guards
whose ranges intersect the RISC-V physical map are now skipped; low guards
and non-RISC-V behavior remain unchanged. Production-body sanitizer tests
cover the actual poison address, map-boundary overlap, retained low/non-RISC-V
guards, failure handling and the source ordering relative to conversion and
all four poison placements. Two real-FDT test runs and the cross-build pass.
Kernel SHA:
`dab2171f6d7d80e2a2879eb7ffd2d6c501eb5066a3da9e83b3398cf659174e7f`.
An initial relative-path mirror copy failed from the build working directory;
no corrected binary was assumed from that invocation. Retried with absolute
paths, set -eu and cmp before the successful build. The loader is unchanged.

The failed full-span trial stopped before mounting Samsung root. Recovered
with the relay and verified default Linux 6.1.31/NVMe root with an empty
kexec slot before staging a fresh corrected RAM-only bundle. No original
package, boot entry, SD, P100A command/DMA state or firmware was changed.

Corrected private bundle verification passed: RAM BFS SHA
`98783b7aa58b283c9ce33623deecd828eaf5a8f20394cfa92a2fe1065c098ebf`,
HPKG SHA `b52f7da807a96fe2c139a74a98a8ea6a7c3e3950f51927fda47d9dd705bb1e2a`.
The extracted ECAM module remains
`23e65e2a9a3e886fc7d1429901db6534f2cc7a9077f121d3e334f4cf839cf11d`.
The helper adds explicit `memory8-map-order-control` and
`memory128-map-order-control` profiles without changing the low-memory default;
SHA `a069094bbafb59b77b36a6d5690982c4697285131dad46ef6f1447323e2a0b83`.
At 23:17 UTC started `smp2-memory128-maporder1-20261014`, boundary 10508568,
with the corrected kernel and two CPUs. It passed the former reservation
panic and reached device/root/package initialization. Runtime memory and
CPU validation are pending; this is not yet a full restoration claim.

That corrected two-CPU/full-span trial reached userspace, sshd listeners and
DHCP at 10.0.0.247. One ICMP reply succeeded; subsequent pinned SSH attempts
timed out, then ICMP stopped replying as well. Serial stopped after thread
239 at byte 10681164, with no final panic. No CPU/RAM userspace test ran.
The reservation conflict is bypassed, but full-RAM runtime is not validated.
Requested a non-blocking desktop/input observation and attempted a bounded
normal SSH reboot; it timed out. Relay recovery is therefore recovery from
an unresponsive experimental boot, not a confirmed clean shutdown. The
source-order fix must not be advertised as fixing this separate stall.

Recovered default Linux again at 23:23 UTC: NVMe root, empty kexec slot,
P100A command 0000. Linux /proc/iomem confirms continuous System RAM from
0x02200000 through four 32 GiB banks to 0x1fffffffff; MemTotal is
131,976,904 kB. This does not diagnose the Haiku stall. The next 64-core
prepare stopped at its Linux befs read-only check: pending BFS journal
entries after the unresponsive boot require normal Haiku replay. No
64-core/full-RAM handoff occurred in that session.

Added an explicit `memory8-journal-recovery-control` profile, restricted to
the previously passing two-CPU/8 GiB loader/kernel pair. All other profiles
retain the direct read-only verification and the low-memory default is
unchanged. Its pinned verifier places both Samsung whole disk and partition
under kernel block-read-only protection, replays only into a fresh private
qcow2/NBD view, extracts only the OS package and RTL override and requires
their original hashes. It never repairs or mounts the physical BFS in Linux.
NBD is detached, the module removed if newly loaded and the original write
policy restored before returning. A failed/disconnected attachment keeps the
source protected if cleanup cannot establish safety. A first attempt stopped
before BFS access because NBD capacity was asynchronous; cleanup passed.
Added a bounded capacity-readiness wait; two subsequent extractions passed.
The original superblock stayed unchanged and Linux taint remained zero.

Verifier SHA:
`8ff6ad3643e4baab6734cbb7799c1ba02bb8beb197cf43f3540a66c2b02c7ff9`.
New native helper `pioneer_linux_boot_haiku_journal-recovery-1.sh` SHA:
`c33e566a92987592c0934b88441f2ac15d71274721e1475d1d0031595d3b0cde`.
Argument/syntax and existing boot-helper tests pass. Prepared and started
`smp2-memory8-journal-recovery1-20261014`, boundary 10746492, at 23:29 UTC.
Only the subsequent normal Haiku root mount may replay the original journal;
no offline original-disk repair or firmware/P100A operation was performed.

The journal-recovery control booted live Haiku with pinned SSH, two CPUs and
8,500,535,296 bytes RAM; a 64 MiB private sample passed again. Added a separate
one-shot full-memory serial diagnostic so the next full-span trial does not
depend on SSH. It requires Haiku/riscv64/UID 0, exactly 64 enabled CPUs and
more than 120 GiB reported RAM, a fixed arm token, unconsumed state and the
previously built memory-test binary hash. It consumes and syncs its arm before
running only the bounded 64 MiB/two-pattern private sample on 64 pinned workers.
There is no card access, model download, reboot or whole-RAM-audit claim.
Host tests cover arm/symlink/hash/refusal gates, the exact RAM threshold,
malformed sysinfo and both worker success/failure reporting.

Script SHA `098ab90ea60ad6dc348b3e95299209ac0b18365ec82b03e641b97327925a8c27`;
launch-job SHA `bd89b3af0084fee5fb849ef36af544a3f75325f11a167d6def968b95d29218f5`.
Installed only the new `/boot/system/settings/launch/pioneer-full-memory-smoke`
job and `/boot/home/pioneer-memory-20261014/boot-full-memory-smoke.sh`, with
staged originals in that directory's `full-serial-1` subdirectory. Existing
SSH configuration/startup is untouched. Armed `armed-full-v1`, then manually
verified the two-CPU control refuses without consuming it. All files synced.

The recovered control also repeated the **4096 MiB** private sample with both
patterns and mask 3 successfully. The subsequent synced `shutdown -rs` closed
SSH and began Radeon/USB endpoint teardown, but unlike the earlier control
did not reach firmware restart within the bounded observation. Serial stopped
at byte 10953061. This is a passed RAM sample, not a confirmed clean reboot;
relay fallback was required again. The source of this teardown stall is not
proven by the last XHCI messages. Do not blame a specific driver from that
output alone or conflate it with the full-memory startup stall.

After the off hold completed, waited more than ten seconds before the on
pulse (about 23:35:40 UTC). Unlike earlier recoveries, no ZSBL/firmware text
has appeared by 23:37 UTC. The same screen reader still owns the UART and
its log remains at byte 10953061; scoped ICMP/Linux SSH cannot reach the
known address. Relay status confirms the power-button contact is released,
but does not measure board power. Asked for the current fans/LEDs/display
state before any further pulse; do not guess it is on or off. Both SDs stay
out and the old SD relay is unused. Full-RAM/64-core comparison is pending,
not executed. The one-shot arm remains staged for that future trial, with
its 64-core/>120 GiB guards; it does nothing in Linux or the recovery control.
No P100A compute, DMA, instruction loading or firmware operation ran.

### October 14 evening: full-memory/all-core comparison

The user confirmed fans/LEDs off after the stalled recovery. A further single
on pulse at 23:57 UTC produced no observed startup; the user said it needed
to sit, so stopped power actions. Following their later manual power-on,
fresh ZSBL and Linux startup appeared. At 00:57 UTC October 15 (still October
14 locally), pinned SSH confirmed default Linux 6.1.31, NVMe root, CPUs 0-63,
131,976,904 kB RAM, empty kexec slot and P100A command 0000. This observation
does not establish why the earlier power-on failed or prove a thermal cause.
No relay pulse was sent during this user-managed startup.

Prepared the corrected full-memory/all-core profile in fresh session
`smp64-memory128-maporder2-20261014`; Linux's direct read-only Samsung mount
and original OS/RTL hash checks passed, so no further journal overlay or
offline repair was needed. Started at 00:58 UTC, serial boundary 11018389,
using the unchanged `memory-2` loader, corrected `dab2171f...` kernel and
`98783b7a...` RAM bundle. Loader verified all 64 CPU/PLIC records (boot hart 2),
two RAM ranges and physical-map span 0x1feb1f5000. Kernel reached its scheduler
without the former reservation panic. Serial one-shot and userspace/runtime
validation are pending; full restoration is not yet claimed.

That full-RAM/64-CPU trial stopped progressing before launch_daemon. Serial
ends at byte 11117550 after debugger-module initialization and the inactive
ECAM root 0x40 refusal. No new panic, CPU/RAM userspace workload, serial
one-shot or SSH listener was observed. The last logged driver is not proven
to cause the stall. The user still saw Linux's shutdown screen; loader and
kernel serial show Haiku took control, so the framebuffer had not yet been
replaced by a Haiku desktop. Full host functionality remains unresolved.

Prepared a separately compiled diagnostic, not a runtime fix: explicit Jam
`-sHAIKU_PIONEER_KDL_TIMER=1` enables a one-shot timer only on logical AP1 with
64 CPUs and a >120 GiB physical-map span. At 120 seconds it deliberately
requests KDL, even on an otherwise successful boot, to inspect stalled boot
CPU state without userspace/SSH. It neither resets hardware nor automatically
reboots or re-arms after continue. It can still fail to provide a capture if
that AP cannot service its timer. Host production-body sanitizer tests cover
CPU/memory gates, exact delay/mode, explicit debug entry and add-timer errors.

Default build and a second default rebuild after the opt-in both remain
byte-identical to kernel `dab2171f...`, with no diagnostic marker. The private
kernel contains the marker and has SHA
`f15e31781218b404db862d4763b916310f1b2e33ba09dc57ee13b961e9a39c4f`.
It is saved separately under `memory-kdl-timer-kernel-1`; the build mirror is
back to the normal kernel. Native packaging/deployment and timed KDL are
pending. No installed Samsung package, default boot, SD or P100A state changed.

Bounded normal SSH reboot failed for the stalled comparison. Recovered with
the relay off hold, a longer off interval, then one on pulse at about 01:10
UTC. Fresh ZSBL and default Linux returned; pinned SSH verified NVMe root,
empty kexec slot and P100A command 0000. This is relay recovery, not a clean
Haiku shutdown, and does not prove why the prior power-on attempt failed.

The private diagnostic bundle passed native package path-set comparison,
kernel/ECAM extraction hashes, fresh private BFS check and full HPKG roundtrip.
RAM BFS SHA `abbd25643a1787f3a857535f4becee8f8b7fe7578404fb1dae52b425afae1e68`;
HPKG SHA `566f44dd4ebd0fbb811fe829faf3173f10294d4b8ed40ffbf96c1d5f40e35261`.
ECAM remains `23e65e2a...`. New native helper
`pioneer_linux_boot_haiku_kdl-timer-1.sh` SHA
`1782286b9683097fca48d9535b18d022337688e46fb9dae93c13809145d17f3f`
adds explicit 64-CPU/full-RAM KDL profiles; defaults and old pins stay unchanged.
A separate explicit journal-view variant retains the pinned read-only backing
verifier if needed; it has not been used for this diagnostic. Direct Samsung
read-only mounting and original boot-input hashes passed again.

Prepared and started `smp64-memory128-kdl1-20261014` at 01:14 UTC October 15,
boundary 11184226, using the unchanged loader and separately pinned diagnostic
kernel/bundle. The intended KDL timeout is diagnostic, not a detected kernel
panic. The normal kernel artifact has already been restored on the build
mirror; no installed/default boot or P100A operation was changed.

The first timed diagnostic reached main2/device initialization, with a
successful timer-programming trace, but serial stopped at SDHCI
`P214:SR0 reset all begin bits 0x0`, byte 11232317. Neither the intended KDL
banner nor a response to a read-only `help` sent through the existing screen
session appeared after the timeout plus grace period. No usable stack capture
or RAM/CPU workload result exists. This does not distinguish a timer/IRQ
problem, debugger-entry/output blockage or another cause, and is not proof
that the SD controller caused the earlier different stall.

Prepared one further private isolation profile, `memory128-kdl-nosd-control`,
using the same timed kernel/bundle. Both SDs must remain absent under the
existing global guard. It removes only `/soc/bm-sd@704002B000` from the fresh
private probe FDT after checking exact compatible, register range and IRQ;
eMMC, CPU and memory information remain. It does not alter the live Linux FDT,
installed OS, default boot, persistent SD support or any card. Source review
shows the current Haiku FDT enumerator ignores status="disabled", hence actual
private-node removal instead of an ineffective status property. Host tests
exercise the production edit, source/same-inode preservation, exact identity
refusals, CPU/timebase preservation and the full memory-parser tests on the
modified DTB. Hardware deployment/result are pending; this is not a claimed fix.

Recovered the unresponsive timed diagnostic by relay after another bounded
normal reboot timed out. Allowed the longer powered-off interval before one
on pulse; default Linux returned and pinned SSH verified NVMe root, empty
kexec slot and P100A command 0000. The timed diagnostic had stopped before
Samsung root/userspace; the next direct read-only source verification passed.

Staged helper `pioneer_linux_boot_haiku_kdl-nosd-1.sh`, SHA
`c99e456013abc2747f473febd7b2c5bf9f9139e019daab90910f1f40cebc5ad2`.
Prepared and started fresh session `smp64-memory128-kdl-nosd1-20261014` at
01:29 UTC October 15, serial boundary 11297645. Same timed kernel, loader,
RAM bundle and 64-CPU/128 GiB settings; only the private SD node is omitted.
Live FDT/default boot/installed package/SD contents/P100A remain unchanged.
Observe before attributing a result to any particular cause.

That no-SD diagnostic completed all 17 main2 stages: root mounted,
launch_daemon loaded/resumed, and secondary schedulers enabled. The bootstage
summary covered 22,037,804 us, ending at 221,087,446 us. Serial then stopped
after user threads 95/96/97 entered; no SSH, full-memory one-shot result or
timed debugger banner appeared. This is progress past the SD reset, not a
working full-memory system or proof that SD caused the other stalls. Normal
SSH reboot timed out. Relay recovery with a longer off interval restored
Linux; pinned SSH confirmed all 64 CPUs and 131,976,904 kB RAM, empty kexec.

Review found the first diagnostic armed its timer after scheduler_start(),
which may switch away before returning. The private revision 3 arms AP1
between its two rendezvous, before scheduler_start(). CPU 0 requires an
atomic release/acquire arm flag before its scheduler entry and emits a
positive arm checkpoint. The callback emits its own checkpoint before KDL.
This distinguishes failure to arm from failure to reach/print the callback;
it does not itself fix the underlying runtime stall. Production-body
sanitizer and source-order tests pass. The regular build, with the opt-in
absent, is again byte-identical to dab2171f... and contains no timer markers.
An initial fast touch/rebuild left the diagnostic object unchanged because
source and object had the same timestamp second; a later forced source
timestamp update rebuilt the ordinary object successfully.

Private revision-3 kernel SHA:
`6b40ab26227afaeb77c6a7e7219872cc1a1517e6b925d2bf61ae1853a4da758b`.
Fresh native bundle `smp-kernel-bundle-memory-kdl-timer-3` passed package-path,
kernel/ECAM extraction, private BFS checkfs and full HPKG roundtrip checks.
RAM BFS SHA:
`04b00965d0b52bb75b08863c805cc96b604e241ae2bb10906ce68dfd01b078e3`.
HPKG SHA:
`836023ebdd6fc6abd128c0ffee127de6deb2bb87e5f036aa76cf7a341398e85f`.
The explicit `memory128-kdl3-nosd-journal-control` pairs those fresh artifacts
with the unchanged memory-2 loader, removes only the private SD node, and
verifies any pending source journal through the existing read-only/COW
procedure. It requires 64 CPUs; existing profiles/defaults remain unchanged.
Revision 3 has not yet been boot-tested. No P100A compute or firmware action.

The journal-profile prepare refused before staging because the source BFS
header was CLEAN (434c454e), not DIRTY. Verified empty kexec slot, automatic
UART policy and both block-device RW policies unchanged. Added a separate
`memory128-kdl3-nosd-control` using the existing direct read-only verification;
the dirty-journal profile remains explicit and unchanged. Fresh helper
`pioneer_linux_boot_haiku_kdl-nosd-3b.sh` SHA:
`2fc1f828eb53ea6a5fc6723d7b0782b10cacd6225a12982b729b7e0978d92dd9`.
Its prepare passed, then normal systemctl kexec started
`smp64-memory128-kdl3-nosd2-20261014` at 01:56 UTC October 15, serial boundary
11465656. Awaiting diagnostic/runtime results; defaults are unchanged.

### Full-memory userspace pass followed by workload stall; debugger input repair

Revision 3 reached all 17 main2 stages and positively confirmed AP1's timer
armed before scheduler release. The full-memory one-shot reported
137,034,936,320 bytes (127.62 GiB usable) and passed its 64 MiB, two-pattern
sample with all 64 pinned workers, mask ffffffffffffffff, END status=0.
Pinned SSH also returned 64 CPUs and the same memory maximum. This is a
bounded sample, not a complete RAM audit or a stable-system claim.

A subsequent explicit pioneer_smp_smoke run stalled while spawning workers;
no completion result arrived. The separate 4096 MiB request timed out before
starting, so no large-sample pass is claimed. The AP1 callback did fire and
entered KDL, warning four CPUs were not trapped. Stack 510 showed the parent
blocked on AddressSpaceWriteLocker during thread_create_thread; worker 526
was blocked on RISCV64VMTranslationMap::Lock during a user-stack store fault.
Workers 523/524/525 were asleep in snooze; earlier workers had exited. The
symbol-derived sCPUTrapped bitmap identified CPUs 8-11 as not trapped. Read-only
gCPU records (448-byte stride, running_thread offset 352, thread ID offset 48
verified against source/disassembly) identified CPU 9's thread as 65, "thread
undertaker"; the other three ran idle threads. Their missing captured register
state prevented normal stack crawls. CPU 0 was already trapped and a handover
to it confirmed its interrupted idle stack. This locates a stalled dependency,
not yet the underlying lockup cause. No debugger handover to CPUs 8-11 occurred.

Attempting the bounded remote-memory reader exposed a separate concrete
diagnostic defect: RISC-V arch_debug_serial_getchar() called GetChar(false),
so the supposedly blocking GDB reader received a no-input -1 converted through
unsigned char to 255. gdb_cmdread_handler also had no command-buffer bound.
After packet input, the UART object's vtable became invalid and debugger input
recursively faulted in arch_debug_serial_try_getchar. This later diagnostic
failure does not explain the earlier workload stall.

Corrected the blocking API to GetChar(true), the nonblocking API to preserve
the UART's int -1 sentinel, and bounded GDB command payloads to leave a NUL
terminator. Overlong packets NAK/reset rather than overwriting adjacent state.
Production-body ASan/UBSan tests use -funsigned-char and cover missing/input
sentinels, blocking selection, the exact buffer boundary and packet restart.
CPU/topology/context-switch, timer, helper and private-FDT tests also pass.
Diagnostic revision-4 kernel SHA:
`9d31e4649f0ac4b9fcfed91dfefb4d911998b10ed7d0f94e2569e375f4ac5ba0`.
Ordinary kernel, verified without the timer markers:
`3b8c940b4793c097742f97a23e113ee0dd64f356005df762446c27b0a09dce36`.
Fresh private bundle checks passed (package paths, extracted kernel/ECAM,
private BFS checkfs and full HPKG roundtrip). BFS:
`faa302273e1aa04048c9ab1da8749f41b09b4c144c7b3031769f63b5c70db023`.
HPKG: `a42cd079b6280160abbfe7b219aa30d8d2c2a2c70cab7254011adb23190f4e39`.
Explicit revision-4 clean/journal no-SD profiles retain all old artifact pins
and defaults; production feature-flag tests cover every combination.

Normal SSH reboot had timed out before KDL. After the later debugger failure,
relay off/rest/on recovery returned fresh default Linux serial and pinned SSH
by 02:13 UTC October 15: all 64 CPUs, empty kexec, Samsung journal CLEAN. No
additional power pulse was sent while waiting for its delayed startup output.
Revision 4 is not yet hardware-tested; P100A compute and firmware untouched.

Revision-4 session `smp64-memory128-kdl4-nosd1-20261014` began at 02:15 UTC
October 15, boundary 12032565. All main2 stages completed in 22,553,769 us.
Pinned SSH returned 64 CPUs and 137,034,936,320 bytes. The 4096 MiB/64-worker
memory sample began and spawned through thread 494, but stalled with no PASS.
The timer entered KDL; this time the four nontrapped CPUs were 60-63. The
fixed GDB reader successfully returned checksum-verified bounded memory reads,
including the trap bitmap, running-thread records, IPI fields and 4096 bytes
of the nontrapped worker's kernel stack; no UART corruption recurred.
CPU 62 ran thread 493 (state RUNNING); CPUs 60/61/63 ran idle threads 61/62/64.
Those CPUs' IPI counters all read 0x14a67, and CPU 62 had a queued local
message. The shared broadcast-list lock read zero, not an owned spinlock.
The saved stack contains VM-fault/scheduler/ICI frames, but no live registers
were captured for CPU 62, so stale frames must not be called its proven live
PC. Different affected quad groups make a fixed bad-CPU diagnosis unsupported.

Detached the memory reader back to KDL and requested `reboot`. Default Linux
returned with pinned SSH, 64 CPUs, empty kexec and source BFS CLEAN; no relay
cycle was needed. This is debugger/firmware recovery, not a synced userspace
shutdown. No card execution or firmware change occurred.

Prepared a separate idle-wakeup isolation comparison. Only an explicit
HAIKU_PIONEER_SPIN_IDLE=1 RISC-V build, also requiring the timed KDL build,
bypasses WFI/idle-module entry on exactly 64 CPUs. Other CPU counts and all
ordinary builds retain the existing idle path. Production-body tests run both
opted-in and ordinary variants with/without an idle module; cross-disassembly
confirms the private 64-CPU branch skips WFI. This tests a hypothesis; it is
not a confirmed cause or permanent fix. Private comparison kernel SHA:
`80a0c240387719e32411079e5bea528c50fcbb671169bbe46acc07bd3ade23b4`.
Fresh verified RAM bundle:
`657e8dea921ce3a0c8a2526c606065051bdcf39088078f3c2fb642009bf401f4`.
HPKG: `0daa5b21c1331d689d95645ccabcd0082dc4cba448a2df24fbed680c83875dd5`.
The ordinary build was rebuilt with both private options absent. Its SHA is
`3bdfd5ddc0690ebdbb46ddbe05dbf52d7de75ae81b1e1d80c7b6f32873c6ff04`;
it retains WFI/no CPU-count bypass in disassembly and has no timer markers.
It differs from the preceding ordinary checksum, so byte identity is not
claimed. The private idle comparison has not yet run on the board.

Revision-5 spin-idle session `smp64-memory128-spinidle5-20261014` started at
02:30:21 UTC October 15, serial boundary 12304944. All 17 main2 stages finished
in 22,501,221 us. Pinned SSH reported 64 CPUs and 137,034,936,320 bytes usable.
The private memory test PASSED its 4096 MiB sample, two complete patterns,
with all 64 pinned workers (mask ffffffffffffffff). This is not a whole-RAM
audit. At the deliberate AP1 timer stop, KDL reported no nontrapped CPUs,
unlike the two WFI comparisons. This supports investigating idle wakeup;
it does not yet establish a precise cause or permanent remedy.

After serial `continue`, the desktop was frozen and SSH timed out. Seven
planned CPU-test repetitions did not start; no pass is claimed. No further
serial output or response to `help` appeared. The debugger exit/resume path
may be a separate confounder; its cause is not proven. Forced relay off hold
completed at about 02:39 UTC, not a clean shutdown. Preparing a separately
opted-in spin-idle runtime test without the timed debugger interruption;
all prior frozen profiles and ordinary-build behavior remain unchanged.

Default Linux recovered through one on pulse after the longer off interval;
pinned SSH at 02:42 UTC confirmed 64 CPUs, 131,976,904 kB RAM, empty kexec,
P100A command 0000 and Samsung BFS CLEAN. This was forced recovery, not a
clean Haiku shutdown. No SD change or card operation was needed.

The private runtime comparison requires both HAIKU_PIONEER_SPIN_IDLE=1 and
HAIKU_PIONEER_SPIN_IDLE_RUNTIME_TEST=1 when the timer is absent. A spin-idle
build without either explicit diagnostic authorization is still rejected.
Built revision-6 kernel SHA:
`91c3de0034de429e212db0bd804668843541438a81148a46929ec9db4851c1ad`.
Disassembly confirms the 64-CPU WFI bypass; timer marker strings are absent.
Rebuilt ordinary objects with both options absent: kernel remains
`3bdfd5ddc0690ebdbb46ddbe05dbf52d7de75ae81b1e1d80c7b6f32873c6ff04`
and retains WFI/no 64-CPU bypass. Production-body idle, serial, timer, CPU,
context-switch, memory-parser, full-memory one-shot and helper tests pass.

Fresh native runtime bundle passed package path-set comparison, extracted
kernel/ECAM hashes, private BFS checkfs and full HPKG roundtrip. BFS SHA:
`f0dac3262f73e6e35ccb8eb2bdcda25875f5ab7841afcb0ec0522026a155cefa`.
HPKG SHA: `3a653b946e9b076c2f35e01afd5c22b0b473aa1c32c239b79afc7ef51a2f1209`.
New helper SHA:
`964d2b3ae8a6a756d5b252d91f03aa0ed9c7c4e719802bf823243126a8a8ef81`.
Explicit `memory128-spin-idle6-nosd-control` and journal counterpart retain
the old defaults and frozen artifact pins. Runtime validation is pending.

Untimed revision-6 session `smp64-memory128-spinidle6-20261014` started at
02:44 UTC October 15, serial boundary 12592078, through normal systemctl
kexec. All 17 main2 stages completed in 25,141,754 us. Pinned SSH reported
64 CPUs and 137,034,936,320 bytes usable. The 4096 MiB/two-pattern memory
sample PASSED with mask ffffffffffffffff; the 126-worker VM shootdown test
PASSED all 32 remap rounds. These requests overlapped briefly; both returned
zero. This is a bounded sample, not a whole-RAM audit or general stability
certification. app_server, Tracker, Deskbar and input_server were present;
physical desktop/input confirmation remains pending.

The old CPU smoke passed its first two repetitions, then CPU 63 returned
worker status 3 (sleep failure); SSH remained responsive. Added exact sleep
status reporting only in a fresh binary, preserving the original. Seven
repetitions passed, then an extended 24-repetition run captured one failure:
CPU 63's snooze returned -2147483638, exactly B_INTERRUPTED. This is a legal
signal interruption, not evidence that CPU 63 was disabled or corrupt.

The updated CPU test retries only B_INTERRUPTED against one fixed absolute
1 ms deadline, reports interruption counts, preserves other errors and caps
retries at 32. Production-body ASan/UBSan tests cover normal sleep, identical
retry deadlines, real errors, elapsed deadlines and a nonprogressing clock.
Fresh binary `pioneer_smp_smoke-interrupt-retry-v3` SHA:
`967c96a229744a715613157a4ded66dac4fbbd125fac06d2dd3f63fbee27d089`.
All 24 hardware repetitions PASSED, mask ffffffffffffffff each time; CPU 61
and CPU 58 each reported one handled interruption. No kernel change was
needed for this userspace test correction. The old failing result is retained.

At about 02:51 UTC, the existing pinned P100A profile passed all six read-only
topology stages on full-memory/all-core Haiku. The previously audited health
transaction then returned telemetry/ARC/PCI state 1 and all restored flags 1.
Firmware remains 12050001 (18.5.0, suffix 01), enabled GDDR f7, status 5515,
reported uncorrected errors zero, ASIC 47.20 C and fan 1,882 RPM. Board/limit
zero values remain unvalidated, not claims of healthy zero readings. PCI and
TLB state were restored, card/root commands 0000/0000, and the independent
profile passed again. No core start, reset write, DMA, firmware message or
flash occurred. Accelerator inference remains unimplemented. The normal
kernel was rebuilt after a comment-only synchronization and remains 3bdfd5dd...

Offline CPU-only Qwen3-0.6B-Q8_0 benchmark completed with 4, 32 and 64 threads,
prompt 32/generation 8, one repetition, no warmup, poll=0, GPU layers=0.
Model SHA remains 9465e63a...; backend explicitly CPU, GPU field empty. Tiny
prompt/generation rates were respectively 3.17/2.98, 17.30/8.12 and
17.24/1.18 tokens/s. This is a short host functional baseline, not robust
performance measurement or P100A inference; 64-thread generation was slower.
The benchmark's internal timestamps were in 1970, so they are not valid wall
times; external Mac UTC is used for this record. Afterwards, all 32 VM remap
rounds passed again on all 126 workers, and pinned SSH/system totals remained
available more than ten minutes after boot. Longer stability and physical
desktop/input confirmation are still outstanding.

Continued P100A startup audit against the pinned official ISA documentation;
see TENSTORRENT_PORT.md. Added a host-tested, offline-only fixed BRISC checksum
image and independent instruction/data-bounds decoder. It is not connected
to any driver operation and has not run on the card. No kernel module, reset
controls, instructions, firmware or DMA state changed during this audit.

The user confirmed the revision-6 Haiku desktop and mouse/keyboard work.
Pinned SSH at about 03:47 UTC October 15 (October 14 locally) still reported
64 CPUs, 137,034,936,320 bytes usable and a passing all-core CPU smoke,
more than an hour after boot. The experimental idle workaround is still in
use; the ordinary installed/default boot has not been replaced.

Implemented a separate root-only, fixed-image BRISC checksum operation in
the optional probe, plus read-only loaded-module metadata query. No general
code upload, coordinates or addresses are accepted. The existing firmware,
harvested-tile and stopped-core gates run first. Two strict bidirectional,
dynamic-VC TLBs cover only the fixed L1 footprint and core CSRs. Only BRISC's
reset bit can change; the other four RISCs stay held. Completion requires
528/PION results and repeated park-PC observations, then repeated stopped
reset-state checks before restoring the saved 22 L1 words. Local BRISC
RAM/registers may reset; this does not claim arbitrary RISC context restore.

Any unknown core activity, completion timeout, uncertain stop or failed
rollback refuses success. After uncertain execution the image stays intact:
never rewrite potentially live instructions. A semaphore-protected cold-
recovery latch denies all later probe operations, including queued callers;
driver reload is explicitly not a substitute for cold recovery. Cleanup
never retries a start/stop or code restoration after this refusal. All
original L1 values stay kernel-local and are not returned/logged.

Host ASan/UBSan tests pass for every setup/restore TLB write loss, all 44
image-install/restore write losses, both reset write losses, timeout, wrong
checksum/PC, permanent restore failure and persistent/transient core-state
changes. The actual assembled RV32I bytes match the driver's fixed image and
pass independent instruction/data-bounds decoding. All seven existing
PCI/ARC/telemetry/reset/L1 policy suites pass. Integrated optional module and
all nine CLIs cross-build. One initial build invocation used unsupported Jam
-C and the first CLI compile exposed a nested-result reference typo; both
were corrected before deployment. Optional module SHA:
`58ffcadcdc3e9ae0804b75ddda17ad55b06da97613a9d8b5b51a1b5b64107174`.
BRISC CLI SHA:
`d8c88121d0b9c194ba4b0c8f7ebdd1629e41e093e4584c85cfbdc7a6a8889c70`.
Neither is deployed yet; installed trace module remains 0d094653... and no
BRISC instruction/start/reset-write result exists yet. No DMA or flash.

Deployed only the optional module after saving/verifying trace module
0d094653... as `brisc-exec-1/tenstorrent_pci_probe.before-brisc-1`. The first
read-only support query returned Invalid Argument from the still-loaded old
module. After the asynchronous devfs reload, it positively returned v1,
19 code/22 footprint words and the expected code/result/park addresses; no
core operation had run during that check. All prior profile, telemetry,
held-core and reversible L1 checks then passed with the newly loaded module.

At about 04:01 UTC October 15 (October 14 locally), serial boundary 13260172,
the first actual fixed BRISC execution PASSED: checksum 528, completion PION,
both PC samples 0x70048, reset 0x47800 -> 0x47000 -> 0x47800. It completed on
the first poll, started/completed/stopped flags all 1, L1/TLB/PCI restored all
1, card/root commands 0000/0000, cold-recovery flag zero. This is real scalar
RISC execution on the P100A, not a CPU-host simulation or Tensix matrix/LLM
inference result. Only one core on the sole whitelisted tile was released.

Independent health and held-core checks then passed: ARC heartbeat advanced,
GDDR reported errors remained zero, ASIC about 48.10 C, fan 1,882 RPM and all
five RISCs held. A second explicit execution passed identically, taking five
polls. The subsequent independent topology profile passed stage 6 with the
original disabled windows/commands; the host all-core smoke passed and memory
maximum remained 137,034,936,320 bytes. No relay/reboot/SD action was needed.
No DMA, ARC firmware message, firmware flash or GDDR compute/memory test has
been run. Next gates are real accelerator math, model-memory transport and a
native runtime/backend; this does not yet make llama.cpp use the P100A.

Started the next read-only source audit, not a GDDR transaction. The active
versioned Blackhole HAL reserves a 64-byte DRAM barrier at zero and optional
profiler space before its allocator's unreserved range. Its older standalone
dram_address_map.h is not sufficient to select the live allocator boundary.
Pinned UMD derives the DRAM harvest mask directly as (~ENABLED_GDDR)&0xff,
so this card's f7 means bank 3 harvested. With NoC translation enabled, its
coordinate manager maps logical bank 0/port 0 to translated (18,12), not the
Tensix coordinate used above. The physical bank-0 port is (0,0). A future
GDDR test must validate that routing and reserved-memory ownership before
touching data. The UMD's host DRAM barrier uses 32-bit PCIe reads/writes;
device-issued NoC DRAM alignment is separately 64-byte read/16-byte write.
No GDDR data reads/writes, capacity test or memory-probe operation were added
or run in this checkpoint. The comment-only module rebuild remains exactly
58ffcadc... and loaded support query/host memory checks still pass.

## October 14: native read-only GDDR route verified

The new fixed bank-0 probe proves the PCIe source translation table maps
translated (18,12) to physical (0,0), then checks the target's physical NoC
coordinates, DRAM/bank-0 endpoint type and repeated NIU/label samples.
The first live attempt safely restored PCI/TLB state but refused the retained
physical/power-on label NOC_ID_LOGICAL. The host-tested v2 policy recognizes
firmware 18.5's observed zero bank-0 label without relaxing physical identity or source
routing proof. At about 21:28 local October 14 it passed three identical
samples, TLB/PCI restored 1/1, commands 0000/0000, cold recovery zero.
Installed module d999e495... and CLI 4f7b46c2... are retained in
/boot/home/pioneer-p100a-20261014/dram-route-2 with the prior module backup.
All earlier CLI hashes are unchanged. This is not GDDR payload testing,
matrix computation or P100A model inference. Kernel/default boot unchanged.

## October 14: high-address GDDR samples on all seven banks passed twice

The seven-bank route check now verifies all enabled bank endpoints against
the measured source translation tables, rather than assuming the newer
UMD's mirror-bank ordering. Firmware's NIU labels are partially configured:
west ports retain physical labels, east ports use translated labels. The
first all-bank read-only attempt refused this difference before payload
access; v2 requires either the correct physical or correct translated label.

At about 21:39 local October 14, the explicit reversible payload test passed
twice: seven routes, all 120 compute tiles held before and before restore,
five distinct 64-byte regions per bank (64 MiB/1 GiB/2 GiB/3 GiB/0xfe000000),
35 saved/modified regions, two patterns, zero mismatches. Original memory
remained kernel-local; data/TLB/PCI restored 1/1/1, commands 0000/0000,
recovery zero. ASIC 48.48 C, fan 1,882 RPM, reported uncorrected GDDR errors
zero, firmware 18.5 unchanged. Haiku remained reachable with 64 CPUs and
137,034,936,320 bytes maximum memory. Installed module b3d9b69d... and CLI
0035dffe... are retained with a backup in pioneer-p100a-20261014/dram-samples-2.
This is 2,240 bytes of high-address sample coverage, not full-capacity RAM
testing, Tensix matrix compute, DMA or accelerator LLM inference. No boot
changes, firmware changes, card swaps or recovery were needed.

## October 14: fixed TRISC0 matrix trial stopped safely but did not complete

The original fixed RV32I image and guarded single-tile driver passed offline
functional/MMIO-bound checks and policy fault tests. At about 21:58 local,
the first hardware attempt started TRISC0 but timed out before fixture 0
completed: phase1/fixtures0/marker0 after 2,000 polls. The guard stopped and
reset the tile, but correctly did not restore L1 after uncertain execution;
recovery was latched. TLB restore1, outer PCI state5/restored0, commands0/0.
This is NOT a passing matrix test or accelerator inference. Haiku remained
reachable with 64 CPUs/full RAM. Requested sync/shutdown over pinned SSH;
no further card operation or module reload is permitted until cold recovery.
Module 693b2a61... and CLI 1898e050... are retained with the GDDR module backup
in pioneer-p100a-20261014/matmul-1. Boot/kernel/firmware defaults unchanged.
