# ASIC reset root readiness and polling order

The Blackhole ASIC timer-reset path now checks the root port before reading
endpoint configuration space. The root-only readiness check requires the
pinned SG2042 root identity, an available PCIe root capability, a valid link
speed/width with no training and any advertised data-link-active indication,
no nonfatal/fatal/unsupported-request device-status bits, and clear root
uncorrectable AER status. Correctable device-status events are not used as a
reset prohibition. The reset preflight checks root readiness before endpoint
reads and around its marker and timer writes. After the trigger, each poll
checks root identity, bus-master state, link and errors before reading the
endpoint, rechecks readiness between the vendor and marker reads and again
before accepting completion. A ready sample cannot guarantee that the link
will remain healthy during a later endpoint read. The helper does not clear
AER, prove continuous writer/DMA exclusion or itself perform recovery.

The actual reset-sequence test passed ASan/UBSan with root-down preflight,
root loss between reads and writes, unconfirmed marker/target retention,
bounded timeout and a later poll without reissuing the reset. A separate
ASan/UBSan test exercised the real readiness helper against malformed root
capabilities, link-training/width/speed faults, reported fatal status and
AER errors, and verified zero endpoint reads. The actual Haiku recovery
qualification cross-build passed (driver SHA-256
`d906150c3654e5d521b45e80927919067bfd3ed8a80f066126571b081af4d147`),
as did the fused/tree/sharded/link-reset non-loadable witness (driver
`4ddf3e3f9131a28b18816dabfa78949aeb4ce4e2a721bb1a36fb695718c2b690`).
The broad recovery reload suite exited 0 (`/private/tmp/q35-asic-root-order-reload-20261125.log`);
it started before the later probe and wrapper edits, which were separately
cross-built and exercised by the wrapper's final simulated test. Its device
operations are simulations.

On the running Haiku candidate, after stable driver55 generation 1 idle and
no diagnostic or exclusive lock checks, the separately named probe invoked
the same root-only readiness helper through the pinned root-port driver:

```text
ECAM_ROOT_RESET_READY version=1 status=0 ready=1 endpoint_reads=0
P100A_RELOAD driver55 tag2026102100010012 generation1 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY
PASS root-only reset readiness and cleanup; no endpoint reads, PCI writes or accelerator execution
```

Probe driver SHA-256 `710bdb640fb096d899447f58d6b25e7d16ea075d8a1fa4765e3baf77cfd2b346`,
client `2d9842a8b21b3fcb860f18efc223cd0c860332b633d0a6d4c1be78b468c10c77`,
and wrapper `760c1713ca166415c26dbb99b04b7b1f3bfa231f0a1817eb969f1cc9fd8f5003`
were checked before execution and before removing the temporary files. The
probe device, image, registration and locks were removed, and stable55
remained idle. Both model files remained present at their recorded sizes,
not freshly hashed. This was an idle root sample, not a reset; no no-reboot
recovery, model output or decode throughput was established. Repeat the
27B/process and stable-idle preflight immediately before card access.
