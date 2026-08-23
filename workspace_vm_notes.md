# Verse OS Workspace VM Notes

## Concept

A Workspace VM is a persistent or disposable virtual environment whose
applications appear as part of the regular Verse OS desktop while their
processes, filesystem, services, and resources remain isolated inside a
virtual machine.

Virtualization allows existing applications to gain strong isolation without
requiring each application to adopt a new security API. Safe integration still
requires a small, carefully controlled interface between the trusted desktop
and each guest. Virtualization moves the security boundary into a central
host–guest interface; it does not eliminate that boundary.

## Proposed Architecture

```text
Trusted Verse OS desktop
├── Window and input broker
├── File-selection broker
├── Clipboard broker
├── Network policy
├── Device-permission broker
├── Resource and lifecycle manager
│
├── Personal workspace VM
│   └── Related applications and private data
├── Development workspace VM
│   └── Compiler, agents, source, and tests
├── Untrusted-task microVM
│   └── One disposable agent, build, or test job
└── Legacy workspace VM
    └── Applications requiring broad compatibility
```

From the user's perspective, guest applications should behave like ordinary
windows on the Verse OS desktop. Workspace boundaries and permissions should
remain visible and understandable without making routine interaction
cumbersome.

## Execution Models

- **Trusted native applications:** Run directly on Verse OS for maximum
  performance and system integration.
- **Workspace VMs:** Persistent environments containing related applications,
  services, and data.
- **Disposable microVMs:** Short-lived environments for generated code,
  downloads, builds, tests, and agent jobs.
- **Compatibility VMs:** Whole guest systems for running unchanged application
  stacks from Verse OS or other operating systems.

The workspace should normally be the isolation unit. A separate VM for every
application would provide finer isolation but impose greater memory, startup,
management, and integration costs.

## Minimal Host–Guest Interface

The trusted host should expose a small, auditable set of services:

- Display surfaces without unrestricted compositor control.
- Sanitized keyboard and pointer events.
- User-selected files instead of automatic home-directory sharing.
- Explicit clipboard operations that can be restricted or disabled.
- Mediated network access with per-workspace rules.
- Virtual audio and narrowly granted camera and microphone access.
- Notifications and controlled drag-and-drop.
- Resource quotas and VM lifecycle controls.

For example, when a guest application requests a file, the trusted Verse OS
host should display the file picker. The guest receives access only to the
selected file, preferably through a temporary capability, rather than access
to the user's entire home directory.

## Security Model

Hardware-assisted virtualization should provide the primary isolation boundary
between workspaces. The system must nevertheless treat the following as part
of its trusted computing base:

- The hypervisor and hardware-virtualization support.
- VM creation, configuration, update, and lifecycle management.
- Virtual device implementations.
- Desktop integration brokers.
- Firmware and relevant hardware mechanisms.
- Any physical device passed directly into a guest.

Virtual devices parse guest-controlled input and are therefore security-
critical. GPU acceleration, shared folders, clipboard synchronization, USB
passthrough, and host–guest networking all increase the attack surface. They
should be minimal, separately permissioned, and tested adversarially.

A VM is not automatically secure merely because it is virtualized. Verse OS
must secure the hypervisor, isolate VM management, protect images and
snapshots, segment virtual networks, limit resources, and maintain prompt
security updates.

## AI Development and Verification

Workspace VMs can form the execution foundation of the cooperative Verse OS
development network:

1. Start each contributed job from a signed, immutable image.
2. Run untrusted inputs, generated code, builds, and tests in a disposable VM.
3. Provide no credentials or private user data by default.
4. Delegate specific resources through narrow, time-limited capabilities.
5. Export results through a restricted and validated output channel.
6. Reproduce important results on independent workers.
7. Destroy disposable VM state after verification unless retention is
   explicitly authorized.

Persistent development workspaces may contain source trees and developer
tools, but generated or externally supplied code should still be tested in a
nested disposable environment or a separate microVM.

## Product and Engineering Costs

- Additional memory and storage for persistent workspaces.
- Startup overhead unless lightweight guests, snapshots, or prewarmed VMs are
  used.
- Difficult GPU acceleration and safe device sharing.
- More complex suspend, migration, backup, and system upgrades.
- Potential fragmentation if every workspace becomes an independently managed
  miniature system.
- Significant kernel, hypervisor, driver, desktop, and tooling work.

These costs should be measured against concrete threat models and usability
goals. Isolation that users routinely disable because it is slow or confusing
does not provide meaningful protection.

## Recommended Direction

Verse OS should use a hybrid model:

> A small trusted desktop, persistent Workspace VMs for related applications,
> and disposable microVMs for agents and untrusted computation.

The product promise should be:

> Existing applications can gain strong isolation without modification, while
> a small host-controlled capability interface provides safe desktop
> integration.

The goal is a fast, cohesive desktop on the surface with strong, visible trust
boundaries underneath. Workspace VMs should make isolation understandable to
people while providing safe execution environments for continuous AI-assisted
development.

## References

- [NIST SP 800-125: Guide to Security for Full Virtualization Technologies](https://csrc.nist.gov/pubs/sp/800/125/final)
- [NIST SP 800-125A Rev. 1: Security Recommendations for Server-based Hypervisor Platforms](https://csrc.nist.gov/pubs/sp/800/125/a/r1/final)
- [NIST SP 800-125B: Secure Virtual Network Configuration for VM Protection](https://csrc.nist.gov/pubs/sp/800/125/b/final)
