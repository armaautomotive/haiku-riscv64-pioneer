# Qwen3.5-4B Pioneer bring-up: next-session handoff

Continue the active goal from checkpoint `94117b2279` and
[`QWEN35_4B_BRINGUP.md`](QWEN35_4B_BRINGUP.md). Finish with two valid warm
Work outputs on Haiku, Q4_K_M projections using all 120 Tensix cores, no CPU
inference or fallback, at least 100 decode tokens/s, separate model-load,
prompt-processing, and decode timings, clean Work unload, intact Qwen3.8-27B
setup, and the tested checkpoint pushed to GitHub. Historical Tag70 measured
0.115 tokens/s; Tag96 produced no valid output. No run has reached 100 tokens/s.

## Current checkpoint: tag0120 failed; recovery rewrite in progress

Only the Tenstorrent-style resident device-program rewrite is in scope until
that rewrite is complete; benchmark acceptance and assistant integration
follow afterward. The standalone Haiku client and resident BRISC/Tensix
service use all 120 Tensix cores and have no CPU inference/fallback path.

Pioneer uses Fedora on its existing NVMe and kexec to Haiku; SD cards are no
longer used. Tag0120 loaded the full 4B model, then failed the first core0
embedding read. Authentication took `99.767714` s, tokenization `12.726700` s,
model load `337.246020` s, and failed prompt processing `0.612460` s. It
produced no token; decode time and dataflow decode rate are unavailable. The
NoC0 response counter was one behind expectation, with transaction ID0
outstanding. Tag0120 preserves the failed read's remote address, route, and
size in the 64-byte worker mailbox, with a read-only `QUERY_READ_REQUEST`.

Tag0122 corrects a concrete NoC route defect in the resident rewrite. The old
code read mutable command register `0x14` as its own NoC coordinate; the
worker and driver now read logical-ID register `0x148`, and read/write
commands use marked static VC1. This could explain the missing embedding
response, but has not been verified on the card. The host protocol fixture,
RV32IMF role compile and Haiku/RISC-V target build pass. The tag0122 default
driver SHA-256 is
`6a4c03b152b01ec623b078755992b8fc342a558af41567e2d8775a1e9b2c9957`.
Tag0123 additionally masks the logical ID's software-available upper bits,
checks the NoC0 identity against each selected worker, and permits documented
hardware changes to reserved `NOC_CTRL` bits when comparing completed
commands. The NoC1 logical-route and destination-translation assumptions are
not yet verified on hardware. The tag0123 normal driver SHA-256 is
`5163d79689ce225100913bbc26b4bc8329e9de22642d5641808c805de839130f`.
Tag0124 adds a read-only NoC1 audit for all 120 workers and the seven bank
routes before the service starts; it rejects identity, translation, drift or
busy-state mismatches. The fixture and Haiku/RISC-V builds pass, but no card
run has verified the audit. Its normal driver SHA-256 is
`3336b78873687f2c491314751e39e0f2942a0bcf0dfccd2d3f9bcdf4f4cc79ea`.

Haiku is currently reachable at `10.0.0.247`. Read-only status confirms the
failed diagnostic client has exited and both model files remain intact, but
the candidate is still `generation3 cold1 active_owner1 open_count1`; the
exclusive recovery lock and stable55 backup remain. Stable55 is not loaded or
idle. Do not run inference or accelerator commands in this state. The
currently loaded `B_KEEP_LOADED` legacy guard has no API to clear this latch,
so the new guard requires one fresh Haiku kernel boot through the NVMe/kexec
path.

Offline recovery-guard work adds a 48-byte kernel proof for reset completion,
firmware readiness, PCI restoration, rebuilt mappings, all 120 cores held,
idle NoC/DMA, and released owners. Ticket and fault-epoch checks reject
concurrent faults. A separate, fail-closed helper now encodes the exact three
Blackhole ASIC reset PCI writes and bounded reset-marker wait; fake-PCI tests
pass. It is connected to a default-disabled recovery prototype. Focused
recovery tests and the full Haiku/RISC-V build pass with stack usage
`15712/18432`. PCIe capability restoration, firmware queue initialization,
and silicon qualification remain open. See
[`tag0119 RUN`](benchmarks/qwen35-dataflow-tag0119-20261121/RUN.md),
[`tag0120 BUILD`](benchmarks/qwen35-dataflow-tag0120-20261121/BUILD.md), and
[`tag0120 RUN`](benchmarks/qwen35-dataflow-tag0120-20261121/RUN.md).

