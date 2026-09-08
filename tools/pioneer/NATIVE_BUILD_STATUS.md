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
