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
Neither the candidate nor its helper has been uploaded, kexec-loaded or booted.

On the currently running fixed-probe controller, the new metadata-only probe
confirmed that the distinct contention module is absent, then removed the
probe registration, driver, exclusive lock and staged files. Stable driver55
remained generation 1, `cold0 active_owner0`; no accelerator execution or PCI
configuration access was made by this negative control.

```text
ECAM_CONTENTION_STATE version=1 status=-2147459069 active=0 waiter=-1 waiters=0
PASS fixed ECAM lease contention-absent and cleanup; no PCI writes or accelerator execution
```

No physical contention test, PCI reset, firmware/MMIO/DMA exclusion,
no-reboot recovery, model output or decode throughput is established. Repeat
the 27B/process and stable-idle preflight immediately before card access.
