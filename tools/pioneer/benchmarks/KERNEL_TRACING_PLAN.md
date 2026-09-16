# Kernel diagnostics foundation

## 2026-09-16: bounded boot-stage recorder (not deployed)

The existing main2 recorder had 16 slots but the successful SMP path records
17 stages. It silently omitted MC (secondary CPUs enabled). The summary was
also emitted before MC. Each stage synchronously printed to serial.

The first patch introduces `BootStageLog<64>`, a fixed-storage, single-writer
container with explicit overflow accounting. Recording stores static strings
and monotonic timestamps without allocation or output. The end timestamp is
the last observed stage, not the time a later dump is requested. The summary
now follows secondary CPU enablement and also runs if launch_daemon loading
fails. This does not claim the desktop is ready: the measured interval begins
at main2, not firmware entry, and ends before user-space startup completes.

`bootstages` dumps retained records in the kernel debugger, once debug module
initialization has registered the command. Earlier boot failures still rely on
existing early checkpoints. All other serial logging remains unchanged. The
normal end-of-main2 summary still prints once; this is not a global quiet mode.

The container is not a concurrent runtime trace API. Only main2 writes it;
readers must run after that writer or with it stopped in the debugger. Entry
count is published after record initialization. A reset must not race readers.
No pointers to transient strings, user memory, or dynamically allocated data
are stored.

### Validation

Host tests pass with AddressSanitizer and UndefinedBehaviorSanitizer:

```sh
clang++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -I headers/private/kernel tools/pioneer/benchmarks/boot_stage_log_test.cpp \
  -o /private/tmp/verse-boot-stage-log-test
/private/tmp/verse-boot-stage-log-test
```

The test also cross-compiles to a RISC-V object with the existing GCC 13.3.0
toolchain. This tests the container, not main.cpp integration or a running
kernel. Full kernel compilation and linking subsequently passed using the
existing build tree at `/Volumes/HaikuBuildLocal`. Only main.o, kernel_core.o,
and kernel_riscv64 were rebuilt. Jam was restored from official haiku/buildtools
revision `8375c2dbeaf109c520798cb234d57f0895463201` in the separate temporary
directory `/private/tmp/verse-haiku-buildtools-20260916`; the old configured
Jam path remains absent. Existing platform/package and linker orphan-section
warnings remain; there were no compiler errors in the modified code.
At the initial compile checkpoint no deployed files had changed; subsequent
candidate packaging and SD deployment are tracked below. Samsung is untouched.

### Candidate packaging and rollback (2026-09-16)

Kernel build SHA-256:
`693794818248a33e3df9d7a0486639a645925a89966b480811559c13426c71d0`.
The local candidate is based on the current SD filesystem, not a fresh image,
to preserve installed assistant files, SSH setup, and settings. Only
`kernel_riscv64` was replaced in the existing haiku system package. Package
entry metadata comparison showed only that entry changed. Extracted kernel
hash matches the build; package readback from the candidate matches its source.

Linux staging/rollback directory:
`/mnt/ssd/haiku-deploy/boot-tracing-20260916`.

- Full 1 GiB partition backup: `sd-p2-before.img`, SHA-256
  `979234c2ee8d2b61f606e9b141edda46978535bf85fcc4e102a0d665a36790c4`.
- Original 300 MiB filesystem: `sd-bfs-before.img`, SHA-256
  `c42dad8ea86a3da67ad6f77ab15dbb7933e43db2e8f94a3a9440faaa5de02165`.
- Candidate payload: `sd-bfs-tracing.img`, SHA-256
  `0107e78bc06887ec2c2f581e274a3982f698234d71bf80e4c333fbaffc3a74f3`.
- Candidate system package SHA-256:
  `0cd78dfc023d8414a1c3cdc1a684cdbf53cd01b04b21209d90acbf5db8b809f5`.

Deployment target verified as `/dev/mmcblk1p2`, on the 29.2 GiB SD with
HAIKUPNR FAT partition and 300 MiB BFS filesystem inside a 1 GiB partition.
Samsung is `/dev/sda1`, not mounted or written. Linux root is nearly full;
staging uses `/mnt/ssd` on the separate R3SL disk. Restore only after identifying
the same SD again; use the deployment script with `sd-bfs-before.img` and its
recorded checksum for a filesystem rollback. Firmware/FAT is not part of this
update. Runtime validation remains pending.

SD deployment completed successfully; complete 300 MiB device readback matches
candidate SHA-256 `0107e78bc06887ec2c2f581e274a3982f698234d71bf80e4c333fbaffc3a74f3`.
The standard deployment script also created and checked the compressed backup
`haiku-pioneer-bfs-before-20260916T170900Z.img.gz` in the staging directory.
The system package, not the loader or firmware, was changed. Linux was then
asked to shut down using `shutdown -h now` for the first hardware test.

