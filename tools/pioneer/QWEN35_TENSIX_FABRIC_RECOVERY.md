# Qwen3.5: candidate Tensix-fabric recovery

The pinned Tenstorrent system-firmware v18.5.0 source includes a Tensix reset
sequence in [`app/smc/pytest/e2e_smoke.py`](https://github.com/tenstorrent/tt-system-firmware/blob/80b056af360a58730756467d5cc8df8db789640d/app/smc/pytest/e2e_smoke.py),
`tensix_reset_sequence`. It is a candidate for recovering a faulted resident
rewrite without resetting the PCIe endpoint. The complete sequence is wired
inside a retained, non-loadable recovery transaction and has been simulated.
A one-shot private recovery request now prepares the guarded mapping and
calls that transaction for a retained faulted session; it is present only in
the non-loadable witness and has not been tried on this Pioneer. Its
context-owned queue adapter, eight RISC reset-unit writes and
120-worker unicast soft-reset pass retain the owner on uncertain failure.
Unicast coverage and the expected all-ones reset-unit readback remain
unverified on the card. The owner also requires an automatic-clock baseline,
250 MHz in forced mode, and automatic mode after release; configurations
without automatic clock mode are currently rejected. The reported
v18.5.0 firmware family is not proof
that the installed binary matches the source.

The upstream test performs these operations in order:

1. Send `FORCE_AICLK` message `0x33` with 250 MHz.
2. Write zero to each of the eight active-low Tensix RISC reset-unit words at
   `0x80030040 + 4*i`.
3. Send `TOGGLE_TENSIX_RESET` message `0xaf`; check the firmware response.
4. Send `REINIT_TENSIX` message `0x20`; check the firmware response.
5. Broadcast `0x47800` to Tensix `SOFT_RESET_0` at `0xffb121b0`.
6. Write `0xffffffff` to the same eight RISC reset-unit words.
7. Send `FORCE_AICLK` with zero to release the forced clock.

This is a whole-fabric operation, not a per-worker reset. The pinned firmware's
[`ReinitTensix`](https://github.com/tenstorrent/tt-system-firmware/blob/80b056af360a58730756467d5cc8df8db789640d/lib/tenstorrent/bh_arc/init.c)
clears NoC translations and calls `NocInit`, which reprograms NoC nodes beyond
the Tensix tiles. It also restores translation and clock-gating settings when
configured. Existing routes and saved device state therefore cannot simply be
assumed valid afterward. The test checks one Tensix scratch bit after the
sequence; it does not demonstrate recovery of this driver's 120-core workload,
its saved RAM and DRAM contents, or stable-driver reload.

The last attempted recovery on the Pioneer used a different ASIC+DMC firmware
reset and produced root PCIe error status before the first endpoint poll; see
[`benchmarks/qwen35-firmware-trace-20261124-ae/RUN.md`](benchmarks/qwen35-firmware-trace-20261124-ae/RUN.md).
That result does not establish whether the Tensix-only sequence succeeds or
fails. Do not repeat the failed ASIC reset as a substitute for testing this
candidate.

Before any physical trial, complete restoration and retirement in the
retained-owner recovery path. Establish exclusive admission and serialization
with firmware messages, PCI mappings and all NoC users; retain owner, snapshots
and quarantine on an uncertain step; and define
which steps can safely be resumed without reissuing a reset. Check the actual
firmware identity and message response conventions, clock state restoration,
all eight reset-unit words, the broadcast's harvested-core coverage, and the
post-reset NoC/DRAM routes, host DMA, all 120 workers and stable-driver state.
The normal idle/27B/model-hash/staged-restore preflight remains mandatory.
A timeout, changed identity, uncertain posted write or failed audit must not be
reported as successful recovery. Do not use this candidate until its real
controller, failure handling and bounded trial are ready.

The transaction now rechecks firmware boot, queue identity and pinned
telemetry after its fleet and DMA audits. It restores the selected route on a
successful audit and stops on an unconfirmed route write or lost guard; the
owner and mappings remain retained on failure. This is simulated evidence,
not a binary identity for the installed firmware or a completed recovery.