An offline audit checks both NoCs across all 120 held cores twice and fails on
unreadable, busy, or outstanding status. The guard now publishes fault epoch
and cold state atomically; an injected fault during latch clearing leaves the
old generation recoverable. Both fixtures pass, as does the RISC-V target
build. A fresh post-reset mapping backend now validates PCI BARs, topology and
each selected core, and restores a TLB value captured after reset. Simulated
MMIO tests cover mapping and cleanup failure cases. The mapping and audit are
integrated into the default-disabled prototype and have not run on the card.
A post-reset PCI helper now restores the fixed active BAR/bridge layout with
bus mastering disabled; simulated PCI tests cover lost writes and unsafe
states. A simulated-tested audit of both PCIe pending states and the pinned
firmware's channel-0 HDMA now follows the fleet audit and restores its fresh
TLB snapshot. Recovery orchestration and owner retirement are wired into the
default-disabled prototype; firmware queue initialization, complete PCI
capability restoration, and physical verification remain open.
The disabled prototype also captures and restores PCIe configuration-space
payload and read-request settings, with simulated failure tests. It still needs
the separate internal DBI payload shadow and remaining PCI capability state;
the endpoint configuration fields alone are not full reset restoration.

The post-reset backend now requires a bounded ARC scratch readiness poll and
the pinned firmware telemetry and harvesting values before fleet and DMA
auditing. Simulated tests cover delayed or failed firmware, wrong telemetry,
interrupted wait, and failed TLB restoration. Message-queue initialization
remains open; this new code has not been run on the card.

The default build now targets dedicated tag0124. Since tag0121 it records final release of a
retained owner under the driver lock and provides a read-only, tag/generation
checked owner-lifetime status ioctl. Extracted real close/status tests and the
RISC-V build pass. The status ioctl does not reset the card or retire any
resource. A separate recovery-run prototype is disabled by default until PCIe
capability restoration and firmware queue initialization are implemented and
the path is qualified. Do not install or run tag0121 until the required clean
Haiku boot and guarded hardware preflight.
Tag0121 now also compiles a proof-gated session-area retirement method;
simulated tests cover invalid evidence, duplicate areas, host-buffer release
ordering, and partial failure. It is called by the disabled prototype only.
The recovery guard now resolves through the existing pinned reload-guard
image path; its previous separate path could not be found by Haiku's module
resolver. A simulated-tested PCI teardown restores the idle BAR/bridge state
without replaying stale TLB snapshots. The recovery sequence is compiled in a
separate non-loadable prototype build. Hardware checks and resumable recovery
after a failed attempt remain open.

The last valid output measurement remains a legacy Work-path result at
`0.115` decode tokens/s. No dataflow run has produced a valid token or reached
the 100 tokens/s target.

## Saved Pioneer instructions and access

The root [`AGENTS.md`](../../AGENTS.md) records the user-authorized
development-box restart procedure and its limits. Follow one OFF, wait,
check power evidence, then one ON. Never repeat OFF to confirm shutdown.

Pioneer `10.0.0.247` SSH accounts, identity paths, and separate pinned host
keys are documented locally in [`PIONEER_SSH_ACCESS.md`](PIONEER_SSH_ACCESS.md).
Use strict host-key checking and the profile for the OS currently running.

## Current hardware state

Haiku SSH is reachable. The tag0120 client exited; no inference process is
running. Current driver status is:

```text
P100A_RELOAD driver60 tag2026112100010120 generation3 quiesced0 cold1 active_owner1 open_count1 guard1;CPU_METADATA_ONLY
```

