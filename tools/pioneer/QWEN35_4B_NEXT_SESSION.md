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
Tag0125 adds a post-reset ARC state0 exchange on firmware queue 0 to the
disabled recovery prototype. Simulated tests cover wraparound, dropped
writes, timeout, bad responses, partial mappings and retained resources; the
normal 27B application-queue tests still pass. This is not a complete or
hardware-qualified recovery sequence. The normal tag0125 driver SHA-256 is
`fc736ae48530f37785f11ca2be56626077e549cae05e3a321db841ecd9caa366`.

Tag0126 connects the internal PCIe DBI payload setting to the non-loadable
recovery compile witness. It captures a stable value through the healthy
session's audited PCIe route before workers start, and requires that snapshot
and matching PCI configuration before reset. After reset it audits the held
fleet and firmware HDMA, restores through a fresh mapping, and retains the
owner and mappings if a write, route, or TLB restoration is uncertain. It
preserves unrelated control bits and writes zero to the status half. Simulated
tests exercise the extracted session adapter, fresh mapping, dropped writes,
busy DMA, route drift and the recovery operation. No tag0126 code has run on
the card; complete PCI capability and firmware recovery, resumable failures,
and physical qualification remain open. Recovery remains disabled in the
loadable build. The normal tag0126 driver SHA-256 is
`51b8ccacdb2efaa1a7d65588040aa9e8a835fe265463d0eedb48c331660faad3`;
the non-loadable recovery witness is
`43bd02e3763c49866f24ebbd9fbef9236ea0edb649d82c914f7981ec0975979f`.

Tag0127 removes the repeated 576-byte local activation copy before every
weight block in the resident projection worker. The Tensix path reads each
already-packed, immutable activation chunk directly from its bounded L1
cache. ASan/UBSan checks cover all chunk bytes for Q8_0, Q4_K, Q5_K and Q6_K,
including Q8's eight blocks per chunk; the RV32IMF service and both Haiku
builds pass. This is an offline change, not a performance measurement.
The normal driver SHA-256 is
`f6c6ef24e5e41bbcd97a960cf85e49f458776655e1718af7eb1a4aedbeeee4e9`,
and the service text SHA-256 is
`529c47979f45426c2114241e7063b2e74fc03403a296cb0afd4a19fbe8492ff9`.

