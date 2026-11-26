# ECAM contention qualification candidate

The qualification controller adds a separate, versioned contention module.
Its hold uses a fixed callback with a two-second semaphore timeout; the caller
cannot supply arbitrary code. A metadata-only query reports the active lease,
the last waiting thread and the number of controller waiters. The normal
recovery module still refuses all requests, and incompatible qualification
and general-gateway flags fail compilation. The probe driver accepts only
same-team, distinct semaphores in the expected initial state. Its wrapper
retains the driver, registration and exclusive lock after an uncertain
contention-client failure so the state can be inspected.

The actual controller methods, timeout release and competing waiter passed
ASan/UBSan simulation. Separate simulated tests exercised the production
metadata binding, semaphore checks, actual user client including a no-block
false-positive case, and the actual wrapper's clean and retained-failure
paths. The Haiku driver and clients cross-built. The recorded controller was
reproduced from its saved Jam link; two qualification builds and a third
comment-only source rebuild all produced ECAM SHA-256
`190980822c2587dc013326e1ae144851f26a2ecc70b303c4eb392ebd15c6f3a2`.
The private BFS image `fdc7539b245e64ce12b5db8f28abc037210ecb2b4f7c539b0f1c8e02fc019dba`
passed package path, entry hash, filesystem and package-roundtrip checks. The
separate helper is `d6e533802cb1f94ea08e331648c45360e09fc5193313cc2f0ec295ff22cdc7b7`;
the original control and previous qualification profiles remain unchanged.
At preparation time neither the candidate nor its helper had been uploaded, kexec-loaded or booted; the live result below supersedes that state.

On the currently running fixed-probe controller, the new metadata-only probe
confirmed that the distinct contention module is absent, then removed the
probe registration, driver, exclusive lock and staged files. Stable driver55
remained generation 1, `cold0 active_owner0`; no accelerator execution or PCI
configuration access was made by this negative control.

```text
ECAM_CONTENTION_STATE version=1 status=-2147459069 active=0 waiter=-1 waiters=0
PASS fixed ECAM lease contention-absent and cleanup; no PCI writes or accelerator execution
```

At the time of that negative control, physical contention was untested; see
the subsequent live result below. PCI reset, firmware/MMIO/DMA exclusion,
no-reboot recovery, model output and decode throughput remain unverified.

## Live contention qualification

The isolated helper verified the candidate and booted it with 64 host CPUs.
Before access, stable driver55 was generation 1, idle, with no diagnostic
process or exclusive lock. The guarded probe held the fixed lease while a
second, read-only PCI profile waited; it observed the specific waiting thread
and completed the profile after release. The general recovery service still
refused its callback. The temporary driver, registration, lock and verified
staged files were removed, and stable55 remained idle.

```text
ECAM_CONTENTION observed=1 holder_ok=1 reader_ok=1 state_idle=1 clean=1
P100A_RELOAD driver55 tag2026102100010012 generation1 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY
PASS fixed ECAM lease contention and cleanup; no PCI writes or accelerator execution
```

The loaded ECAM's 75 exported symbol tuples match the candidate and differ
from the prior fixed-lease build (`symbol-match.json`); this is layout
evidence, not a full running-image hash. Both model files retained their
recorded sizes (4B 2,740,937,888 bytes; 27B 18,973,870,528 bytes), not fresh
hashes. The final-tree reload regression passed with exit 0
(`/private/tmp/q35-ecam-contention-final2-reload-20261125.log`); its hardware
operations were simulated. This live result proves one read waited and
resumed under the lease; it does not prove device-removal safety, firmware,
MMIO or DMA exclusion, PCI reset, no-reboot recovery, model output or speed.
Repeat the 27B/process and stable-idle preflight before card access.