The exclusive recovery lock and stable55 backup remain, so the driver is not
idle. Both models are intact: Qwen3.5-4B `2,740,937,888` bytes and Qwen3.8-27B
`18,973,870,528` bytes. Do not issue another accelerator/reset command until
the new proof-based recovery operation has been integrated, tested offline,
and installed after a clean NVMe/kexec Haiku boot.

## Tag61: inner matrix receipt was overwritten

Tag61 passed idle, geometry, and command setup in its prior tag60 checkpoint,
then the ROOT matrix returned zero. It added shape, scale, and activation
receipts to the matrix image. The run again showed
`dispatcher_stage=0x5e000007 detail=0`, but the outer dispatcher overwrote the
matrix receipt before terminal reporting. Authentication took `67.641308` s,
tokenization `12.638489` s, and model loading `223.297109` s for all 3,450
spans. No prompt prefill, valid output, or decode rate was produced.

Recovery passed again: Work STOP proved safe idle, clean unload reported
`state=2 active=0 cold=0`, and stable041 hot-restored at generation10 without
reboot. Next copy the inner stage/detail into ROOT progress context before
the dispatcher writes its outer failure marker. See [`tag261 BUILD`](benchmarks/qwen35-work-tag261-20261119/BUILD.md)
and [`tag261 RUN`](benchmarks/qwen35-work-tag261-20261119/RUN.md).

## Tag60: ROOT matrix returned zero bands

Tag60 added checkpoints around the ROOT idle checks, matrix command setup,
matrix return count, and result copy. It loaded from stable041 generation6 at
generation7, authenticated in `68.115594` s, tokenized the 18-token prompt in
`13.258253` s, and loaded all 3,450 spans in `220.424219` s. The Work run
faulted before prefill with zero commits. Idle, geometry, and command checks
passed; `q35_work_matrix_band()` returned0 instead of160
(`dispatcher_stage=0x5e000007`). The ten-tile job is type13/Q5_K, 2,560 inputs,
one group, and route `0x492`. No valid output or decode rate exists.

STOP passed the safe-idle proof, the client unloaded cleanly
(`state=2 active=0 cold=0`), and the wrapper hot-restored stable041 at
generation8 without a reboot. The client and shared lock are gone, both model
files are intact, and stable041 is idle. See [`tag260 BUILD`](benchmarks/qwen35-work-tag260-20261119/BUILD.md)
and [`tag260 RUN`](benchmarks/qwen35-work-tag260-20261119/RUN.md). Next add
failure receipts inside the ROOT matrix routine to separate shape, scale, and
activation validation returns.

## Tag58 hardware recovery and remaining fault

Tag58 fixes the stale cached peer-nonce comparison in the second safe-idle
audit and returns pre-reset proof failures as retryable without quarantining
the owner. Its client repeats the read-only STOP proof while the driver FD is
open. The warnings-as-errors cross-build and WorkOwner retry regression
passed. See [`tag258 BUILD.md`](benchmarks/qwen35-work-tag258-20261119/BUILD.md).

Tag58 loaded from stable041 generation2 at generation3. Authentication took
`99.633210` s, vocabulary/tokenization `12.869909` s, and model loading
`223.299044` s for all 3,450 spans. It faulted before prefill with zero
commits at `dispatcher_stage=0x58778210 detail=0x4`, the pending worker119
4-byte write to remote doorbell `0x74100`. No valid output or decode rate was
produced.

The client then passed safe-idle proof, completed explicit STOP and clean
unload (`state=2 active=0 cold=0`), and the wrapper hot-reloaded stable041 at
generation4 without a board reboot. Postflight reported
`SAFE_FAULT_RECOVERY_VERIFIED`. The retryable proof-failure branch passed its
offline owner regression but was not triggered by this hardware run. The
recovery path is verified; the worker119 doorbell fault still blocks prompt
prefill and throughput measurement. Continue from the stable041 idle state;
no restart is needed to load another candidate.

## Verified explicit recovery through tag250

The latest hardware run used tag250 from stable041 generation6. It loaded all
3,450 resident spans (`2,740,498,432` bytes); authentication took `67.576945`
s, prompt tokenization `12.875780` s for 18 tokens, and model load
`222.802717` s. ROOT faulted before prefill with zero commits at
`dispatcher_stage=0x58778210 detail=0x4`, identifying worker119's 4-byte write
to remote `0x74100` with the NoC command still busy. No output or decode rate
was produced.

