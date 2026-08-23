

1) Create and Deliver Value by solving human problems on open and available compute hardware in the new age of 
accelerated progress enabled by AI.

2) Survival as a project in the current age depends on speed of productivity and the availability of cloud and 
Local LLm models and harneses with distributed systems can enable OS development progress using collaborative resources. 

3) Workloads of the future will depend on AI and robotics, and the project will build a foundation that can support this with 
clean, effecient, fast, reliable and secure qualities to enable new achievents in quiality of life and advances in 
our understanding of the world.  


Current Plan:
1) Bring up boot and services on RiscV platforms.
2)

Port LLama.cpp
Port Tenstorent hardware support 

Integrate distributed OS development and testing tools to develop code clarity, compatibility, functionality, reliability and security. 


---

# Proposed Mission and Goals

## Mission

Build a fast, coherent, reliable, and approachable personal operating system
for the age of intelligent software.

This project extends Haiku's distinctive strengths: rapid startup, immediate
responsiveness, architectural clarity, efficient use of hardware, and a
consistent user experience. It should remain understandable as a complete
system rather than becoming an accumulation of disconnected components.

AI agents will be first-class participants in both the development and use of
the system. They will help measure performance, diagnose failures, improve
code, test hardware, maintain documentation, and propose architectural
refinements. Their work must be reproducible, reviewable, secure, and subject
to human control.

The system will run well on open and accessible hardware, beginning with
RISC-V, while providing an efficient foundation for local AI, accelerated
computing, automation, and robotics.

## Principles

### 1. Responsiveness is a feature

Optimize for the experience of the person using the system: fast startup,
low interaction latency, predictable scheduling, and graceful behavior under
load.

### 2. Architectural coherence over accumulation

Prefer small, understandable interfaces and well-integrated system
facilities. New components must justify their complexity and fit the
architecture as a whole.

### 3. Measure before optimizing

Performance and reliability work must be supported by reproducible
benchmarks, profiles, traces, and regression tests.

### 4. AI-assisted, human-governed development

Agents may analyze, implement, test, document, and propose improvements.
Material changes must remain auditable, reviewable, reproducible, and easy to
reverse.

### 5. Local-first intelligence

Make local models and agents useful without requiring cloud services. Network
services may extend the system, but core operation, privacy, and user control
must not depend on them.

### 6. Reliability through continuous verification

Test changes across supported machines and emulators. Treat boot failures,
crashes, data loss, performance regressions, and nondeterministic behavior as
measurable engineering problems.

### 7. Security through capability and least authority

Give agents and applications only the resources they require. Make
consequential actions visible, attributable, and controllable by the user.

### 8. Efficient computing

Use memory, storage, energy, and accelerator resources deliberately.
Efficiency is part of system quality, not an afterthought.

### 9. Open hardware and open development

Support hardware that can be documented, tested, repaired, and improved by
the community. Publish the tools, measurements, and decisions used to develop
the system.

### 10. Preserve the joy of a personal computer

The system should feel quick, cohesive, discoverable, and enjoyable—not
merely serve as infrastructure for agents.

## Initial Goals

### 1. Establish measurable system baselines

- Record boot time, service-start time, idle memory, application-launch
  latency, scheduler latency, power use, and crash rates.
- Add automated regression thresholds for supported reference machines.
- Retain benchmark results so performance changes can be tracked over time.

### 2. Make boot fast, observable, and dependable

- Trace the complete boot path and identify its critical path.
- Start independent services concurrently where correctness permits.
- Detect unnecessary initialization and blocking I/O.
- Produce useful diagnostics for every failed or delayed boot.
- Define explicit boot-time targets for each reference platform.

### 3. Build an agent-ready development loop

- Provide agents with structured build errors, test results, traces, crash
  dumps, API documentation, and architectural constraints.
- Automate isolated builds and tests for agent-generated patches.
- Require evidence—tests, benchmarks, or diagnostics—for proposed changes.
- Keep provenance for generated code and architectural proposals.

### 4. Improve reliability continuously

- Expand deterministic unit, integration, boot, stress, and fault-injection
  testing.
- Run continuous tests in emulators and on physical hardware.
- Automatically reduce crashes and failed tests into reproducible cases.
- Track mean time between failures and time to diagnose regressions.

### 5. Establish RISC-V as a reference architecture

- Reach repeatable boot on selected documented RISC-V platforms.
- Bring up storage, networking, graphics, USB, power management, and SMP in
  explicit milestones.
- Maintain emulator-based testing alongside physical-device testing.
- Avoid architecture-specific shortcuts in shared system design.

### 6. Support efficient local AI workloads

- Port and optimize llama.cpp as an initial inference runtime.
- Define native facilities for model discovery, accelerator selection,
  resource accounting, and cancellable background work.
- Add Tenstorrent support where hardware and documentation make it practical.
- Benchmark CPU and accelerator inference for latency, memory, energy, and
  responsiveness under concurrent desktop use.

### 7. Introduce a secure agent execution model

- Give every agent an identity, declared permissions, resource limits, and an
  activity log.
- Require explicit authorization for destructive or privacy-sensitive
  actions.
- Isolate untrusted models, tools, and generated programs.
- Make agent actions inspectable, interruptible, and reversible.

### 8. Improve architecture without losing coherence

- Maintain an architectural map of subsystems, ownership, dependencies, and
  performance-critical paths.
- Use agents to identify duplication, unclear interfaces, and unnecessary
  coupling.
- Accept refactoring only when it improves measured properties or materially
  reduces complexity.
- Record important design decisions and rejected alternatives.

## Near-Term Milestones

1. Select one emulated and one physical RISC-V reference platform.
2. Produce a reproducible boot image and publish its boot trace.
3. Establish performance and reliability dashboards.
4. Port llama.cpp with a minimal native example application.
5. Create an isolated agent-driven build, test, and patch-review pipeline.
6. Define the first agent permission and audit-log model.
7. Set numerical targets for boot time, application launch, idle memory, and
   continuous-test reliability.

## Continuous Improvement Policy

Continuously measure the system and use agents to propose, test, and explain
improvements under human-governed engineering controls.
