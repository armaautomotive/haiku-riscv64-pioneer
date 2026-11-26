# Isolated ECAM lease qualification

The recorded ECAM controller `23e65e2a...` was reproduced byte for byte
from the saved Jam link action and objects. Three builds using the recorded
Jam actions and the pinned qualification sources produced candidate ECAM
`bdaafd15529776acb04dd9f0b3acda39138a1faf093dd64c34f1c1cc92c6b5ab`.
The candidate may initialize, but its general recovery callback still returns
`B_NOT_ALLOWED`. The only newly callable qualification service invokes a fixed
no-op callback within the controller lease. Its source, failure cases, lease
exclusion and blocked-reader wakeup passed focused simulated tests. The general
recovery gateway and the original witnesses remain non-loadable.

A private BFS image made from the recorded private package contains the
already-booted pin-service kernel and the ECAM candidate. Its SHA-256 is
`ed37c2b13f940dad5561553b88c5acdaf3bdda60cb83c87d7e5557fb042cf79f`.
Package paths, candidate entry hashes, BFS check and package roundtrip passed.
Full local extraction found 5,857 entries in both packages, with file contents,
symlink targets and POSIX modes differing only at the intended kernel and ECAM
paths. Haiku-specific package attributes were not compared. A separate helper
with hash `39e08ba817707a74d30d26f6c34a9ad8f0095489745734962fafac80e5647539`
pins this image and preserves the existing control profile. Nothing has been
uploaded, kexec-loaded or booted for this ECAM candidate.

The isolated probe and client cross-built with hashes `6741d1fb...` and
`02b86c88...`. On the running pin-service kernel and original ECAM, the guarded
trial checked stable driver55 generation 1 idle and the absence of diagnostic
processes, took an exclusive lock, and ran a read-only PCI/FDT profile. It
found one card and profile stage 6, then observed the fixed-probe module was
absent as expected. The temporary driver, device, lock and files were removed,
and stable status remained idle. No PCI configuration writes, accelerator
execution, DMA, firmware operation, model inference or reset were performed.

```text
ECAM_LEASE_PROBE version=1 manager=0 pci=0 cards=1 profile=-2147459069 stage=6 ready=1 lease_module=-2147459069 lease=-2147483635 callback=-2147483635 calls=0 recovery_module=-2147483635 recovery=-2147483635 recovery_calls=0
PASS fixed ECAM lease absent and cleanup; no PCI writes or accelerator execution
```

This negative control does not establish that the candidate boots or that the
lease works on real hardware. The full reload regression is running separately;
repeat the 27B/idle preflight immediately before any card access. Device
removal, firmware/MMIO/DMA exclusion, controller lock ordering, no-reboot
recovery, full model output and decode throughput remain unverified.

## Idle-path refinement (supersedes the candidate above)

With the controller mutex held, an inactive lease now returns before looking
up the caller thread. The focused controller test covers the idle path with an
invalid thread ID as well as active lease and waiter behavior. Two independent
Jam-action builds reproduced the control and produced ECAM SHA-256
`138f97383da9db05288ab451621442c58d7733d6fd8f389c9156eccc658a8757`.
The rebuilt private BFS is `c4f7223d4f7542068b7c58ecaf4aca9f9a1321ae92ed042898af0d47e96d47f3`,
and its separate helper is `03a95c8d60741677c738c371ff4b37aa6f9064d4e4671ec3d2ab068c2ddabe34`.
The candidate remains unbooted. The earlier full regression overlapped the
refinement; the focused test and new witness build cover the final change.

## Live fixed-lease qualification

The isolated helper verified its staged inputs and kexec-booted the refined
BFS. Haiku came up with 64 host CPUs and stable55 generation 1 idle. The
loaded ECAM image reported 68 exported symbol tuples, all matching the
qualification candidate and none matching the saved control; this is symbol
layout evidence, not a full binary hash (`symbol-match.json`).

After idle and process checks the guarded probe reached PCI profile stage 6,
ran the internal fixed no-op lease callback once, and verified that the general
recovery call returned `B_NOT_ALLOWED` without invoking its callback. Full
output is in `live-probe.txt`. The temporary driver, device, lock and stage
were removed. Both model files retained their recorded sizes, not fresh hashes.
There were no PCI configuration writes, accelerator execution, firmware
commands, DMA or reset operations in this test. The result does not prove
concurrent physical config exclusion, device-removal behavior, DMA/MMIO or
firmware exclusion, reset or no-reboot recovery.

Serial logs in four recent boot segments contained approximately 37–41 P340
pre-VM store-fault traces per boot, including earlier control boots; the
source calls the VM fault handler after tracing and no P332 fatal marker or
KERNEL PANIC appeared in those sampled segments. This observation does not
prove the resolution of every individual fault.

The clean reload regression for the idle-path refinement completed with exit 0
(`/private/tmp/q35-ecam-early-final-reload-20261125.log`). Hardware operations
in that regression were simulated.