The Work STOP reproof and clean unload passed. Stable041 hot-reloaded at
generation8 (`cold0 active_owner0 guard1`), without reboot; the shared lock
was absent and both model files remained intact. Tag250 logs and checksums are
in [`tag250 evidence`](benchmarks/qwen35-work-tag250-20261119/evidence).

## Tag52 error cleanup

The tag52 offline build keeps card I/O out of the close callback. The client
now tracks whether BANK BEGIN, a span upload, or Work PREPARE may have taken
effect. On model-load failure it explicitly tries SPAN END, Work STOP when
PREPARE may have bound the ROOT, then BANK END before close. START/POLL errors
also flow through Work STOP and BANK END. The warnings-as-errors cross-build
and existing WorkOwner regression passed. Tag250 is the hardware proof for
STOP recovery and clean unload without board reboot; tag52 is not yet installed
on the card. Hashes are in
[`tag52 build record`](benchmarks/qwen35-work-tag252-20261119/BUILD.md).

## Tag53 hardware fault receipt and recovery

Tag53 extends the terminal Work poll result with the ROOT's last ten NoC
command words, eight counter values/deltas, and worker119's stable 16-word
status plus two PC samples. The driver reads them only after the all-120-core
safe-idle proof; there are no additional packets, resets, or retries. The
Work protocol is version 3. The warnings-as-errors cross-build and WorkOwner
regression passed. Hardware reproduced the pending 4-byte worker119 doorbell
fault with zero commits. The ROOT NoC snapshot and worker119 status/PC samples
were both valid. Worker119 status reason `0x50` is the peer image's
`panels != count` fault, but detail words 7/8 both read zero; the mismatch
remains unexplained. Authentication took `99.599927` s, prompt tokenization
`12.441273` s, and model load `224.169305` s. No decode result or valid output
exists.

The client completed explicit STOP and clean unload (`state=2 active=0
cold=0`). Tag53 was quiesced and stable041 hot-reloaded at generation4 without
reboot. The lock is absent and both model files remain intact. Full logs and
hashes are in [`tag253 evidence`](benchmarks/qwen35-work-tag253-20261119).
Continue with a diagnostic that captures the peer's nonzero job/count fields
and validates the fault snapshot mapping. Do not restore tag251's close-time
I/O experiment.

## Tag251: failed automatic close recovery

Tag251 tried to run the full fleet proof and teardown from the device-release
callback when the client exited after a terminal fault. The guarded test
reached the known worker119 doorbell fault, then `close()` did not return and
Haiku became unreachable. This automatic close path was reverted for tag52;
card I/O stays in explicit STOP/END, and client close retains the owner.

The board was recovered with the authorized single OFF/wait/ON sequence and
the pinned Linux-to-Haiku kexec path. The tag251 recovery helper hot-reloaded
stable041 at generation2; current status is idle and both model files are
intact. The raw post-cycle logs are untrusted because their writers were
interrupted. Details and hashes are in
[`tag251 RUN.md`](benchmarks/qwen35-work-tag251-20261119/RUN.md).

Tag250 and tag253 verify explicit STOP reproof, clean unload, and stable041
hot reload without board reboot. The latest tag253 trial already restored
stable041 at generation4. The next hardware candidate must pass a new strict
idle preflight and retain explicit STOP/BANK END on every fault path.

## GitHub progress

The current branch is `codex/qwen35-4b-p100a` on `origin` in
`armaautomotive/haiku-riscv64-pioneer`. The user asked to push incremental
Qwen3.5-4B progress as milestones are verified. Push source/build evidence
and run records on that branch; do not include local access credentials or
unrelated workspace changes. Commits through `88cb300ed0` are pushed. Tag258
hardware recovery and tag257 cold-latch recovery evidence should be pushed as
the next checkpoint; do not stage local access credentials or unrelated
workspace changes.

No valid decode-rate measurement exists yet. Per-run evidence is under
`benchmarks/`.