The main remaining throughput architecture issue is dispatch. The current
decoder executes 548 separate resident worker operations for a generated
token, each of which the host stages, publishes, audits and acknowledges per
core; this is a source-level count, not a timing result. Continue the rewrite
toward device-side queued graph execution with bounded per-core descriptors,
device-side synchronization and fault receipts, while retaining all 120
matrix workers and validated model ownership. Tenstorrent's
[dispatch kernel](https://github.com/tenstorrent/tt-metal/blob/main/tt_metal/impl/dispatch/kernels/cq_dispatch.cpp)
and [prefetch kernel](https://github.com/tenstorrent/tt-metal/blob/main/tt_metal/impl/dispatch/kernels/cq_prefetch.cpp)
show device-consumed command queues and bounded staging, not a claim that
the current Haiku implementation already runs TT-Metalium.

Tag0128 adds the first device-dispatch synchronization component. A bounded
NoC1 peer transport admits only the per-worker receipt and release slots;
worker 0 remains a compute worker and collects one committed receipt from all
120 workers before sending releases. Payload and commit are separate ordered
64-byte transfers; stale or torn commits do not release work, and transport
uncertainty requires retaining the full graph. A partial release can leave
some workers advanced and must be treated as a whole-graph fault. The existing
saved L1 view covers these slots; their clear/readback while cores are held is
compiled only into a non-loadable dispatch witness. Simulated tests cover all
120 peers, failed and partial sends, stale/torn markers, explicit timeout,
held-core initialization and full L1 restore. The concrete NoC transport also
compiles for RV32IMF without atomic instructions or runtime helpers.

This is not yet a working queued graph: trusted descriptor-stream storage and fetch,
fresh-epoch ownership, worker service integration, bounded deadline and fault
propagation, and silicon validation are still open. Neither the barrier nor
its peer NoC writes have run on the card. Normal tag0128 driver SHA-256:
`6ae463bb56390e8e1693c57bbdf9b11684b54742e74b379e598d322e1af8b4b7`;
non-loadable dispatch witness SHA-256:
`dfa3472ad3714c1276bc88e072185c9a3c8574278eb16dce41e466c89f6de1e1`;
service text SHA-256:
`b544ef4561f1ec4463b9b73f55a05ead65c81becd7b610aed9634840c1cb35d1`.

The device-dispatch rewrite now also has a stream-format and workspace-plan
prototype. Each core's 2 KiB records hold an existing typed invocation, epoch,
step, core, and corruption checksum; the NoC0 fetch reuses the idle second
weight page and publishes command magic last. The prior command is invalidated
before a read, so an unsuccessful fetch cannot leave it runnable. The planner
reserves disjoint per-core ranges inside the packed workspace after the rotary,
activation and graph-scratch ranges; 548 full records on all 120 cores require
134,676,480 bytes before other workspace use. Synthetic workspace tests check
all 120 leases, overlap, capacity, forged ranges and record corruption;
embedding and projection records round-trip. The concrete fetch compiles for
RV32IMF without runtime helpers or A atomics. The checksum detects accidental
corruption; it does not authorize model access. The kernel must independently
verify workspace ownership and each command, keep the stream immutable, and
bind epochs before the worker can use it. The resident service does not yet
execute this stream; dynamic token/position patching, worker integration,
completion/fault handling and hardware validation remain open. Rebuilding all
records every token would also move about 135 MB per token, so the final path
must reuse fixed records and update only dynamic fields.

An offline link witness places the current worker image in `0x72000..0x7ffff`
and the stream/barrier compile probes in `0xa0400..0xa9fff`, between the
Tensix destination-init window and the invocation slot. Both ranges lie in
the existing saved L1 snapshot. The witness links 54,452 bytes of primary
text and 7,308 bytes of dispatch text within their respective limits, with no
unresolved imports. Tag0129 adds bounded installation and readback of both
regions while all cores are held, includes the second region in image identity
and program-counter checks, and refuses writes to it while workers run.
Injected write failures, failed or corrupt readbacks and the held/running
access rules pass simulated tests. This installation exists only in the
non-loadable dispatch witness: its second section contains compile probes,
and the resident worker does not execute the queued service. The normal
tag0129 driver SHA-256 is
`8f7a25dc4b2cbddeaf8b6d691a9342386f7e8e5e4c9d565f7b3e766f4ca5eb31`;
the non-loadable witness SHA-256 is
`22f4d211d27e3ae0a49c6040a6a3b02871b55e94c98d2fc1997f85f8f6248072`.
Neither has run on the card.

Tag0130 extracts the existing worker's single-command executor from the
host-driven service loop so a future queued service can reuse the same Tensix
and auxiliary operations. The caller remains responsible for proving command
authority, publication, fleet coordination and fault ownership; extraction
does not supply those checks or connect the stream. The resident image is
54,536 bytes, within its 57,344-byte allocation, and the RV32IMF and simulated
role tests pass. Normal and non-loadable Haiku builds pass. Normal driver
SHA-256: `66fc673b05a400cf090de5d621840c1ef73a42c68b1222b1d7c21c24d766d75d`;
non-loadable witness SHA-256:
`c9b51667a271c350701729e240af5f90af06d50ddcb7e0a7b8249635d15a27e8`.
The refactor has not run on the card and establishes no new token rate.

Tag0131 adds a per-worker graph control loop that fetches, authorizes and
executes each command, then waits for the 120-worker barrier. It uses bounded
poll counts and retains the graph on failed fetch, authorization, execution,
barrier, timeout or queue reset. A successful barrier may release some peers
before a different peer fails; a local completion is therefore not a fleet
completion proof. Simulated tests run two actual encoded embedding records on
120 workers through stream fetch and barrier logic, using a fake executor and
fake authority; they exercise failed fetches, denied authority, uncertain and
reported execution failures, partial release and timeout. They do not verify
model math or hardware operation.

A non-loadable RV32IMF witness links this control loop to the real NoC fetch,
barrier and extracted worker executor, with 54,652 bytes of primary code and
8,164 bytes of dispatch code. Its authorization hook rejects every command and
the resident service never enters the graph. Kernel validation of every exact
model-bound command, immutable stream ownership, fresh non-reused epochs,
dynamic token/position handling, fleet completion proof, fault reporting and
physical qualification remain necessary. Reusing an epoch/step can accept a
stale barrier marker; do not reuse one. A full stream per token is too large to
upload repeatedly. The normal tag0131 driver SHA-256 is
`9046b4e81acb393102d79c6c4c1b533819cde9f1b0b2e579393fff7caa5e947c`;
the non-loadable witness is
`8d40af3f685cb64b584da5841cc8faa86cfb7c6f46b28d4b677f6b596e19d362`.
Neither has run on the card; there is no new decode-rate measurement.

Tag0132 adds a metadata-only stream recorder using the existing decoder
sequence and a separate planning copy of the live model authority. It builds
each model-bound program, binds and encodes each of the 120 invocations, and
advances only the private candidate state. A sink interface demands verified
writes and a seal; no real storage or sealing backend is connected. It must
never promote the candidate to live state without a complete physical audit.
Synthetic full-shape model tests record 548 steps across 120 cores for two
successive positions, cover all nine command types, reject forged worker IDs
even with a recomputed checksum, and exercise failed writes, begin and seal.
They use a fake sink and do not execute model math. The tests exposed and fixed
an RMS record bug: its core field is at a different offset from the other
invocations. The RV64 kernel-shaped compile witness reports a largest single
frame of 3,488 bytes; a full call-chain stack audit is still required before
integrating it into the driver. The witness remains non-loadable, and its
device authorization hook still denies all commands. The current encoder
would regenerate about 135 MB per token, so efficient dynamic updates and
non-reused barrier epochs remain required.

Normal tag0132 driver SHA-256:
`b082707b8dafdfc3e2e76bd2b99349d7d02a6bd4a7180114ee54ff71448aa1b4`;
non-loadable dispatch witness:
`3ffaf3a86e85d45aaa3a067f4d9f9ffc2a0d8e410c89b1e9ca2a65c9db60c313`.
The recorder is a separate compile/simulation witness and is not connected to
either driver. No tag0132 code has run on the card.

Tag0133 adds a private page sink in the non-loadable witness. It derives the
workspace starts from the model authority's already-validated manifest,
revalidates the 120 stream leases, checks the initialized-page ledger, and
claims those ranges in the seven-bank owner. Two 2 KiB records share a 4 KiB
page; an odd final record is zero-padded. The existing resident page transfer
does full device readback. Ordinary bank range/page access refuses the
claimed ranges, and the claim's owner cannot write after sealing. Failure of
a transfer quarantines the owner. The actual driver adapter is compiled only
as a non-loadable witness; no request invokes it and the recorder is not yet
connected to it. A simulated-hardware test exercises the real bank resolver
and page transfer, page pairing, odd records, overlap/initialized-ledger
rejection, sealed access and an uncertain transfer. The existing seven-bank
control tests and both Haiku builds pass. The generic interlock only covers
bank-API access: audit every other mutation path and retain the owner, model,
stream, and fresh epoch through execution before treating storage as sealed.
Efficient dynamic updates and physical qualification also remain open.

The tag0133 normal driver SHA-256 is
`761d8c9458d7f8d80a2ccfbde53987129edbb76ff203f0d9ebc659cf08152108`;
the non-loadable witness is
`b717dfff0fee41c63f7fc6861e483a47ff0edadccd85e4c1fe157e832aabf1a4`.
Neither has run on Pioneer; there is no new decode measurement.

Tag0134 connects the metadata recorder, bound-workspace validation, private
page sink and resident transfer in one non-loadable driver compile witness.
The full synthetic graph test records 548 operations for 120 cores through
the real page sink, bank resolver and simulated transfer, and reads back the
first and last pair for each core. That is 32,880 verified page writes per
token with the current format: it is not a viable decode strategy for 100
tokens/s. Another test covers the 545-operation prompt-only sequence. Public
bank uploads, span uploads, legacy store/stream/bulk uploads and legacy
Qwen35 dataflow actions now refuse while a stream claim exists; the actual
bank control fixture checks that outside-lease model writes are rejected.
Other diagnostic and lifecycle paths still require a complete ownership audit.

The combined non-loadable build reports a conservative recorder stack
envelope of `17408/18432` bytes including a 2,048-byte allowance. This is
not an audit of a real ioctl caller; the normal dataflow envelope in the same
witness is `18160/18432` and the headroom is limited. The owner is not wired
to a request, graph launch or clean session release. Its device authorization
hook still rejects all commands; an actual lifetime/epoch protocol, fleet
completion and failure audit, efficient fixed-record reuse and physical
qualification remain necessary. The normal tag0134 driver SHA-256 is
`8966cc4906c61d36dfc8c76fec02c1917f034a190bbaffba0426d41b93717dc5`;
the non-loadable witness is
`a41ba66116553b520bc08ca37d152faad379e5687f9142b422eb582b6aae8514`.
Neither has run on Pioneer and there is no new tokens-per-second result.

Tag0135 prototypes reusable template records. Each worker fetches a sealed
template into local L1, checks it, and patches the token/embedding address,
position and recurrent ping-pong selectors in the private staging copy. A
synthetic full-shape test compares every one of the 548 operations on all
120 cores against newly generated model-authorized plans at positions 1 and
2, including a token in another embedding shard. Typed-builder tests also
cover the final context position, last vocabulary token, and active/inactive
cores. A failed patch leaves the command magic unpublished. The worker link
witness uses 54,652 bytes of primary and 9,996 bytes of dispatch code; it
remains non-loadable and its authorization hook still rejects all commands.

Template epoch is now distinct from each run's barrier epoch and sequence
range. RMS scale and argmax receipts use the command sequence, so recycling
the same sequence on a later token could accept stale data. A session-ticket
prototype allocates monotonic, non-overlapping ranges and refuses exhaustion;
it is not yet bound to the live session or an audited graph lifecycle. The
run's token, position and embedding row table must be constructed from the
live validated model, published immutably to every worker and tied to those
fresh tickets. Completion/fault receipts, guarded teardown, mutation-path
audit, hardware qualification and measured throughput remain open. Records
are still fetched from device memory each operation; no physical performance
result exists. The normal tag0135 driver SHA-256 is
`90ef29e142c91a321b32d48dd2a99ae464345969ccb5c1bc3f2dce9dbe752d67`;
the non-loadable witness is
`93fd0da9854f76674f12980c0d26288458e1d8701dea0057daf3257e724ffe27`.

Tag0136 adds a fixed 512-byte per-core graph-run descriptor in the saved L1
view at `0xab100`, after the barrier release slots. It contains the template
binding, lease, audited routes, token context, fresh run ticket, trigger and
poll bound. A corruption checksum is not authorization. The non-loadable
resident-worker witness recognizes a graph marker, validates a local copy of
the descriptor and enters the graph loop; its per-command authorization hook
still rejects every command. The host-side publisher preflights all cores,
stages and reads back every descriptor and marker, then publishes triggers.
A failed stage or partial publication retains the graph. Simulated tests
cover descriptor corruption, staged readback failure, partial trigger and
L1 write bounds; no worker has executed this on Pioneer.

The split worker witness now uses 55,812 primary and 10,220 dispatch bytes.
Its build calculates a conservative `7824/8192` worker-stack envelope with
1,024 bytes allowed for small helpers; it is not a whole-program stack proof.
The non-loadable driver's conservative recorder stack envelope is
`17424/18432` with a 2,048-byte allowance and no real ioctl caller. Both
have limited headroom. The publisher is not connected to a live session,
and sealed ownership, trusted dynamic context/ticket publication, worker
authorization, full fleet completion and fault/teardown auditing are still
required before physical use. Normal tag0136 driver SHA-256:
`7e89661ffae9792945a07b4583a2de876e588e0653ec084d2e1666d3e15e2951`;
non-loadable witness:
`7984c3bd895bf9341289537ed2d1f6128a73a1ad479e55b74df860fca64fcf03`.
There is no new throughput measurement.

Tag0137 corrects the ready check to accept a fully acknowledged worker with a
nonzero sequence while rejecting stale snapshots, shutdown and unexpected
reserved fields. Its non-loadable graph completion prototype audits all 120
terminal workers and the stable decode argmax before any acknowledgement;
faults and partial acknowledgements retain the graph. Simulated prompt,
decode and failure tests pass. The real ownership gate denies every graph,
and no live model state is committed. Complete immutable session ownership,
worker authorization, exactly-once state commit, and cleanup before hardware
qualification. The Haiku/RISC-V normal and non-loadable builds and the
resident-bank regressions pass. Normal driver SHA-256:
`d11e3806d24913a1b248d612b0b06dd78413b3f324009a342464c1b83f31c9ea`;
non-loadable witness:
`4d5bddfc3b5f5e576c00ee585a24c31acfbd8af67da0af184271d34cdca52dbc`.
The non-loadable kernel stack estimate is `18160/18432`; recorder and worker
estimates have the limitations noted above. This does not diagnose the tag0120
embedding failure and is not a decode performance result.

An offline graph-metadata replay now predicts the candidate authority without
regenerating 120 per-core records. Its simulated prompt and decode states
match the full recorder across two positions while the original state remains
unchanged. The standalone Haiku/RISC-V kernel probe compiles with a largest
individual frame of 3,536 bytes; a whole call-chain bound is still required.
It is not connected to the driver and cannot promote state; connect it only
under the future immutable-session and audited-completion ownership protocol.

Tag0138 permits a 545-operation prompt run to consume the prefix of the same
sealed 548-record decode stream; decode runs use all 548 operations. Simulated
tests compare all core records for prompt positions zero and one, including
dynamic token patching, and check worker prefix termination and bounds. The
publisher now requires ownership validation before staging, including the
template's fixed activation slot. The actual callback still rejects every run;
the future session must retain and verify that fixed slot, sealed plan, model,
fresh ticket and live authority before it can authorize publication. The
target suite and both Haiku/RISC-V builds pass. Normal driver SHA-256:
`5996e2b386eea9c8df5660830186458cc2ab98e17e57ed6f76468126b03640c6`;
non-loadable witness:
`f56bd0561a08005dd5e80a77bb98fc0ffe3dcc812879f4af6c316aa431f71ae6`.
No hardware run or performance measurement has been made for this change.

Remaining firmware policy: the local Tenstorrent KMD checkout at
`22cca4cb33455d962cf8340a8f5b896dae696fd7` restores PCI state and DBI MPS,
sets MRRS to 4096, sends state0, then requests watchdog timeout message
`0xc1` (default 10,000 ms), and optionally configures firmware log DMA. Its
log buffer setup requires IOMMU translation. The Haiku prototype currently
restores the prior MRRS, sends state0, and does not set that watchdog or
enable firmware log DMA. Determine the pinned firmware's watchdog semantics
and Haiku's safe policy before sending `0xc1`; do not assume the Linux
default or introduce an unowned DMA buffer. Full PCI state restoration remains
a separate open requirement.

Haiku is reachable at `10.0.0.247`. Following one authorized power cycle
and the pinned NVMe/kexec handoff, the saved recovery script restored stable55
at generation2: `cold0 active_owner0 open_count1`. The prior diagnostic has
exited, the recovery lock is absent, and both model files retain their
recorded sizes. The current `B_KEEP_LOADED` guard is still the legacy version;
after the new guard is staged, it requires another fresh Haiku kernel boot
through the NVMe/kexec path to activate. See
[`tag0124 recovery`](benchmarks/qwen35-dataflow-tag0124-20261122/RECOVERY.md).

Offline recovery-guard work adds a 48-byte kernel proof for reset completion,
firmware readiness, PCI restoration, rebuilt mappings, all 120 cores held,
idle NoC/DMA, and released owners. Ticket and fault-epoch checks reject
concurrent faults. A separate, fail-closed helper now encodes the exact three
Blackhole ASIC reset PCI writes and bounded reset-marker wait; fake-PCI tests
pass. It is connected to a default-disabled recovery prototype. Focused
recovery tests and the full Haiku/RISC-V build pass with stack usage
`15712/18432`. PCIe capability restoration, remaining firmware initialization,
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
default-disabled prototype; remaining firmware initialization, complete PCI
capability restoration, and physical verification remain open.
The disabled prototype also captures and restores PCIe configuration-space
payload and read-request settings, with simulated failure tests. Tag0126 adds
the separate internal DBI payload shadow; remaining PCI capability state is
still required for full reset restoration.

The post-reset backend now requires a bounded ARC scratch readiness poll and
the pinned firmware telemetry and harvesting values before fleet and DMA
auditing. Simulated tests cover delayed or failed firmware, wrong telemetry,
interrupted wait, and failed TLB restoration. The new state0 queue exchange
and remaining initialization have not been run on the card.

The default build now targets dedicated tag0138. Since tag0121 it records final release of a
retained owner under the driver lock and provides a read-only, tag/generation
checked owner-lifetime status ioctl. Extracted real close/status tests and the
RISC-V build pass. The status ioctl does not reset the card or retire any
resource. A separate recovery-run prototype is disabled by default until PCIe
capability restoration and remaining firmware initialization are implemented and
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
running. The independently verified stable driver status is:

```text
P100A_RELOAD driver55 tag2026102100010012 generation2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY
```

The recovery lock has been released. Both models retain their recorded sizes:
Qwen3.5-4B `2,740,937,888` bytes and Qwen3.8-27B `18,973,870,528` bytes.
The proof-based in-place recovery operation remains disabled and unqualified;
complete it and activate its new guard before the next model diagnostic.

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
