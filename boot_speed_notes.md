# Verse OS Boot Speed Notes

## Objective

Make Verse OS start quickly, consistently, and observably. Optimize for the
time until the computer becomes genuinely useful—not merely the moment a boot
logo or desktop background appears.

Boot performance work should follow a measurement-first process:

1. Record the current boot timeline.
2. Identify the critical path and sources of variance.
3. Describe real dependencies explicitly.
4. Run independent work concurrently where it is safe and beneficial.
5. Verify correctness, reliability, and performance on every change.

Fast boot must not come at the cost of unreliable initialization, hidden
failures, data corruption, or work deferred until it interrupts the user.

## Existing Foundation

Haiku already has pieces of the proposed design that Verse OS should build on:

- The kernel performs early initialization and starts `launch_daemon`.
- Kernel modules can declare module dependencies.
- `launch_daemon` jobs can declare requirements.
- Job initialization checks for cyclic dependencies.
- The launch system queues jobs with dependencies through `BJob` machinery.
- The launch worker pool scales with the detected CPU count and currently
  permits up to approximately three workers per CPU.

The first task is therefore to observe and validate the existing dependency
and scheduling behavior. Verse OS should extend it where boot remains
unnecessarily serialized rather than replace it without evidence.

Relevant starting points include:

- `src/system/kernel/main.cpp`
- `src/system/kernel/module.cpp`
- `src/servers/launch/Job.cpp`
- `src/servers/launch/Worker.cpp`
- `src/servers/launch/LaunchDaemon.cpp`

## Define the Boot Milestones

Record several user-relevant milestones instead of reporting one ambiguous
boot-time number:

- Firmware entry to bootloader start.
- Bootloader start to kernel entry.
- Kernel entry to scheduler availability.
- Boot volume mounted.
- `launch_daemon` started.
- Essential system services ready.
- Login or desktop process started.
- First desktop frame presented.
- Input accepted and processed.
- Core applications can launch without blocking on deferred boot work.
- Network usable, reported separately because it may depend on external
  infrastructure.

The primary target should be **time to responsive desktop**. Time to first
frame and time to full background readiness should remain separate metrics so
the system cannot appear fast by hiding blocking work behind the desktop.

## Boot Timeline Instrumentation

Introduce a low-overhead tracing facility that works across boot stages. Each
record should contain at least:

- A monotonic timestamp.
- Boot identifier.
- Stage and event name.
- Begin, end, ready, wait, failure, or retry event type.
- CPU and thread identifier when available.
- Parent operation or dependency identifier.
- Result status.
- Optional bytes, device, module, service, or resource metadata.

Use a fixed-size in-memory ring buffer during early boot so measurement does
not depend on the filesystem or allocate heavily. Transfer or expose the trace
after the system is ready. Serial logging can remain available for debugging,
but synchronous console output should not be the primary profiler because it
can substantially change timing.

Support at least two modes:

- **Baseline mode:** Minimal events, always enabled, suitable for regression
  tracking.
- **Diagnostic mode:** Detailed spans, scheduling events, dependency waits,
  I/O, module loading, and device probing.

Instrumentation overhead must itself be measured.

## Dependency Graph

Represent boot work as a directed acyclic graph where practical:

```text
boot task A ──requires──▶ boot task B
```

A task becomes runnable only after all hard dependencies have reached their
declared readiness state. Independent runnable tasks may execute concurrently.

The graph should distinguish several relationships:

- **Hard dependency:** The task cannot start safely without another task.
- **Readiness dependency:** The task may be spawned but cannot advertise
  itself as ready until a condition is satisfied.
- **Ordering constraint:** Tasks share state or hardware and must be ordered.
- **Resource constraint:** Tasks are independent but contend for a scarce
  resource such as a storage device.
- **Optional dependency:** Improves functionality but must not block core boot.
- **Event trigger:** Starts work when a device, volume, network, or user event
  occurs.

Avoid inferring dependencies solely from historical startup order. Every edge
should state the actual invariant it protects. False dependencies serialize
boot unnecessarily; missing dependencies create nondeterministic failures.

The tooling should detect and report:

- Dependency cycles.
- Missing dependencies.
- Tasks that wait for undeclared resources.
- Edges that never affect scheduling.
- Repeated retries or timeouts.
- Tasks dominating the critical path.
- Excessive fan-in and initialization bottlenecks.

## Parallel Boot Scheduling

When more than one graph node is ready, schedule independent work across
available CPUs. The scheduler should be critical-path-aware rather than merely
starting everything at once.

Useful scheduling inputs include:

- Historical duration of each task on the current hardware class.
- Whether a task lies on the predicted critical path.
- CPU, memory, storage, device, and locking requirements.
- Whether the task is latency-sensitive or deferrable.
- Observed contention and cache effects.

More concurrency is not automatically faster. Unbounded parallel module and
service startup can produce storage contention, memory pressure, lock
contention, poor cache locality, nondeterministic bugs, and slower boot on
small systems. Worker limits should be hardware-aware and supported by
measurements.

## Kernel and Module Initialization

Kernel initialization requires stricter treatment than userland service
startup. Early boot code may assume single-threaded execution, depend on
global initialization order, access hardware with ordering requirements, or
run before normal synchronization facilities are available.

Potential work plan:

1. Instrument every major kernel initialization phase.
2. Document existing ordering assumptions and module dependencies.
3. Separate pure preparation from state-changing activation.
4. Mark functions as serial-only, parallel-safe, or conditionally parallel.
5. Begin parallel execution only after the scheduler, allocator, locking,
   timers, and required CPU facilities are demonstrably ready.
6. Start with independent, read-mostly, or device-specific probes.
7. Add stress, race, deadlock, and repeated-boot testing before expanding the
   parallel region.

Module dependency declarations are a useful input, but they do not prove that
module initialization is thread-safe. Parallelism must be introduced through
an explicit contract and verified incrementally.

## Device Discovery

Hardware discovery can dominate boot and often contains timeouts. Track:

- Time spent enumerating each bus.
- Driver and device probe duration.
- Firmware loading and device reset time.
- Storage discovery and filesystem-mount latency.
- Retries, sleeps, and timeout expiration.
- Devices not required for a responsive desktop.

Probe independent buses or devices concurrently when their controllers and
drivers permit it. Cache stable discovery information only when invalidation
is correct and a safe full-probe fallback exists.

## Deferred and On-Demand Work

Work that is unnecessary for the initial user experience may be triggered on
demand or after the responsive-desktop milestone. Deferral is appropriate only
when:

- The first consumer can activate the facility reliably.
- Activation latency is bounded and visible in measurements.
- Failures are reported clearly.
- The work cannot unexpectedly consume resources during interaction.
- Security policy is active before less-trusted work begins.

Do not defer required work merely to improve the headline boot number.

## Reliability and Verification

Every boot optimization should be evaluated for correctness and variance, not
only best-case speed. Automated testing should include:

- Cold and warm boot where the platform allows meaningful distinction.
- Hundreds or thousands of repeated boots in emulators.
- Repeated boots on physical reference machines.
- Single-core and multi-core configurations.
- Slow and fast storage.
- Low-memory conditions.
- Missing, slow, failing, and hot-plugged devices.
- Dependency failure and timeout injection.
- Race, deadlock, and scheduler stress testing.
- Clean rollback to the previous boot implementation.

Track median, 90th, 95th, and 99th percentile boot times. A change that lowers
the median but creates rare stalls or boot failures is a regression.

## Regression Dashboard

For each reference platform, retain:

- Total time for every defined milestone.
- Critical-path tasks and their durations.
- CPU utilization and parallelism over time.
- Storage I/O and wait time.
- Peak memory used during boot.
- Task failures, retries, and timeouts.
- Trace version, build revision, configuration, and hardware identity.

Set regression thresholds only after representative baselines exist. Compare
like-for-like configurations and preserve raw traces for investigation.

## AI-Assisted Optimization

Agents may use boot traces and dependency graphs to:

- Identify critical-path changes and timing regressions.
- Suggest missing or unnecessary dependencies.
- Locate long serial regions and resource contention.
- Generate targeted instrumentation and reproducible tests.
- Compare behavior across hardware classes.
- Propose refactoring supported by measured evidence.

Agent proposals must include the affected invariant, before-and-after traces,
reliability results, and rollback plan. Agents should not autonomously alter
boot dependencies or promote boot changes without governed review.

## Initial Milestones

1. Define the responsive-desktop milestone and measurement protocol.
2. Add a shared monotonic boot-event format and early ring buffer.
3. Produce a complete trace from bootloader through responsive desktop.
4. Export the existing `launch_daemon` job requirements as a graph.
5. Overlay task durations and dependency waits on that graph.
6. Identify the current critical path on one emulated and one physical
   reference platform.
7. Verify how much parallelism `launch_daemon` currently achieves in practice.
8. Remove or correct one measured userland serialization bottleneck.
9. Instrument kernel and module initialization before attempting parallel
   refactoring.
10. Establish repeated-boot reliability and performance regression tests.

## Guiding Rule

> Measure the complete boot, optimize the critical path, parallelize only
> proven-independent work, and count the system as started when it is truly
> responsive.
