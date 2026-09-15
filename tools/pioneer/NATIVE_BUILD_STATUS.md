# Native build status (2026-09-07)

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