### First successful runtime capture, 2026-09-16

After earlier power-on attempts produced no readable UART output, Linux booted
with the SD removed. Read-only SD inspection verified the candidate package,
recorded EFI loader and firmware hashes. FAT check reported only the dirty bit
and a one-byte primary/backup boot-sector difference; no repair was performed.
After cleanly unmounting both SD partitions, shutting down Linux, waiting ten
seconds and issuing power-on, UEFI and Haiku output resumed. This sequence does
not establish the cause of the preceding no-output attempts.

Serial evidence in `/private/tmp/pioneer-serial-check-20260916/screenlog.0`:

```text
P202:BOOT stages=17 start=84027815us end=111891762us elapsed=27863947us dropped=0
P202:BOOT[16] MC secondary CPUs enabled after main2 at=111891762us delta=157678us
```

All 17 records were retained, including MC, with zero drops. The measured main2
interval was 27.864 seconds. Largest intervals: boot-filesystem mount 12.726 s,
main2 entry through device-manager readiness 6.773 s, launch_daemon image load
5.150 s, device-module initialization 2.930 s. These are inclusive wall-clock
intervals, not isolated CPU costs or full power-on-to-desktop time. User-space
threads subsequently started; desktop/input and debugger-command checks remain
pending. No speedup claim is supported by this single capture.

### User-space startup instrumentation candidate

User confirmed desktop, mouse, keyboard, and normal responsiveness. SSH also
verified `/boot/system/kernel_riscv64` matches `693794818248a33e3df9d7a0486639a645925a89966b480811559c13426c71d0`.

Added a private stack-local `StartupTiming` collector (16 records, explicit
dropped count). Each Mark stores a static label and monotonic system_time;
Flush writes tagged BOOT_TIMING v=1 records to syslog only after measured work.
The kernel is unchanged by this second candidate. Instrumented boundaries:

- launch_daemon, system/user distinguished: environment initialization,
  settings parsing, job initialization, job submission, callback completion.
- Tracker ReadyToRun: MIME setup, defaults, watcher setup, callback completion.
- Deskbar constructor body: settings, window construction, app enumeration,
  Show return, constructor completion; separate ReadyToRun callback marker.

These do not assert service readiness or first-frame presentation. Base-class
construction is outside the constructor-body interval. Service launch events
already exist in `launch_roster log`; launching is not readiness. Startup
records carry team IDs and start timestamps to distinguish restarts. Syslog
output may itself cost time outside the measured sections and can delay later
work; it is not the final low-overhead runtime tracing transport. A hang inside
a section will not flush its partial stack-local buffer.

All three application targets compile/link. Tracker instrumentation is in
libtracker.so, so the package candidate includes that library as well as
Tracker, Deskbar, and launch_daemon. No live desktop binary was replaced.
Host tests pass with ASan/UBSan for deferred logging, timestamp retention,
delta calculation, overflow (18 marks/16 slots), and empty logs:

```sh
clang++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -I tools/pioneer/benchmarks/startup_timing_test_support \
  -I headers/private/shared tools/pioneer/benchmarks/startup_timing_test.cpp \
  -o /private/tmp/verse-startup-timing-test
/private/tmp/verse-startup-timing-test
```

After a test boot, collect `grep BOOT_TIMING /boot/system/var/log/syslog` and
`launch_roster log`. Verify expected components, increasing timestamps within
each section, zero drops, and normal desktop/input. Compare same-boot monotonic
values, not calendar timestamps (the machine's wall clock remains unreliable).
The bootstages debugger command remains untested: deliberately entering KDL
pauses the machine, so schedule that check only after the user saves work.

Candidate package: `/private/tmp/verse-user-startup-20260916/haiku-startup.hpkg`,
SHA-256 `c7e0d87fcd3c15c284c320586b92455359e4341ab5514474a7ad0aee034f3d62`.
Its complete path inventory matches the previous package. Extracted
`servers/launch_daemon`, `Tracker`, `Deskbar`, and `lib/libtracker.so` match the
new build files byte-for-byte; extracted kernel matches the already-tested
tracing kernel. Note Tracker and Deskbar are package-root entries, not apps/.
Candidate is local only, not installed. Reboot/deployment and deliberate KDL
entry are awaiting the user saving work.

The retained-log debugger check subsequently passed on the running SD instance:
`kernel_debugger "bootstages validation"` entered KDL on CPU 59; `bootstages`
returned the same 17 records, unchanged start/end timestamps, and zero drops.
`continue` returned successfully to the SSH command (exit 0). The user-space
candidate remains undeployed. Direct package replacement was deferred because
the SD BFS volume has only 34.1 MiB free for a roughly 33.1 MiB candidate;
the agreed alternative is a backed-up Linux-side update.

### Next checkpoints

#### Expanded-SD boot measurement, 2026-09-16

The startup-instrumented package booted successfully from the expanded 4 GiB
SD filesystem; user confirmed Haiku was up and SSH verified the unchanged
kernel hash. Serial captured all 17 kernel records, zero dropped, start
84298276 us, end 112611237 us, elapsed 28312961 us (28.313 s).
Main inclusive intervals: device manager 6.975 s, boot filesystem 12.822 s,
device modules 2.869 s, launch_daemon image load 5.304 s. This is one run,
not evidence of an improvement or regression against the prior 27.864 s run.

Syslog contains monotonic, zero-drop records for:

- User launch_daemon (team 126): start 118811820 us, callback completion
  122155752 us; measured interval 3.344 s, including 3.286 s after job submission.
- Deskbar constructor (team 197): start 143482622 us, complete 152540288 us;
  9.058 s total, including 8.521 s between settings and window construction.
  ReadyToRun completion at 152540739 us.
- Tracker ReadyToRun (team 190): start 152291501 us, complete 152577108 us;
  0.286 s total.

These are callback boundaries, not first visible-frame timings. No system
launch_daemon BOOT_TIMING records were found in current syslog or serial, and
syslog.old was absent. Early logging availability is a hypothesis to inspect,
not a confirmed cause; system launch timing validation remains incomplete.
No scheduler, graphics or driver changes were made in this validation step.

#### Read-only follow-up: startup delay breakdown

User confirmed desktop, mouse and keyboard work normally. Existing serial
substage timestamps from this boot narrow the 12.822 s M2-to-M3 interval:

- Boot partition discovery B0-to-B1: 6.213 s. Within it, initial devfs scan
  I0-to-I1 consumed 5.510 s, SD partition scanning 70 ms and Samsung scanning
  204 ms. The trace includes AHCI/NVMe initialization; it is not a measurement
  solely of SD throughput. No asynchronous discovery retry is shown in this
  boot's interval.
- BFS mount B1-to-B2: 45.692 ms.
- System packagefs B3-to-B4: 4.591 s; home packagefs B4-to-B5: 99.585 ms.
- Disk-system rescan B8-to-B9: 919.225 ms; monitoring setup B9-to-BA:
  878.778 ms, including another initial device scan.

Deskbar's 8.521 s window-construction interval includes the BWindow base
constructor, TBarView construction (including a temporary decorator-query
window), menus/tray and AddChild callbacks. Existing markers cannot attribute
the delay to any one of these, or separate I/O from IPC/scheduling waits.

The missing system launch_daemon records have a concrete source-level
explanation: syslog resolves its logger port via get_launch_data, which calls
get_launch_daemon_port. That helper explicitly rejects a request from the
system daemon's own main thread to prevent a self-wait. send_syslog_message
then returns without sending when the port lookup fails. Therefore the current
generic Flush transport is unsuitable for that caller; merely waiting for
syslog_daemon to launch is not a sufficient fix. Retain records for retrieval
or use a separately validated transport without self-RPC in a later patch.

Next measurement targets: devfs device publication, system package activation,
and Deskbar's nested window/view construction. Preserve timestamps in memory
and emit after the outer measured section, avoiding nested flush overhead in
the measured interval. No live binaries, services or kernel were changed by
this read-only investigation, and no speedup has yet been demonstrated.

1. Build-tool restoration and kernel compilation passed. Candidate and rollback
   hashes are recorded above; finish SD readback and hardware validation.
2. On hardware verify all 17 stages, MC in the summary, zero dropped records,
   usable desktop/USB/networking, and debugger dump after boot. Do not claim
   a speed improvement without matched repeated boots.
3. Investigate the existing system-profiler mapping fault separately. Do not
   rerun the previously crashing capture on the installed kernel or describe
   this boot-log patch as a fix for that fault.
4. Design runtime capture around preallocated per-CPU storage. Specify nested
   interrupt handling, publication, reader snapshots, CPU migration, buffer
   exhaustion, filtering, and lost-event reporting before adding hot-path
   hooks. Avoid allocation, formatting, serial I/O, and global locks there.
5. Validate disabled-path overhead, enabled-path cost, repeated enable/disable,
   concurrent export, overflow, and 64-CPU stress before using trace output
   to justify scheduler or memory-placement changes.

GPU, PCI, DMA, scheduler policy, and the old profiler are unchanged by this
first patch. Boot-stage deltas remain inclusive of intervening work and other
logging; they are not per-function CPU execution times.
