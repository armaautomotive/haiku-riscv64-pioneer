> **Tag0165 selector fix:** tag0164 observed zero Dst coefficient/minimum rows even though the L1 source coefficients were nonzero. The rewrite now restores the fixed-format Dst metadata selector (config73 `0x13`) before the SFPU accumulator metadata load; ordinary vector unpack had reset it to `3`. This is in addition to the affine alias fix. Full target suite/builds pass; hardware effect remains unverified. See `QWEN35_4B_NEXT_SESSION.md` for exact current artifact hashes.

> **Tag0165 prepared fix:** the earlier affine staging copy overwrote the source minima with coefficients because `PioneerQ38Panel` has a 512-byte operand prefix, not the offline Q5 tile's 576-byte prefix. The rewrite now places the panel so its fields directly match the SFPU source addresses, with compile-time layout checks and a distinct-values test. Full target suite and builds pass; hardware impact is unverified. Tag0164 is still running and must unload before testing this fix.

> **Tag0164 next:** trace the loaded Dst metadata and first-panel SFPU accumulator to distinguish a metadata-load fault from arithmetic/readback. Full target suite and builds passed; candidate remains untested on hardware. See `QWEN35_4B_NEXT_SESSION.md`.

> **Tag0163 candidate:** restored qualified Tensix source-window initialization and destination clear, and added bounded core0 first-projection samples for packed activation, scale, dot, and coefficient. Tag0163 hardware showed matching nonzero matrix dots but zero projection output; see `benchmarks/qwen35-dataflow-tag0163-20261122/RUN.md`.

> **Tag0162 hardware trace:** token0 embedding samples match the independently decoded GGUF; layer0 and final RMS samples are nonzero, while sampled first QKV, gate, and final output projection pages are all zero. Output remains incorrect; unload was clean and stable driver55 generation6 is idle. See `benchmarks/qwen35-dataflow-tag0162-20261122/RUN.md`. Continue only the rewrite and investigate the projection path.

# Qwen3.5 Tenstorrent device-program rewrite

Tag0162 adds a diagnostic-only root-worker trace at eight graph steps and
a CPU-side query for the last audited argmax score. It reads raw device bits
only after graph completion, checks two stable copies and verifies each sample's
sequence against the audited result. The trace is not an inference input or
release proof. An independent Q6_K decode of token0 from a hash-matched GGUF
prefix gives rows16/31 as `0xbd2f21c0`/`0x3cb4c800`; these apply to the trace
only when its last graph input is token0. Trace, protocol, PCI range and full
target tests pass, as do normal, trial and non-loadable witness builds. No
tag0162 hardware result existed at the time of that entry.


Tag0161 passed the former convolution stall and completed one graph run, but
generated `0,0` (`!!`) instead of the verified `11751,13` (` Paris.`). The
raw one-token decode rate was 0.242095 tokens/s after 19.186788 s prompt
processing and 612.118705 s load; because output is incorrect, this is not an
acceptance benchmark. Unload succeeded and stable driver55 was restored idle
without reboot. See the [tag0161 record](benchmarks/qwen35-dataflow-tag0161-20261122/RUN.md).
The rounding-independent SiLU follow-up was not in tag0161; model numerical
correctness remains the next rewrite problem.


The [Blackhole BabyRISCV instruction set](https://github.com/tenstorrent/tt-isa-documentation/blob/e0b765516b78f8faf34004f93d68ad49c59f4725/BlackholeA0/TensixTile/BabyRISCV/InstructionSet.md)
confirms that `fdiv.s`/`fsqrt.s` are not implemented and that float rounding
mode bits are ignored. The SiLU exponent calculation previously emitted
`fcvt.w.s ... rtz`; it now decodes the small exponent with integer operations
so its truncation does not depend on hardware rounding mode. The worker-image
audit rejects these conversions as well as unsupported divide/square root.
SiLU and convolution numerical tests, the graph service image build and the
full target suite pass. This follow-up has not yet run on hardware.


Tag0160 traced the first convolution stall to arithmetic on group0. Its exact
worker disassembly contains `fdiv.s` inside `DataflowSiluProduct`; BRISC's
existing qualified math uses software division. Tag0161 replaces the runtime
floating divides with that helper and adds an image build guard rejecting
floating divide and square-root instructions. SiLU, convolution and one million
independent software-division comparisons pass; the target suite and all three
Haiku builds pass. This diagnosis and fix still require hardware confirmation.
Tag0160 needed an NVMe power cycle to restore stable driver55; no token or
decode rate was obtained. See [the run record](benchmarks/qwen35-dataflow-tag0160-20261122/RUN.md).


The rewrite client now performs up to four bounded recovery attempts when a
captured graph fault specifically returns `RECOVERY_PENDING`, keeping the
original owner open while workers may settle. Every retry uses the existing
fence and all-core fault audit; all other errors stop retrying. The retry
policy and real teardown integration pass simulation and target builds. This
has not been exercised on the card and does not recover a permanently stuck
NoC or qualify the disabled ASIC-reset prototype.


Tag0160 adds correlated, read-only per-group convolution progress markers for
validation, reads, arithmetic and writes. The progress-aware wait recognizes
forward group/phase movement without treating the marker as a completion or
recovery proof. A simulated all-120-core convolution test verifies marker
order and the regular target suite now includes it. The target suite and
normal, qualification, and non-loadable reset-witness builds pass. Hardware
qualification has not run. The qualification driver SHA-256 is
`8c524d28bcb59107d6772d9a7831ff4c1dec6929978e7c2b66f24eb5c3ab4fc7`.


The guarded tag0159 run reached step7 (layer0 recurrent convolution), after
embedding, residual save, normalization, and four projections, then timed
out with worker0 executing. No token or decode throughput was produced.
In-place recovery failed for this unsettled graph; an authorized NVMe power
cycle restored stable driver55 and preserved the recorded model-file sizes.
The progress receipt is diagnostic and proves neither numerical correctness
nor safe idle. See the [tag0159 record](benchmarks/qwen35-dataflow-tag0159-20261122/RUN.md).


Tag0158 timed out after 8.258036 s with a worker still running; no token or
decode rate was produced. Its unsettled graph could not be recovered in place.
An authorized NVMe power cycle restored stable driver55 idle and preserved both
model files at their recorded sizes. Tag0159 adds read-only, non-authoritative
worker step/phase observations in a separate saved L1 word, and a bounded
progress-aware host wait (30 s inactivity, 600 s total). Tests cover the
progress encoding, idle and total deadlines, terminal and invalid states, and
separation from host-running writes. The target suite and all three target
builds pass; tag0159 has not yet run on the card. The ASIC-reset recovery
prototype remains unqualified. See the [tag0158 record](benchmarks/qwen35-dataflow-tag0158-20261122/RUN.md).


## Scope

Rewrite the Qwen3.5 Work execution path around one resident BRISC worker per
Tensix core. Each worker owns one immutable shard, performs bounded NoC 0
reads, issues projections to the Tensix matrix engine, and sends results with
NoC 1. Keep two alternating L1 pages for weight and activation staging, sharded
intermediate activations, and no host inference path. This follows
Tenstorrent's per-worker BRISC programming pattern; it is not a claim that the
Haiku image runs TT-Metalium or its separate NCRISC/TRISC kernels. Keep the
checkpoint history on `codex/qwen35-4b-p100a`.

Complete the device-program rewrite before benchmarking; integrate with the
assistant only after the hardware generation and throughput gates pass.

## Current status (2026-11-22)

The active work remains the Tenstorrent-style device-program rewrite. It uses
120 resident BRISC workers and Tensix matrix engines, with no CPU inference
or fallback. Pioneer boots from its existing NVMe: Fedora hands off to Haiku
with kexec. The user confirmed SD cards are no longer used; do not use or
recommend an SD-card workflow.

Tag0153's first hardware qualification loaded the verified 4B model and
installed the graph, but the first prompt graph failed with a retained
session before any token. The no-reboot recovery reached restored-fleet resume
and failed; a second close attempted the old stop audit after restoration.
An authorized NVMe cold recovery restored stable driver55, idle and without a
recovery lock. See [the run record](benchmarks/qwen35-dataflow-tag0153-20261122/RUN.md).

Tag0154 adds graph phase/core failure codes, preserves an exact
settled-graph proof across close retries, and retries only the final fleet
audit after all restored workers have started. A partial start remains
retained and is not replayed. Recovery responses now expose the service-close
failure details. Host simulations and the RV32IMF target suite pass; this
change was qualified in the diagnostic described below. The exact cause of
tag0153's graph failure remains unknown.
Before another card operation, recheck that the 27B diagnostic has exited and
the stable driver is idle.

Tag0154's diagnostic reached worker 0's graph-completion check without a
token. Its recovery failure identified stale service-image trust after the
original dispatcher image was restored, because both images have the same
boot word. Tag0155 invalidates the cached proof before held-core writes and
restoration, and records a device-graph fault receipt for a CPU-only host
query. Partial restoration and timeout cases retain ownership. The 0155
target/simulation suite and all three builds pass; hardware qualification
remains open. The card is back on stable driver55 with no lock after NVMe
cold recovery. See [the tag0154 run record](benchmarks/qwen35-dataflow-tag0154-20261122/RUN.md).

Tag0155 narrowed the graph fault to worker 0's first graph-record fetch and
successfully recovered that settled fault, unloaded the bank and restored the
stable driver without rebooting. No token was generated. Tag0156 adds the
fetch/NoC stage diagnostics needed to localize the remaining error; simulated
tests and all builds pass, but tag0156 has not run on hardware. See
[the tag0155 run record](benchmarks/qwen35-dataflow-tag0155-20261122/RUN.md).

Tag0156 traced the first graph fetch failure to the driver's exact-match check
on NoC counter `0x20c`. Tenstorrent documents that counter as unsupported to
read on Blackhole; the other recorded counters matched and the graph fault
again recovered without reboot. The next rewrite omits that register while
retaining all supported checks; it needs hardware qualification. See
[the tag0156 run record](benchmarks/qwen35-dataflow-tag0156-20261122/RUN.md).

Tag0157 passed the initial graph fetch and two steps, then worker 0 failed in
the first RMS step. The card again recovered and unloaded without reboot. The
next rewrite stages RMS scale records in the NoC-accessible input page rather
than on the worker stack, and adds a receipt for the RMS failure stage; that
fix requires hardware qualification. See
[the tag0157 run record](benchmarks/qwen35-dataflow-tag0157-20261122/RUN.md).

Tag0154's qualification driver SHA-256 is
`152edda1d9214b869eb74299010d1e973da91c6070e7f8f191a4cbb6f597c223`
and its standalone client SHA-256 is
`21e63fed6cdcc05bc3b168d022042245ba3e6eae0c153aaaeb49ad9a23c2f564`.
The normal and non-loadable graph/recovery witness builds also passed. The
worker stack envelope is `7632/8192` and the graph ioctl envelope is
`13856/18432` including the helper allowance; neither is a whole-program proof.

Tag0147's offline fault-recovery work added a bounded graph settlement audit
and a service transition that permits closing but never resumes inference on
the faulted model. A successful audit requires a host-write fence, stable
receipts and backend proofs for both NoCs, queues, worker image, routes, and
graph peer/fetch traffic for all 120 cores before acknowledgements. The
fixture tests failure retention and partial acknowledgements in simulation.
At tag0147 the backend, driver ioctl integration, restored-fleet release and
hardware qualification remained open. The target suite and both Haiku/RISC-V builds pass; the normal
tag0147 driver SHA-256 is
`48ba20fbd4f3118cf1d8a099f37bad8d505a61e3e45eadf3d2dfb5bed337e7ba`,
and the non-loadable graph witness is
`24eca47a2b71b115147bc4263a486960619b20e81827b915e36d67f1c909fa31`.

Tag0148 updates the real non-loadable graph staging adapter to avoid
rewriting unchanged descriptor words on warm runs. It validates the entire
old record and its completed sequence, rejects changes outside the allowed
dynamic words, verifies each changed write, and retains the graph on any
uncertainty. The publisher still reads and compares the complete new record
before any trigger. In a simulated prompt-to-decode transition this reduces
descriptor writes from 128 to seven per core; multiple full-record reads and
the host's per-step metadata replay remain. The target suite and both Haiku
builds pass. The normal driver SHA-256 is
`96c8f9b2d749c54cce873be27d2b6fc184dd3bc4827a1e02c9f48c8d3d83646b`,
and the non-loadable graph witness is
`343a690d5ab7e6a189e1bfbee13df75f7ed53b6988e47494f23464d114f4490e`.
No hardware throughput is implied.

Tag0149 connects that audit to the graph owner's RECOVER request in the
non-loadable build. It checks the owned stream and surrounding context, fences
the last host writes with a same-device read, audits both NoCs and each
worker's stable mailbox and queues, and acknowledges terminal workers only
after the entire fleet passes. A settled session can close but cannot run a
new graph. The service restores and resumes saved worker state before the
retained graph run and candidate are released. Failed or partial audits keep
the owner; later attempts start with another fence and re-audit all workers.
Simulated integration covers audit refusal, retry, all-core acknowledgement,
close and release. The implementation remains non-loadable and has not run
on Pioneer; uncertain outstanding traffic can still prevent recovery. The
target and reload suites and both Haiku/RISC-V builds pass. The recovery
call-path envelope is `6608/18432`, including a 2,048-byte helper allowance;
it is not a whole-program proof. Normal driver SHA-256:
`fdab1921ab9ed3f439ed78d8bef826dc1c881709ed709aae3e8caca6a12b30f6`;
non-loadable graph witness:
`61063443a267b8e7b10e15a57107429e53bac6ee29b92ce4c91e74ea52e615bf`.

Tag0150 builds a distinct graph qualification driver without enabling the
ASIC-reset prototype. Its dedicated tag is `2026112100020150`; a build that
combines the trial and reset-witness modes is rejected. The actual graph
service sections match the non-loadable graph witness byte for byte. Offline
target/reload tests and normal, witness and trial cross-builds pass; no trial
has been installed or run. Trial driver SHA-256:
`aaf92ec23d14be2b62edadf230f1c3fc317e4d2bee67f76647a55cdb814a15f6`;
normal and non-loadable witness drivers:
`385db06798142123d7efe56bbe8f671217f8a9525d1faab455901df8603681f1` and
`2e7c2b32598dfa51ff026fba5750782bfe4a44f764c0cf69aba4b9935ee3a05f`.

An important tag0150 performance limit was its barrier: 548 decode
steps times 119 peers times two blocking release writes makes 130,424
sequential coordinator writes per token. Ten milliseconds per token would
allow roughly 77 ns per write if every other operation cost zero. This is a
source-level count, not a measurement. Continue the rewrite toward the
Tenstorrent-style multicast/semaphore pattern, with explicit proof of the
actual router masks, NoC1 coordinate mapping, receiver set, acknowledgement
accounting and data-before-commit ordering. Do not run a benchmark until the
rewrite is complete.

Tag0151 replaces the serial root release loop with a fixed four-way unicast
tree. The coordinator still gathers all 120 receipts; every receiver checks
its committed release before forwarding to its fixed children, and arbitrary
peer release edges are rejected. All 120 cores remain compute workers. The
NoC1 audit now checks translation for all used worker coordinates at each
sender. Root release sends fall from 130,424 to 4,384 64-byte transactions per
decode token; total unicast releases across the fleet are unchanged. Simulation
covers out-of-order scheduling, partial relay failure, malformed and torn
releases, route restrictions and drift. The target/reload suites and normal,
non-loadable witness and qualification Haiku cross-builds pass, but this is
not a hardware result or a throughput measurement. Qualification driver tag
`2026112100020151` SHA-256:
`d3580922016123209304a033ac25ca56a25b3630688673a26714125d1cdf090a`;
normal and witness drivers:
`ac7945b2b377c2da2ffc0f4eef2b0a48b5dc59ffafe7220c3fe9acb15bcb2bb0` and
`1fa285843b4615fffb638e451df5c1353beaf2f05d1dd19219dc18c00f0beb6d`.

Tag0152 extends the tree to collect arrivals. Every node checks the committed
receipts from its own children, aggregates the earliest failing worker and
sends one receipt to its parent. Worker 0 therefore polls four child receipts
instead of 119 direct receipts at each graph step. Arrival slots are cleared
and checked on every relevant held core, and the transport admits only a
child-to-parent arrival or a parent-to-child release. Failure identities
outside the reporting subtree and releases that erase known failures are
rejected. The target/reload suites, all-core simulated failure and scheduling
tests, and normal/witness/qualification cross-builds pass. These checks do not
measure hardware speed; fleet-wide unicast traffic and host audits remain.
The worker stack envelope is `7632/8192` including a 1,024-byte helper
allowance, not a whole-program proof. Qualification driver SHA-256:
`1d527077af67276ca9024f8011f91f56e9fde8b7f053fe4268000cea041da82d`;
normal and witness driver SHA-256:
`dc31e3e98d3f0fd84e2f44baecb06534d7360f87a8439aaa6e6c2ce2d4dabb70` and
`7d0b349751e8324769fb71400eca5d23a24cd08b846201e5260c8c01fe64c121`.

Tag0153 removes two redundant pre-command full descriptor reads per worker.
The physical staging path already proves the initial descriptor word by word
or compares the whole old descriptor and verifies every changed warm word. A
new graph-session ledger records the exact descriptor only after those checks
and the marker write succeed, requires matching fleet-wide ticket/token and
all 120 staged cores, and permits each command once in order. The session lock,
sealed stream and restricted peer write ranges protect staged records until
publication; the post-run full descriptor audit is retained. By source count
this eliminates 30,720 32-bit MMIO reads per graph, while the ledger adds 61,440
bytes to the aligned, heap-owned graph object. Simulated tests cover stale,
forged, partial, duplicate and ordered publication across two runs. Target
and reload suites and all three Haiku cross-builds pass; there is no hardware
measurement. Qualification driver SHA-256:
`7d2319caa8c40a57913f567a13e1639b8a10f54ab2aa5ceac2bfd8793601f6b5`;
normal and witness driver SHA-256:
`f8a88cffab42bacab19c07c6aa0cf7084e0a2b85a771719dbe956fdd965c6dc0` and
`23b00c9419c017301e7eca6dea9a11218b553c9eb3bf266194a0bcac1992d146`.

After the tag0120 diagnostic exited cold, a single authorized power-off/on
cycle and the verified NVMe Fedora-to-Haiku handoff restored a fresh Haiku
boot. The saved hash-verified recovery script restored stable driver55:
tag `2026102100010012`, generation2, `quiesced0 cold0 active_owner0
open_count1 guard1`. The recovery lock is absent and no inference/dataflow
client is running. The 4B file remains `2740937888` bytes and the preserved
27B file remains `18973870528` bytes. No model diagnostic has run after this
recovery. Details are in
[`tag0124 recovery`](benchmarks/qwen35-dataflow-tag0124-20261122/RECOVERY.md).

Tag0120 failed the first core0 embedding read: NoC0 response counter index36
observed `0x00029066`, expected `0x00029067`; completion mask `0xfe`, busy
initiator mask `0x0`, and outstanding transaction-ID mask `0x00000001`. The
request was address `0x04185380`, route `0x312`, size `3392` bytes. It
produced no token; decode timing is unavailable.

Candidate tag0122 fixes a concrete resident-worker routing defect found by
comparing the Blackhole register definitions with Tenstorrent's NoC code.
Tag0120 used command-buffer `NOC_RET_ADDR_HI` at offset `0x14` as the local
coordinate. That register is mutable command state, not the core identity;
it could route a read response to another core. The worker and host audit now
use `NOC_CFG(NOC_ID_LOGICAL)` at `0x148`, and the NoC commands use marked,
static-VC1 read and nonposted write control fields. See Tenstorrent's
[Blackhole register definitions](https://github.com/tenstorrent/tt-metal/blob/main/tt_metal/hw/inc/internal/tt-1xx/blackhole/noc/noc_parameters.h)
and [NoC initialization and transfers](https://github.com/tenstorrent/tt-metal/blob/main/tt_metal/hw/inc/internal/tt-1xx/blackhole/noc_nonblocking_api.h).
This is a plausible cause of tag0120's missing response, not a confirmed
hardware diagnosis. A host fixture checks the stale-command-register case and
the read/write command fields; the RV32IMF role build and Haiku/RISC-V target
build pass. Tag0122 is not installed or hardware-tested. Its default driver
SHA-256 is `6a4c03b152b01ec623b078755992b8fc342a558af41567e2d8775a1e9b2c9957`;
the service text hash is
`6af17124c1859b34166617577dce72bbbafb06e094146a796546a2650db51089`.

Tag0123 strengthens that fix: the logical ID's upper 20 bits belong to
firmware and are ignored when decoding its coordinates; the NoC0 coordinate
must also match the selected worker before launch. Completion checks ignore
only documented hardware-mutated reserved bits in `NOC_CTRL`, while still
checking the request type, VC and all other software-controlled fields.
Without this, a successful transfer could be rejected after hardware changes
those bits. Tests cover all 120 worker-coordinate mappings, nonzero upper ID
bits, changed control fields, and allowed reserved-bit changes. The authoritative
[Blackhole NoC memory map](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/MemoryMap.md)
documents both behaviors. Tag0123 has not run on the card; in particular,
the NoC1 logical-route and destination-translation assumptions remain to be
verified. Its normal driver SHA-256 is
`5163d79689ce225100913bbc26b4bc8329e9de22642d5641808c805de839130f`.

Tag0124 adds a read-only NoC1 fleet audit before starting the dataflow service.
The previous route proof copied the NoC0 bank routes to NoC1 without checking
the second NIU's translation. The new audit samples each worker twice, checks
its NoC1 identity and idle state, and verifies all seven bank routes against
the independently established NoC0 geometry and the reversed 17-by-12 NoC
coordinate system. It refuses unsupported DDR translation modes and FIFO
state. The NoC1 aperture is read-only and is present only in the dedicated
dataflow build. Simulated tests cover all 120 workers, altered bank routes,
changing registers, busy traffic and invalid identities. The Haiku/RISC-V
target builds pass, but this audit has not been exercised on the Pioneer.
See Tenstorrent's [Blackhole coordinate documentation](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/Coordinates.md).
The tag0124 normal driver SHA-256 is
`3336b78873687f2c491314751e39e0f2942a0bcf0dfccd2d3f9bcdf4f4cc79ea`.

Tag0125 connects the disabled recovery prototype to a bounded ARC state0
exchange on firmware queue 0 after reset and telemetry checks. The transport
uses fresh mappings, validates queue ownership, and retains the mappings and
cold latch if publication or completion is uncertain. Simulated tests cover
pointer wrap, every dropped queue write, timeout, bad replies and partial
mapping; the existing 27B application-queue tests still pass. The state0
operation follows Tenstorrent's [Blackhole initialization](https://github.com/tenstorrent/tt-kmd/blob/main/blackhole.c)
and [firmware queue protocol](https://github.com/tenstorrent/tt-kmd/blob/main/msgqueue.c).
This is only one part of firmware reinitialization and has not run on the
card. Full PCI/DBI restoration, remaining firmware setup, resumable failure
handling and physical qualification remain open. The normal driver SHA-256 is
`fc736ae48530f37785f11ca2be56626077e549cae05e3a321db841ecd9caa366`;
the enabled prototype remains non-loadable and is used only to compile-check.

Tag0126 connects the DBI payload snapshot and restore to the non-loadable
recovery witness. The healthy-session adapter checks the fixed PCIe identity,
BARs, NoC route and status before mapping the control register read-only. The
post-reset path audits both NoCs across the held fleet and the pinned firmware
HDMA, then uses a fresh writable mapping and restores its fresh TLB snapshot.
The owner and mapping are retained on uncertainty. Fake-device tests cover
the real session adapter, mapping and recovery orchestration, including lost
writes, route changes, busy DMA and failed TLB restore. Normal and non-loadable
Haiku/RISC-V builds pass; the dataflow stack bound remains `15712/18432`.
The normal driver SHA-256 is
`51b8ccacdb2efaa1a7d65588040aa9e8a835fe265463d0eedb48c331660faad3`;
the non-loadable witness is
`43bd02e3763c49866f24ebbd9fbef9236ea0edb649d82c914f7981ec0975979f`.
This remains untested on Pioneer, is not complete PCI/firmware recovery, and
does not establish a new decode rate.

Tag0127 removes a repeated per-block 576-byte activation copy in the resident
projection worker. It passes the exact packed L1 cache chunk directly to the
existing Tensix code, with bounds tied to the format, input width and block.
ASan/UBSan tests check all bytes and chunk reuse for Q8_0, Q4_K, Q5_K and Q6_K;
the RV32IMF worker compile and normal and non-loadable Haiku builds pass. The
normal driver SHA-256 is
`f6c6ef24e5e41bbcd97a960cf85e49f458776655e1718af7eb1a4aedbeeee4e9` and
service text SHA-256 is
`529c47979f45426c2114241e7063b2e74fc03403a296cb0afd4a19fbe8492ff9`.
There is no physical correctness or throughput evidence for this change.

The host still performs a separate all-core dispatch lifecycle for each of
548 resident operations per generated token. This source-level count is not
a timing measurement. Moving the checked graph and synchronization onto the
device is the major outstanding architectural work toward 100 tokens/s.

Tag0128 starts that work with a device-side 120-worker barrier and restricted
NoC1 peer transport. Worker 0 continues to compute, then collects one receipt
per peer and distributes a release. Two ordered transfers separate each
payload from its commit marker; stale/torn markers, timeouts and partial sends
fail closed. A partial release can permit earlier peers to advance, so the
entire graph must be retained on uncertainty. The barrier's fixed L1 slots are
within each core's existing saved snapshot; clear/readback is included only in
the non-loadable device-dispatch witness. Simulated tests exercise all workers,
failure selection, stale/torn records, send failures, held-core setup and L1
restore. RV32IMF compilation of the real transport has no atomic instructions
or runtime helpers. Normal and non-loadable Haiku builds pass; their driver
SHA-256 values are `6ae463bb56390e8e1693c57bbdf9b11684b54742e74b379e598d322e1af8b4b7`
and `dfa3472ad3714c1276bc88e072185c9a3c8574278eb16dce41e466c89f6de1e1`.
The worker service does not yet invoke this barrier; queued descriptor fetch,
epoch ownership, timeout/fault integration and hardware qualification remain.

The queued rewrite now includes a 2 KiB per-core descriptor record, a
bounded NoC0 fetch into the idle second weight page, and a planner for 120
non-overlapping streams in the packed model workspace. Fetch checks record
identity, sequence, core, size and an accidental-corruption checksum; it
invalidates the old command before reading and publishes the new magic last.
Synthetic tests cover actual embedding/projection records, all stream leases,
workspace overlap/capacity, malformed records and read failure. The concrete
fetch compiles for RV32IMF with no runtime helper or A-extension instruction.
The checksum is not authorization: trusted kernel lease validation, typed
model-bound command authorization, immutable storage, dynamic token/position
handling, worker execution, and hardware tests remain to be implemented. A
full 548-record stream for all cores occupies 134,676,480 bytes; rebuilding it
for each token would create substantial transfer overhead, so fixed records
must be reused and only dynamic fields updated.

The primary worker has only 2,892 bytes of unused code space. A separate
offline linker witness places the existing 54,452-byte primary image at
`0x72000` and the 7,308-byte stream/barrier compile probes at `0xa0400`,
before the invocation slot at `0xaa000`; both fit and have no unresolved
imports. The entire range is covered by the existing L1 snapshot. Tag0129's
non-loadable witness now writes and reads back both segments while the fleet
is held, includes the second segment in image verification and accepted
program-counter ranges, and prohibits writes to it while cores are running.
Simulated tests exercise dropped writes, failed and corrupt readbacks and the
held/running access boundary. The second segment still contains compile probes,
not the actual queued service, and no tag0129 image has run on Pioneer. The
normal driver SHA-256 is
`8f7a25dc4b2cbddeaf8b6d691a9342386f7e8e5e4c9d565f7b3e766f4ca5eb31`;
the non-loadable witness SHA-256 is
`22f4d211d27e3ae0a49c6040a6a3b02871b55e94c98d2fc1997f85f8f6248072`.

Tag0130 extracts execution of one local command from the service loop for
reuse by the future queued worker. The existing service still supplies the
command and retains its publication, completion and acknowledgment protocol;
the new executor does not itself authorize queued commands or coordinate the
fleet. The RV32IMF image is 54,536 bytes, the synthetic role tests pass, and
both normal and non-loadable Haiku builds pass. Their driver SHA-256 values are
`66fc673b05a400cf090de5d621840c1ef73a42c68b1222b1d7c21c24d766d75d`
and `c9b51667a271c350701729e240af5f90af06d50ddcb7e0a7b8249635d15a27e8`.
This has not run on the card and does not establish a decode rate.

Tag0131 adds a per-worker control loop for stream fetch, exact-command
authority, execution and fleet barrier. Bounded polling and any uncertain
step retain the whole graph; local completion is not fleet completion. A
120-worker simulation uses real encoded embedding records, the stream fetch
and the barrier, with fake execution and authority. It covers successful
two-step execution, rejected authority, failed reads, uncertain execution,
reported worker failure, partial release, queue failure and deadlines. A
non-loadable RV32IMF witness links the loop with real NoC transport and the
worker executor, but its authorization hook rejects every command and the
service never calls it. Its primary and dispatch sections use 54,652 and
8,164 bytes respectively. Normal and witness builds pass; their driver
SHA-256 values are
`9046b4e81acb393102d79c6c4c1b533819cde9f1b0b2e579393fff7caa5e947c`
and `8d40af3f685cb64b584da5841cc8faa86cfb7c6f46b28d4b677f6b596e19d362`.
The kernel must still authorize and protect the exact stream, bind fresh
epochs, update dynamic fields efficiently, audit fleet completion and report
faults. Nothing here establishes correctness or speed on Pioneer.

Tag0132 adds a separate metadata-only stream recorder. It clones the checked
model authority into an isolated planning candidate, runs the existing full
decoder sequence, binds each program to all 120 cores, and emits encoded
records to a write/readback/seal interface. The candidate is speculative and
must not be promoted to live state without a full physical audit. A synthetic
full-shape model test produces 548 steps for two positions and covers all nine
invocation kinds, forged worker IDs, failed writes and failed begin/seal. It
uses fake storage and performs no inference. The test caught a real stream
format bug: RMS stores its core field at a different offset; validation now
uses each invocation's declared offset. A separate RV64 kernel-shaped planner
compile reports 3,488 bytes as its largest single frame, not a call-chain
bound. Storage ownership, sealing, dynamic updates, full stack audit and the
device authorization hook remain open; the recorder is not linked to either
driver. Normal and non-loadable tag0132 drivers build successfully, with
SHA-256 values
`b082707b8dafdfc3e2e76bd2b99349d7d02a6bd4a7180114ee54ff71448aa1b4`
and `3ffaf3a86e85d45aaa3a067f4d9f9ffc2a0d8e410c89b1e9ca2a65c9db60c313`.
No new hardware measurement exists.

Tag0133 adds a non-loadable private bank-page sink. It reconstructs workspace
bounds from the already-validated live manifest, validates the stream plan,
claims 120 disjoint uninitialized ranges in the bank owner, and buffers two
records per page. The existing page transfer writes and reads back each full
page; an odd final record is zero-padded. Generic bank access cannot enter a
claimed range, the owner cannot write it after sealing, and an uncertain
transfer quarantines the bank owner. Tests exercise the actual bank resolver
and transfer using simulated hardware, including partial/failed transfer,
overlap, occupied pages, incorrect ownership and sealing. The normal seven-bank
control tests and both Haiku builds pass. The adapter is compile-only and no
recorder is connected to it. Other mutation paths, ownership across the
entire graph, fresh epochs, efficient token updates, completion audit and
hardware qualification still need work. Normal and witness driver SHA-256:
`761d8c9458d7f8d80a2ccfbde53987129edbb76ff203f0d9ebc659cf08152108`
and `b717dfff0fee41c63f7fc6861e483a47ff0edadccd85e4c1fe157e832aabf1a4`.

Tag0134 composes the real model-bound recorder, workspace validation and
private page-transfer sink in the non-loadable driver witness. A synthetic
548-step/120-core test reaches the real bank/store implementation against
simulated hardware and checks first and last records on every core; prompt-only
planning produces 545 steps. Public bank/span and legacy uploads and old
Qwen35 dataflow actions are gated once the claim exists. The actual bank
control test covers denied outside-lease uploads. Other diagnostic and cleanup
paths still require an ownership audit. The combined build's conservative
recorder stack envelope is `17408/18432` bytes including a 2,048-byte
allowance, without a future ioctl caller; the witness's normal dataflow envelope
is `18160/18432`. There is no public graph entry, retained session lifecycle,
device authority, final fleet audit or physical qualification. The current
32,880 page writes per token also make fixed-record reuse essential. Normal
and non-loadable driver SHA-256 values are
`8966cc4906c61d36dfc8c76fec02c1917f034a190bbaffba0426d41b93717dc5`
and `a41ba66116553b520bc08ca37d152faad379e5687f9142b422eb582b6aae8514`.

Tag0135 prototypes fixed-record reuse. The worker fetches and checks a sealed
template, patches token/embedding, position and recurrent ping-pong fields in
local staging, then publishes the command. Synthetic full-shape tests compare
every core and operation against freshly generated authorized records for
positions 1 and 2 and an embedding-shard change; typed-builder comparisons
cover the last context position and last vocabulary token. Failed patches do
not publish command magic. The non-loadable RV32IMF worker witness links the
dynamic fetch but its graph authorization hook denies execution. Its primary
and dispatch code use 54,652 and 9,996 bytes.

The template epoch is now separate from per-run barrier epochs and command
sequence ranges. RMS and argmax receipts also require fresh sequences; an
unbound ticket allocator prototype issues non-overlapping ranges and rejects
exhaustion. Neither tickets nor the trusted dynamic run context are connected
to an owned live session, and fleet completion, failure retention, clean
release, mutation-path audit and hardware qualification remain incomplete.
The normal and non-loadable tag0135 driver SHA-256 values are
`90ef29e142c91a321b32d48dd2a99ae464345969ccb5c1bc3f2dce9dbe752d67`
and `93fd0da9854f76674f12980c0d26288458e1d8701dea0057daf3257e724ffe27`.

Tag0136 adds a 512-byte graph-run descriptor in each worker's saved L1. A
non-loadable resident worker recognizes the graph marker and enters the
graph loop with a checked local descriptor copy. The host publisher checks
all workers, stages and reads back all 120 descriptors and markers, then
issues triggers; partial publication is retained as a graph fault. Simulated
tests cover malformed descriptors, failed readback, partial triggers and
idle-only descriptor writes. The graph authorization hook still denies all
commands and no live session invokes the publisher. The worker's conservative
stack envelope is `7824/8192` bytes including a 1,024-byte helper allowance;
it is not a whole-program proof. The recorder envelope is `17424/18432` with
a 2,048-byte allowance, excluding a real ioctl caller. Run ownership, trusted
ticket/context binding, completion/failure audits, cleanup, the remaining
mutation paths and hardware qualification remain open. Normal and non-loadable
tag0136 driver SHA-256 values are
`7e89661ffae9792945a07b4583a2de876e588e0653ec084d2e1666d3e15e2951`
and `7984c3bd895bf9341289537ed2d1f6128a73a1ad479e55b74df860fca64fcf03`.

Tag0137 fixes a repeated-run readiness check: a fully acknowledged worker may
have a nonzero command sequence, so requiring the startup mailbox's zero
sequences rejected a ready worker after its first command. The new check
requires an idle mailbox matching a stable ready snapshot, and rejects stale
snapshots, shutdown and unexpected reserved fields. This is a code-level fix;
it does not establish the cause of tag0120's embedding failure.

A non-loadable graph completion prototype now waits for all 120 terminal
workers, audits their descriptors, final commands, runtime and NoC state, and
checks a stable argmax for decode before acknowledging any worker. It retains
the graph on incomplete, inconsistent or partially acknowledged results.
Simulated decode and prompt tests cover success and fault cases. The actual
driver's ownership gate still rejects every graph, and completion does not
commit model state. A real session must establish immutable model and ticket
ownership, connect authorization and completion, commit model state exactly
once, and implement safe cleanup before hardware use. The normal and
non-loadable Haiku/RISC-V builds and the resident-bank regressions pass. Their
driver SHA-256 values are
`d11e3806d24913a1b248d612b0b06dd78413b3f324009a342464c1b83f31c9ea`
and `4d5bddfc3b5f5e576c00ee585a24c31acfbd8af67da0af184271d34cdca52dbc`.
The non-loadable build's reported conservative kernel stack is `18160/18432`;
the existing recorder and worker envelopes still exclude a real caller and
whole-program proof respectively. This work has not run on the card and
provides no throughput measurement.

The next offline component replays the graph's authority transitions into a
private candidate without regenerating the 120 per-core records. Simulated
prompt and decode tests compare the candidate with the full recorder for two
successive positions and verify that the original authority remains unchanged.
The standalone Haiku/RISC-V kernel compile passes; its largest individual
frame is 3,536 bytes, not a call-chain stack bound. This component is not
connected to the driver or allowed to promote state, and does not prove device
execution or measure decode speed.

Tag0138 makes one sealed 548-record decode template usable for prompt tokens:
the descriptor carries a 545-step prompt ticket or a 548-step decode ticket,
and each device worker executes exactly that many records. This avoids a
second protected stream claim and upload. Full-shape simulated recording
compares all 120 cores' first 545 records against independently recorded
prompt programs at positions zero and one, including a dynamic token patch;
the fleet simulator also checks prefix termination and count bounds. Before
staging any worker, publication now requires the ownership callback to bind
the sealed stream, model, token, ticket and fixed activation slot. The real
callback still denies all publication; recording that fixed slot in the live
session and establishing the immutable ownership proof remain necessary.

The target test suite and normal and non-loadable Haiku/RISC-V builds pass.
The normal and non-loadable driver SHA-256 values are
`5996e2b386eea9c8df5660830186458cc2ab98e17e57ed6f76468126b03640c6`
and `f56bd0561a08005dd5e80a77bb98fc0ffe3dcc812879f4af6c316aa431f71ae6`.
The non-loadable kernel stack estimate is `18160/18432`, and the worker
envelope remains `7824/8192` with a 1,024-byte allowance, not a whole-program
proof. This change is not hardware-qualified and gives no throughput result.

Tag0139 adds a read-only sealed-stream identity proof. The recording witness
retains its template slot and audited routes; the sink compares the complete
plan against its recorded copy, and the bank ledger checks the exact owner and
120 claimed ranges, sealing, current baseline and quarantine state. Simulated
tests reject altered ranges, changed stream bindings, a different owner, a
drifting baseline and quarantine. This proves only stream identity: the
future session must separately prove service safety, model identity, fresh
ticket ownership and model-state commit. In particular, the existing general
`Unsafe()` check treats an installed service as unsafe and cannot by itself
authorize a resident graph. The publication callback still denies all runs.
The target suite and 27B shared-bank regression pass, as do normal and
non-loadable Haiku/RISC-V builds. Driver SHA-256 values:
`305bed311286cb667932385e755129f7dfe08be450779d08a11d20fc71d21236`
and `55d17dccbe446ac1adea65ff7738933b5dc241463266552a3ddb7df385db841c`.
No card run or speed measurement has been made.

Tag0140 adds a graph phase to the resident service in the non-loadable
dispatch build. It reserves the next worker-mailbox trigger, binds one
publisher, blocks normal work and close while the graph is in progress, and
adopts the new service sequence only after that publisher's completion has
been audited and the model-state commit callback succeeds. An inert preflight
may cancel with the same publisher; staged or uncertain graphs retain a fault
and cannot use the old projection-recovery or close paths. The simulated test
exercises consecutive prompt/decode graphs, duplicate commit, wrong-publisher
cancellation, an unaudited result, partial staging and commit failure. The
real driver commit callback remains deny-all: there is no live candidate
promotion or complete graph-fault recovery yet. The service owner must retain
the publisher and all graph resources through the fault.

The graph lifecycle test, full target test suite and normal/non-loadable
Haiku/RISC-V builds pass. Normal and non-loadable driver SHA-256 values:
`e90017ffe37b80e4e8f3ffa20318aaa5c7a929cf114010958b069f7eabb305cc`
and `df9177d08b62e3989ba3b23edf41db799e3b8a4fbf92e9cb15802cc54b635788`.
The graph worker and recorder stack envelopes retain the previously documented
limitations. This is simulation and compilation only, with no decode result.

Tag0141 adds a session-lock-scoped model transaction for the queued graph.
It builds a private candidate from the current model, binds the exact token,
ticket, activation slot and publisher, and captures compact snapshots of all
mutable metadata for both original and candidate. Promotion swaps the live
authority pointer exactly once only after the matching publisher is audited
and both snapshots still match. The retired authority stays owned by the
session until references are ruled out. Static layout and routes are immutable
after binding and still require the separate sealed-stream/session proof.
The simulated service test connects this transaction through a prompt and a
decode step; other tests reject forged token layout, changed live or candidate
state, wrong publisher, missing audit and repeated commit. No tensor work is
performed by this host metadata transaction.

The real driver's publication and commit callbacks still deny execution: the
transaction, its candidate, ticket issuer and sealed recording are not yet
owned by an actual driver session, and device authorization and graph-fault
cleanup remain open. The target suite and normal/non-loadable Haiku builds
pass. Normal and non-loadable driver SHA-256 values:
`9207eb940020793ef91e0510f7fdb8051cde16aa5854003d3f2ca3d6ddd06eb1`
and `a2abbd831a7841ed3325e4af7f289d29e845c1ff020314c59e353a7d0cd6656c`.
The new model-planning stack envelope is `9472/18432`, including a 2,048-byte
helper allowance but excluding a real ioctl caller. No hardware or speed
result exists for this change.

Tag0142 composes the sealed stream, fresh ticket issuer, resident service,
publisher, all-core completion audit and model transaction into a synchronous
graph-session coordinator. One recording can be claimed by only one session,
preventing a second issuer from reusing its barrier epochs. The coordinator
returns output only after the state commit; uncertain publication or audit
retains the run, candidate and service fault. Simulated tests exercise two
successive prompt/decode runs, duplicate stream claims, failed preflight and
an audit fault. A non-loadable driver witness now connects this coordinator to
the real recording's bank proof and the PCI transport. Its controlled context
safety check permits only this ready/running dataflow service while preserving
the other `Unsafe()` checks; it also verifies the runtime owner, cold/reload
state and seven-bank model session. This code has not run on the card.

There is still no actual session allocation, public graph request or retained
object teardown, and the device's per-command authorization hook denies all
commands. Those must be implemented before a hardware trial. The target suite,
27B shared-bank regression and both Haiku builds pass. Normal and non-loadable
driver SHA-256 values:
`60f47279614f341405f471e35dcdf578d7cf0d16124676ab2eebc919447378cd`
and `d5d1959ceadfa18e2ca0577747334dd34ea2fd8eeae04e5c7057a962e4d9ca66`.
The coordinator stack envelope is `13552/18432`, including a 2,048-byte helper
allowance but excluding the future ioctl caller. No decode rate is available.

Tag0143 replaces the non-loadable worker's unconditional graph-command denial
with a local fetched-command consistency check. After the descriptor has been
validated and the sealed record fetched and patched, it checks the record's
core, step, structure and checksum, then compares the staged invocation with
the published L1 program before calling the typed Tensix worker. The check is
not model authorization: that depends on the host's validated recording,
disjoint sealed bank claim, private descriptor and session ownership. There is
still no live session allocation, public graph request, or fault-safe graph
teardown, and no card run has established the memory/route assumptions. The
dispatcher has moved from the test directory into the rewrite sources.

Simulated tests cover corrupt headers and payloads, altered program words,
wrong core/step, prompt bounds and all nine invocation families from a
dynamically patched recording. The target suite and both Haiku builds pass.
Normal and non-loadable driver SHA-256 values:
`ae683b12983e8fd759a0231f6216b6f85342fe72220dd76ede07352ea243f3d4`
and `48e65c1b0c8a52e92be259c8c997b3b85f01069174fe78d7d08a9f8737b43eb5`.
The worker witness has a `7792/8192` stack envelope including a 1,024-byte
helper allowance; it is not a whole-program proof. These checks add work per
command and have not been benchmarked.

Tag0144 adds a context-owned graph lifetime in the non-loadable driver. It
retains the recording and session, allocates each run and candidate, transfers
the candidate into the live model only on success, and retains uncertain runs
instead of freeing their resources. The context's ordinary unsafe check now
also catches that retained graph. Normal successful runs free their run object
and retired authority under the session lock. A simulated allocator/runner
test covers allocation failure, two promotions, retained fault and release
only after a simulated restoration proof. The real graph-restoration proof
still denies release, and graph-fault recovery without reboot is not complete.

The kernel lacks C++17 aligned delete, so 64-byte-aligned graph objects now use
a checked placement allocation with matching destruction; the ordinary model
authority keeps its existing new/delete convention. The allocator test, target
suite, shared 27B bank regression and Haiku builds pass. Normal and non-loadable
driver SHA-256 values:
`d35020cfd9200901d585f9f163e44a1d4ea3536273f301f931178819a966d2d6`
and `f56e29a6a6ddb5e29b580a247a2c95fec5f343f7207d4d5ec201f5e320eafa00`.
The recording and graph-run stack envelopes are `17552/18432` and
`13744/18432`, respectively, including 2,048-byte helper allowances and
excluding a real ioctl caller. Recording, binding and running this owner are
still compile witnesses, not a public generation path or hardware result.

Tag0145 connects the context-owned graph to the existing install and generation
requests in the non-loadable dispatch build. Install records a sealed decode
template for activation slot 0, starts the resident service and binds the
graph session. Generation uses only the graph runner: prompt tokens use the
545-step prefix, the last prompt token and decode tokens use 548 steps, and
there is no host tensor inference fallback. Prompt time includes the first
selected token; decode time covers the remaining output tokens, matching the
client's `(output_count-1)/decode_seconds` calculation. Recording is included
in model-load time. Requests for another activation slot are rejected.

Simulation covers token ordering, EOS, invalid output, separate timing, two
warm requests through the owned runner and retained audit failure. The target
suite and normal/non-loadable Haiku builds pass. Normal and non-loadable driver
SHA-256 values:
`813d44299bb77faeb02bc3d45f805afbe4d499fdd9456f87aeec5bfa22557582`
and `6d4d5c72f94fc90187df7889b4cf6022ef2dcc7b0632af991e42e746e1c149d7`.
The measured install and generation call-path envelopes are `16400/18432`
and `15872/18432`, respectively, each including a 2,048-byte helper allowance;
neither is a whole-program stack proof. The driver remains non-loadable pending
fault recovery, teardown review and physical qualification. No card run or
decode throughput exists for this path.

Tag0146 connects retained graph heap objects to the existing non-loadable
ASIC-reset recovery prototype. Their release is allowed only after the same
locked recovery ticket has proved reset, firmware readiness, all 120 cores and
NoCs quiet, host DMA idle, the temporary mapping closed and the old owner's
mapping disposal completed. The release is CPU-only and occurs before PCI
unreserve; a failed proof or release keeps the cold latch. Mapping disposal
now records completion separately from an attempted or partial disposal.
Simulated recovery tests exercise the graph-release ordering and refusal, and
the existing failure-injection suite keeps the owner/cold latch on earlier
failures. This is not yet an in-place recovery guarantee: the ASIC-reset and
firmware sequence is non-loadable and unqualified on Pioneer, ordinary graph
faults still lack a bounded no-reset teardown, and no hardware trial occurred.

The recovery tests, target suite and both Haiku builds pass. Normal and
non-loadable driver SHA-256 values:
`44817035ee8ff1728fbf5052c1711c288858727cbbd2aee9dbc70058fd8db93f`
and `48ed831cec7bf16e01ac42d9965d1539683e81c7d678307b8b8ee7f6aec0b3d3`.
The graph generation call-path envelope is `15920/18432` with a 2,048-byte
helper allowance, not a whole-program proof. No decode rate is available.

Offline recovery-guard work now defines a 48-byte proof requiring ASIC reset,
post-reset firmware readiness, restored PCI state and mappings, all 120 cores
held, idle NoC and host DMA, and released retained owners. The resident guard
requires a ticket tied to the fault epoch, so a concurrent fault denies
recovery. The extracted tests pass, as does the full Haiku/RISC-V target
build (`15712/18432` kernel-stack audit); recovery guard hash after its atomic
fault-state and module-path fixes:
`8b29c57ac8b47f1c6218dcdf31733cd736f63727ad0527daae3cd12e4fc906aa`.
An isolated Blackhole reset helper now encodes the exact three PCI config
writes used by Tenstorrent's KMD, rejects bus mastering or a stale reset marker,
and waits a bounded number of times for the marker to clear. Its fake-PCI
ASan/UBSan test passes with the reload-guard fixture. A prototype ioctl
connects it to the recovery sequence but is disabled by default. Complete
PCIe capability restoration, remaining firmware initialization, physical recovery,
and silicon validation remain open. A compiled PCI restoration helper
reestablishes the fixed active BAR and bridge layout after the reset marker
clears, while refusing bus mastering, an uncleared marker, unsafe interrupt
state, and unexpected identities. Simulated PCI tests exercise the real BAR
plan, dropped writes, and decoder ordering. It does not by itself restore all
PCI capabilities or establish firmware readiness. The currently loaded legacy guard
is `B_KEEP_LOADED` and cannot be upgraded in place. The completed clean boot
still loaded that legacy guard; after staging the new guard, another fresh
Haiku kernel boot through the NVMe/kexec path will be required to activate it.

The post-reset mapping now polls ARC scratch readiness with a bounded wait,
reads the fresh firmware telemetry table, and requires the pinned firmware
bundle and harvesting values before the fleet audit. Simulated tests cover a
delayed boot, timeout, ARC error, wrong firmware, interrupted wait, and TLB
restore failure. This confirms only the selected scratch and telemetry
contract; the queue state0 exchange is now wired to the disabled prototype,
while the complete recovery lifecycle still needs hardware qualification.

The post-reset audit checks both NoC banks on all 120 cores twice, with
explicit read failures and busy/outstanding transaction checks. A dedicated
mapping implementation validates PCI BARs, captures the TLB only after reset,
checks the fresh topology and each selected core's identity, and restores the
post-reset TLB value before releasing mappings. Simulated-MMIO fixtures cover
both networks, PCI/BAR rejection, partial map failure, wrong core identity,
and retained mappings on failed cleanup. The implementation is wired into the
default-disabled prototype but has not been exercised on the card. A single atomic
word publishes the guard's fault epoch and cold latch together; an injected
fault during latch clearing leaves the previous generation recoverable. These
checks do not qualify a physical reset.

The post-reset mapping backend now also checks both PCIe endpoints for pending
transactions and errors, and checks the pinned firmware's read/write HDMA
channel 0 enables and states in three stable samples. It requires the fleet
audit first and restores a TLB value captured after reset. Simulated MMIO tests
cover busy DMA, a changing DMA state, pending transactions at either PCIe end,
and restoration. This evidence is limited to the pinned channel-0 firmware
contract; the prototype connects it to owner release but remains disabled by
default and has not been tested on silicon.

The failed owner retains its cookie, PCI reservation, snapshots and
quarantined host buffers after the client exits. The recovery operation must
distinguish a final-freed owner from a still-live file descriptor and revoke
old generation access before retiring those resources. Host buffers must stay
wired until the reset, PCI pending-transaction and host-DMA idle proofs pass.
The PCI, mapping, DMA audit, guard, firmware and ownership steps now have a
single prototype recovery operation. The default driver refuses that
operation before any card access: complete PCIe capability restoration and
remaining firmware initialization still need implementation and the sequence
has not been hardware-qualified. An extracted test runs the real
operation body against simulated hardware and injects a failure at every stage;
a separate test verifies the default gate performs no hardware action. A
separate compile-only build includes the prototype but cannot load as a driver.
Failed recovery attempts remain cold and are not yet resumable.

The prototype now captures the endpoint PCIe maximum payload and read-request
settings twice before reset and restores those configuration-space fields
afterward with bus mastering disabled. Simulated tests cover malformed or
cyclic capability lists, invalid sizes, dropped writes, and recovery refusal.
This does not yet restore the device-internal DBI payload shadow or the rest
of PCI capability state. Tenstorrent's Blackhole driver saves and restores its
internal payload value and reinitializes hardware after reset; see its
[Blackhole implementation](https://github.com/tenstorrent/tt-kmd/blob/main/blackhole.c)
and [reset path](https://github.com/tenstorrent/tt-kmd/blob/main/chardev.c).
The new path is included only in the disabled prototype and is not qualified
on the card.

Dedicated candidate tag0121 now records the retained owner's final file
descriptor free while holding the driver lock. A read-only status ioctl
reports the owner-lifetime prerequisites only when the exact driver tag and
generation match; it refuses the lifetime condition with extra open
descriptors, a live owner, or a quiesced driver. Extracted real close/status
fixtures and the target build pass. This status does not initiate a reset,
release any owner or host buffer, or grant hardware recovery. Tag0121 is not
installed; its historical default driver SHA-256 is
`35b04d1f2b2f4654e615a6b343bda8dcbb443c4455eb6178f42c6e73cb29906a`
and its measured stack bound is `15712/18432` bytes. Stable55 is now loaded
and idle after the authorized cold recovery.

The recovery guard export now uses a name beneath the existing reload-guard
image path. Haiku's module resolver stops at the first regular file, so the
previous distinct path would not have found the shared pinned image. The
module fixture verifies both names resolve through the same image path.

A post-audit PCI teardown restores the original fixed idle BAR and bridge
state without calling the pre-reset mapping cleanup, which could restore stale
TLB contents. Simulated PCI tests exercise invalid snapshots and dropped
writes. The teardown is connected to the default-disabled recovery prototype
and is not qualified on the board.

Tag0121 also contains an in-kernel retirement method for the dedicated
dataflow session's mapped areas and host buffers. It requires the reset,
firmware, PCI, core, NoC and DMA evidence with exactly one retained owner,
rejects duplicate area IDs, and releases host buffers only after other mapped
areas. On a deletion failure it retains the remaining areas and rejects a
repeat attempt. Simulated OS tests cover invalid proof, duplicate IDs, release
order and partial failure. The prototype ioctl calls it only after its other
checks; the ioctl is disabled by default and has not run on the Pioneer. No
retained owner or buffer has yet been freed by it.

No valid output, two warm runs, clean unload, or decode-rate measurement has
been established; 100 tokens/s remains unmet. The two-warm benchmark and
assistant integration stay gated until the rewrite and physical recovery path
are complete. See
[`tag0117 RUN`](benchmarks/qwen35-dataflow-tag0117-20261121/RUN.md),
[`tag0118 BUILD`](benchmarks/qwen35-dataflow-tag0118-20261121/BUILD.md),
[`tag0118 RUN`](benchmarks/qwen35-dataflow-tag0118-20261121/RUN.md),
[`tag0119 BUILD`](benchmarks/qwen35-dataflow-tag0119-20261121/BUILD.md),
[`tag0119 RUN`](benchmarks/qwen35-dataflow-tag0119-20261121/RUN.md), and
[`tag0120 BUILD`](benchmarks/qwen35-dataflow-tag0120-20261121/BUILD.md) /
[`RUN`](benchmarks/qwen35-dataflow-tag0120-20261121/RUN.md).
Tag0109 passed guarded preflight and uploaded all
`2,740,498,432` resident bytes. Authentication took `100.467732` s, prompt
tokenization `13.243073` s, and model load/upload `334.616263` s. Generation
ioctl `0xAF56` returned `EINVAL`; the client diagnostic printed the request
fields, but no prompt processing, valid output, decode timing, or tokens/s was
produced. Offline inspection found the cause: `ProbeControl` routed Qwen35
operations only through `RESIDUAL`, omitting the FFN-through-generation ioctl
range. The generation command never reached its validation handler. Unload
also failed at stage9/core0, resume phase3, with resume detail2
(`DataflowServiceResumeIdleProofFailed`). The client exited; the guarded
wrapper retained the exclusive lock and stable-driver backup while the
candidate reported generation3 `cold1 active_owner1`. The guarded recovery
helper restored stable driver55 at generation2; the lock is absent, and
postflight confirmed both model sizes, 64 CPUs, and full RAM. Run details and
raw evidence are in
[`benchmarks/qwen35-dataflow-tag0109-20261121/RUN.md`](benchmarks/qwen35-dataflow-tag0109-20261121/RUN.md).

Tag0110 routes the complete ioctl range, adds compile-time range checks,
reports returned request fields and precise restore subchecks, skips the
running-idle proof for the fully restored reset-held root, and retries
recoverable close failures while the owner retains its snapshots. Its full
offline variant-60 build passes; owner and PCI-memory ASan/UBSan fixtures and
the RV32IMF compile pass. The guarded hardware run uploaded the full model,
then failed at the first prompt token in the embedding operation
(`position0`, `DataflowDecoderEmbeddingRunFailed`). No valid output or decode
rate was produced; close failed the core0 stop audit. Recovery restored stable
driver55 generation2 on Haiku, cleared the lock, and confirmed both model
sizes intact. Continue the rewrite offline before another benchmark. Run and
recovery evidence are in
[`benchmarks/qwen35-dataflow-tag0110-20261121/RUN.md`](benchmarks/qwen35-dataflow-tag0110-20261121/RUN.md).
Tag0110 build details are in
[`benchmarks/qwen35-dataflow-tag0110-20261121/BUILD.md`](benchmarks/qwen35-dataflow-tag0110-20261121/BUILD.md).

Tag0111 added worker-level embedding failure receipts to QUERY/RECOVER,
abortable pending embeddings after the all-core audit, and same-owner recovery
before service close. It built and passed its focused offline tests, but its
guarded hardware trial panicked in generation. Offline stack-usage evidence
showed a 194800-byte FFN frame and a 75808-byte projection-builder frame,
among several others larger than the whole 20 KiB kernel stack. The card was
recovered to stable55 generation2 idle and the 4B/27B model files remain
intact. The trial had no valid output or decode-rate result; see
[`benchmarks/qwen35-dataflow-tag0111-20261121/RUN.md`](benchmarks/qwen35-dataflow-tag0111-20261121/RUN.md).

Tag0112 reuses a heap-owned projection descriptor across the layer sequences,
keeps the 120-core schedule in the heap-owned authority, and builds directly
into the supplied descriptor without a large temporary copy. The offline
variant-60 driver/client/service build passes. The builder checks the measured
kernel stack chain and rejects any Qwen35 frame over 8192 bytes; the
conservative chain is 15696 bytes against an 18432-byte budget. Seven focused
ASan/UBSan tests pass, and the RV32IMF compile has no A atomics or unresolved
runtime helpers. Tag0112's hardware run uploaded the model, then failed during
the first prompt embedding: core0 reported worker state4, reader failure1,
phase5. The lower-level cause was not exposed by that receipt. QUERY/RECOVER
returned pending and service close failed its core0 stop audit; a cold recovery
restored stable55 generation2 and preserved both model files. There is no valid
output or decode rate. Build details are in
[`benchmarks/qwen35-dataflow-tag0112-20261121/BUILD.md`](benchmarks/qwen35-dataflow-tag0112-20261121/BUILD.md); run details are in
[`benchmarks/qwen35-dataflow-tag0112-20261121/RUN.md`](benchmarks/qwen35-dataflow-tag0112-20261121/RUN.md).

Tag0113 added worker-level embedding failure codes and recovery audit
stage/core reporting while preserving the fixed worker mailbox and ioctl
request sizes. It reproduced the first prompt embedding failure on core0:
failure7 (`EmbeddingWeightRead`), phase5. The all-core runtime audit failed at
stage6/core0, QUERY/RECOVER remained pending, and CLOSE failed at the core0
stop audit. The guarded wrapper retained the lock and backup when the driver
reported `cold1 active_owner1`. Its run record has the distinct authentication,
vocabulary, model-load, and failed-prompt timings; decode was not reached.
See [`tag0113 RUN`](benchmarks/qwen35-dataflow-tag0113-20261121/RUN.md).

Tag0114 is the hardware-tested recovery-diagnostic checkpoint. It preserved
the fixed 64-byte mailbox and ioctl request size, and added the subcheck
detail. Its failed card run and timings are in
[`benchmarks/qwen35-dataflow-tag0114-20261121/RUN.md`](benchmarks/qwen35-dataflow-tag0114-20261121/RUN.md).
Tag0115 adds the bounded read-only NoC drain wait for failure7 before
repeating the strict audit. The full variant-60 build, focused recovery
ASan/UBSan test, RV32IMF compile, and `15456/18432` stack bound pass. Its
hardware result identified a persistently busy NoC0 queue0 status; see
[`tag0115 RUN`](benchmarks/qwen35-dataflow-tag0115-20261121/RUN.md). Driver SHA-256 is
`52a4a16ceb170767089edac960925aac0fa549160f47cfd949ba657f25e21885`; client
SHA-256 is `80d28f4d75a48612773b07339010308ced09e8c8debf8e7708c35b85f7908da5`.
The card is currently stable55 generation2 after the guarded post-cycle
restore. See [`benchmarks/qwen35-dataflow-tag0115-20261121/BUILD.md`](benchmarks/qwen35-dataflow-tag0115-20261121/BUILD.md).

Tag0116 records the exact NoC transfer stage, failing index, and raw status
value for an embedding read failure using two existing mailbox reserved words.
It keeps mailbox and ioctl sizes unchanged and retains the strict recovery
audit. The full build, RV32IMF compile, and ASan/UBSan fixtures pass; the
conservative stack bound is `15712/18432`. Driver SHA-256 is
`2acf08ecf6578adcd3529c65259489eb92b4ceca5ffdcb78f6e75f1f0034e710` and
client SHA-256 is `52097a6efb88e89181b96a7476a291fb8afb1d7c828cc9bbf03c1c401c857106`.
It has not been installed on the card. Details are in
[`benchmarks/qwen35-dataflow-tag0116-20261121/BUILD.md`](benchmarks/qwen35-dataflow-tag0116-20261121/BUILD.md).

Tag0117 adds a guarded one-run client option that requests two tokens and
skips the second generation. This lets the rewrite validate a full prompt and
one decode step without running the throughput test. The variant-60 build,
target compile, sanitizer fixtures, and wrapper syntax checks pass. Driver
SHA-256 is `e5387ede717a52354f64fe8cb41ca73bf9a9cc1bc5ab86de0155dca6ec59283c`;
client SHA-256 is `7faaabe33f14399c717b91efc1f690c7defbb7a1d3b74f981d91ced47510276f`.
The one-run card diagnostic failed in the first prompt embedding. It preserved
the expected completion counter (`0x00029067`) beside the observed value
(`0x00029066`) and was recovered to stable55 generation2. See
[`benchmarks/qwen35-dataflow-tag0117-20261121/BUILD.md`](benchmarks/qwen35-dataflow-tag0117-20261121/BUILD.md).

## Progress

- 2026-11-21 (tag0115, bounded read recovery): Added a wait that reads only
  the four resident NoC initiator command registers and 32 queue-status
  registers, and only for worker failure7 (`EmbeddingWeightRead`). A stuck
  status returns a register-index detail without modifying hardware state; a
  quiet status still must pass the existing runtime audit. The fixed mailbox
  and request sizes are unchanged. The recovery ASan/UBSan test, RV32IMF
  compile, complete build, and `15456/18432` conservative stack audit pass.
  Driver SHA-256 is
  `52a4a16ceb170767089edac960925aac0fa549160f47cfd949ba657f25e21885`.
  Hardware trial timed out on NoC0 queue0 and required guarded post-cycle
  restoration to stable55 generation2. Details are in
  `benchmarks/qwen35-dataflow-tag0115-20261121/BUILD.md`.

- 2026-11-21 (tag0115, hardware result): Authentication took `99.897654` s,
  vocabulary/tokenization `13.042067` s, model load `330.400460` s, and the
  failed prompt operation `0.611537` s. Core0 reported embedding weight read
  failure7; the bounded recovery scan timed out on NoC0 queue0 (index4/detail
  `260`), with no output or decode time. The wrapper retained the cold
  candidate, stable backup, and lock. The guarded post-cycle helper restored
  stable55 generation2 and cleared the lock. See
  `benchmarks/qwen35-dataflow-tag0115-20261121/RUN.md`.

- 2026-11-21 (tag0116, failure-detail rewrite): Added exact NoC transfer
  stages and captured register values to the existing failed-embedding receipt.
  The host packs stage/index into its existing recovery detail and reports the
  raw value from the existing request result word. The mailbox remains 64
  bytes, the ioctl request size is unchanged, and the recovery path still only
  waits and audits; it does not clear commands or reset cores. Full build,
  target role compile, and recovery/embedding/owner/service-owner ASan/UBSan
  fixtures pass. Stack use is `15712/18432`. The candidate remains offline;
  Haiku is stable55 generation2 and idle. See
  `benchmarks/qwen35-dataflow-tag0116-20261121/BUILD.md`.

- 2026-11-21 (tag0117, single-run diagnostic): Added a client mode that
  validates one full prompt and a two-token output, skipping the second run.
  It loaded the full model and failed the first embedding NoC read at
  counter0/index36 (`observed=0x00029066`). No token or decode rate was
  produced. Recovery restored stable55 generation2 after the accidental
  power reset; the lock was cleared and both model files remain intact. See
  `benchmarks/qwen35-dataflow-tag0117-20261121/RUN.md`.

- 2026-11-21 (tag0118, expected-counter diagnostic): Captures the expected
  completion counter beside the observed value, carrying it through mailbox
  reserved word3 and a read-only `QUERY_EXPECTED` action. Fixed mailbox and
  ioctl sizes remain unchanged; dataflow ABI is 4. Full build, focused
  sanitizer tests, RV32IMF role compile, and `15712/18432` stack audit pass.
  The guarded diagnostic failed with observed counter0 `0x00029066` versus
  expected `0x00029067`; recovery restored stable55 generation2 and cleared
  the lock. See `benchmarks/qwen35-dataflow-tag0118-20261121/BUILD.md` and
  `benchmarks/qwen35-dataflow-tag0118-20261121/RUN.md`.

- 2026-11-21 (tag0118, hardware result and recovery): Authentication took
  `68.186682` s, prompt tokenization `13.766015` s, full model load
  `334.004328` s, and failed prompt processing `0.615329` s. The first core0
  embedding NoC read timed out with counter0/index36 observed at `0x00029066`
  and expected `0x00029067`; output and decode time are zero. QUERY/RECOVER
  reported audit stage6/core0/detail1508 and CLOSE failed the core0 stop
  audit. The cold-latched candidate was quiesced after a fresh Haiku boot;
  guarded recovery restored stable55 generation2, removed the lock, and
  verified both model files. No valid output exists. Details and raw evidence
  are in `benchmarks/qwen35-dataflow-tag0118-20261121/RUN.md`.

- 2026-11-21 (tag0119, read-only NoC failure receipt): Captures an 8-slot
  completion-counter match mask, four-initiator busy mask, and 32-slot
  outstanding transaction-ID mask in the fixed 64-byte mailbox; a read-only
  `QUERY_NOC` returns it through the existing 36-byte ioctl request. Also corrects receipt cleanup to clear
  only the six reserved words, preserving `logicalCore`. Full variant-60
  driver/service/client build passes at `15712/18432` stack bytes. Service
  owner, owner, embedding-worker ASan/UBSan, and runtime-recovery tests pass.
  The one-run diagnostic is recorded in its run file: the response counter
  timed out and the candidate cold-latched. Guarded recovery remains pending
  because the Pioneer has not answered SSH since the reboot. See
  `benchmarks/qwen35-dataflow-tag0119-20261121/BUILD.md`.

- 2026-11-21 (tag0114, hardware result): Authentication took `99.340053` s,
  vocabulary load and prompt tokenization `13.168584` s, full model load
  `329.758232` s, and the failed prompt operation `0.609034` s. Core0 failed
  embedding weight read (failure7, phase5); no valid output or decode rate was
  produced. QUERY/RECOVER stopped at stage6/detail11 (resident NoC idle), and
  CLOSE failed the core0 stop audit. The accidental power reset left the
  Haiku candidate at generation3 `cold1 active_owner1`. The authorized host
  reboot reached pinned Linux for the SD-free recovery handoff. See
  `benchmarks/qwen35-dataflow-tag0114-20261121/RUN.md`.

- 2026-11-21 (tag0114, recovery subcheck diagnostic): Split the all-core
  runtime-stability predicate into named failure details and returned the
  failing subcheck in unused low bits of the existing owner diagnostic. The
  fixed 64-byte worker mailbox and ioctl request size did not change, and no
  safety predicate was relaxed. Focused ASan/UBSan owner and service-owner
  tests pass, as does the RV32IMF compile. The full variant-60 driver/client/
  service build passes at 15456/18432 conservative stack bytes. Driver
  SHA-256 is `ea4fa51d250c143bd5144156aa6aba71f43e11c9f1c89ece41280ba7a08c3964`;
  client SHA-256 is `80d28f4d75a48612773b07339010308ced09e8c8debf8e7708c35b85f7908da5`.
  It is not installed or hardware-tested yet. Stable55 is idle at generation2
  after the guarded tag0113 post-boot restore; the lock is cleared and the
  stable backup matches. Details are in
  `benchmarks/qwen35-dataflow-tag0114-20261121/BUILD.md`.

- 2026-11-21 (tag0113, first hardware failure): Authentication took
  `98.782252` s, vocabulary and prompt tokenization `14.065504` s, and model
  load `330.909273` s for 2740498432 resident bytes. Prompt embedding failed
  after `0.612365` s on core0 with `EmbeddingWeightRead` failure7, phase5.
  No token or decode time was produced. QUERY/RECOVER failed at runtime audit
  stage6/core0; close failed the core0 stop audit. The wrapper retained tag0113,
  its lock, and the stable55 backup at `cold1 active_owner1`. The recovery
  details and hashes are in
  `benchmarks/qwen35-dataflow-tag0113-20261121/RUN.md`.

- 2026-11-21 (tag0113, failure and recovery diagnostics): Added distinct
  embedding worker failure codes and recovery-audit stage/core receipts while
  preserving the fixed worker mailbox and request sizes. The full variant-60
  driver/client/service build, RV32IMF compile, and focused ASan/UBSan owner,
  service-owner, and embedding-worker tests pass. The conservative stack bound
  is 15456/18432 bytes, with the generation sequence inlined into the measured
  ioctl frame. Driver SHA-256 is
  `e2f7d4bb865cab020c6e52cb411eb99b15ec204b1faa7b1c683496493a0d42cf`; client
  SHA-256 is
  `152c5136d707b899e139ed674d3cfb43c85f74f9058c8773795ed6becbbf94e3`. No
  tag0113 hardware result yet. See
  `benchmarks/qwen35-dataflow-tag0113-20261121/BUILD.md`.

- 2026-11-21 (tag0112, offline stack rewrite): Moved the reusable projection
  descriptor to session-owned heap storage, placed the core schedule scratch
  in the heap-owned projection authority, and made the builder fill the
  supplied descriptor instead of stacking and copying a second 64 KiB object.
  Seven focused ASan/UBSan tests pass, as does the RV32IMF compile without A
  atomics or runtime helpers. The full variant-60 driver/client/service build
  passes; its stack audit reports a conservative 15696-byte chain against the
  18432-byte budget, with no Qwen35 frame above 8192 bytes. Tag0112 driver
  SHA-256 is `b2f3a6be660a07a5c76b363b7ab116ba81b68095126e221c5a741f972a37f664`;
  the client remains `a043d2c1ba96ed28133bbf46ced63e6e1b9e70811d0ce5fedac764c3011419ae`.
  The guarded hardware run then loaded 2740498432 packed bytes in
  `328.264191` s after `98.363993` s authentication and `13.123478` s
  tokenization. The first prompt embedding failed: result31/stage4, worker
  core0 state4/reader1/phase5, partial failed-prompt time `0.612929` s,
  output0, decode time0. QUERY/RECOVER returned pending; close failed the
  core0 stop audit and the wrapper retained its lock and stable backup. The
  authorized cold recovery restored stable55 generation2, cleared the lock,
  and verified the 4B/27B files and 64-CPU/full-RAM profile. No valid output,
  completed prompt time, decode time, or tokens/s exists; the 100 tokens/s
  target remains unmet. See `benchmarks/qwen35-dataflow-tag0112-20261121/RUN.md`.
- 2026-11-21 (tag0111, hardware failure): The candidate installed after a
  clean idle preflight, then Haiku panicked during the generation ioctl. The
  compiler stack report exposed frames of 194800 bytes (FFN) and 75808 bytes
  (projection builder), exceeding Haiku's 20 KiB kernel stack. After reboot
  and the pinned SD-free handoff, recovery restored stable driver55 generation2
  idle and verified the 4B/27B file sizes. Authentication was `99.830468` s,
  tokenization `13.752066` s; an upload-progress sample reached 2657021952 of
  2740498432 payload bytes. Damaged logs prevent a completed model-load time;
  no valid output or decode rate exists. See
  `benchmarks/qwen35-dataflow-tag0111-20261121/RUN.md`.
- 2026-11-21 (tag0111, offline rewrite): Added worker-level embedding failure
  receipts to QUERY/RECOVER, made a pending embedding safely abortable after
  the all-core recovery audit, and moved recovery into the same open service
  ownership window before CLOSE. The complete offline build and focused
  ASan/UBSan tests passed. Driver SHA-256 was
  `34b8680b70b9664176492832be457a26bc82263efaf2be39a233f3e1bad29bd4`; the
  client SHA-256 remains
  `a043d2c1ba96ed28133bbf46ced63e6e1b9e70811d0ce5fedac764c3011419ae`.
- 2026-11-21 (tag0110): The tag0109 `EINVAL` was traced to a missing outer
  ioctl route for operations after `RESIDUAL`. `ProbeControl` now routes the
  contiguous rewrite range through generation. Added compile-time range
  assertions, returned request diagnostics, detailed restored-core idle
  subchecks, held-root restore proof handling, and bounded close retry after a
  resume failure. The complete offline driver/client build passes; owner and
  PCI-memory ASan/UBSan tests and the RV32IMF compile pass. Driver SHA-256 is
  `a9b79420fd351bfb7cbcdda2e70565455973a80a154ca54ba985b1d714f3bd5d`, and
  client SHA-256 is
  `b90f94175aa8a847f360b9ac2a118596d1cd092d37e475ea7043f0281a9d43fe`.
  Tag0110 uploaded the full model in `326.685735` s after `99.320494` s
  authentication and `12.779983` s tokenization. Generation failed on prompt
  position0 during embedding (`stage=4`); `0.609649` s is partial failed
  prompt time. No output, completed prompt timing, decode time, or tokens/s
  exists. Close failed the core0 stop audit. The recovery helper restored
  stable55 generation2 after the authorized power cycle and pinned SD-free
  handoff; postflight confirmed 64 CPUs, full RAM, cleared lock, and both
  model sizes. Continue offline rewrite before another hardware run. See
  `benchmarks/qwen35-dataflow-tag0110-20261121/RUN.md`.
- 2026-11-21 (tag0109): Added structured generation-ioctl rejection details
  and split fleet-resume diagnostics, plus a bounded 10,000-spin settle window
  before restored-core idle checks. The all-core Tensix rewrite still has no
  CPU inference path. The full offline variant-60 driver/client build passes;
  owner and PCI-memory ASan/UBSan fixtures pass; the RV32IMF role compile and
  both guarded-script syntax checks pass. Driver SHA-256 is
  `b87123aab5ff32e7080f14addf7e8e8a945c100e6380032eb52745e6a0387f84`, and
  client SHA-256 is
  `d55bed03a653529c78383c04d19584def654fda83d874257e06ab48e406be4a0`.
  Tag0109 silicon evidence is now recorded. It still has not passed generation
  or clean unload. See
  `benchmarks/qwen35-dataflow-tag0109-20261121/BUILD.md`.
- 2026-11-21: Added the standalone resident-dataflow benchmark client and
  builder. It authenticates the pinned GGUF, uploads the packed model, binds
  its manifest and graph workspace, installs the 120-core resident service,
  requests two repeatable greedy generations, reports model-load, prompt, and
  decode timings separately, checks the known `11751,13` output prefix, and
  explicitly closes the service and seven-bank session. Variant 60 now enables
  the required span uploader under its fixed build tag without enabling the
  legacy Work path. The Haiku RV64 client and variant-60 driver build offline;
  driver SHA-256 is
  `0f9e9b04ebcd66509f05da5053f47bd21c323122fd9e41db9ce5934e532cae7a`, client
  SHA-256 is
  `b2f45646e69e4ae7c83bff3f0572ed60c4ea97d822bebfda158f4456f1a49c08`, and
  service text SHA-256 remains
  `2e0e144040f9c29f9679d5300ff923847c75216b3bf0e577366be4e929e5f8c8`.
  Read-only Haiku preflight found no 27B diagnostic process and reported stable
  driver55 tag `2026102100010012`, generation14, `quiesced0 cold0
  active_owner0 open_count1 guard1`. No model inference has run yet. Build and
  preflight evidence is in
  `benchmarks/qwen35-dataflow-tag0102-20261121/BUILD.md`.
- 2026-11-21: Added the request-level greedy generation ioctl at `0x8847`.
  It accepts prompt token IDs, skips unused vocabulary projection for prompt
  positions before the last, returns generated token IDs, stops at an optional
  EOS token, and enforces the model context bound. The model ABI is now 5. The
  response separates prompt time (through selection of the first output
  token) from decode time (subsequent generated tokens), so decode throughput
  is computed from `output_count - 1` over `decode_time_us`. ASan/UBSan checks
  cover prompt head skipping, bounded positions, EOS, and timing fields; the
  RV32IMF target compile and offline variant-60/default-control build pass at
  tag `2026112100010102`. Driver SHA-256 is
  `fea103ea9e5290da1742c2efe3585564eddf5dd627a95b54f904a7ab53582b87`; service
  text remains 52,900 bytes with SHA-256
  `2e0e144040f9c29f9679d5300ff923847c75216b3bf0e577366be4e929e5f8c8`.
  This code is not yet validated on the Pioneer.
- 2026-11-21: Added device-side greedy token selection after the tied Q6_K
  vocabulary projection. The resident BRISC worker scans the on-card FP32
  logits and writes a checked token/score/sequence record to core 0's mailbox;
  all 120 workers still receive, complete, and acknowledge the command. The
  host owner verifies each invocation readback, the stable result record, all
  worker receipts, runtime state, and drained queues before committing the
  selected token. The decoder ioctl now returns `output_token` in its existing
  40-byte ABI record. ASan/UBSan checks pass for stable greedy ties, NaN
  rejection, all-core owner choreography, vocabulary authorization, and the
  full mixed decoder sequence. The Haiku RV32IMF target compile passes with no
  A-extension instructions or runtime helpers. An offline variant-60 build at
  tag `2026112100010101` passes: driver SHA-256
  `1cacb820faa496a9d72eabad3d551fbd166ef7ecc9aa5a40b92bb3977550bedf`,
  resident service text SHA-256
  `2e0e144040f9c29f9679d5300ff923847c75216b3bf0e577366be4e929e5f8c8`
  (52,900 bytes of the 57,344-byte limit), reload guard SHA-256
  `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229`, and
  default-control SHA-256
  `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d`.
  This was offline evidence only; the subsequent entry adds the request loop.
  Pioneer correctness and decode rate remain unmeasured.
- 2026-11-20: Added `PioneerQwen35DataflowTensixOps.h` as the concrete
  compute-role adapter. It binds one immutable core invocation, selects the
  right 16-value activation panel and scale, issues the Q8_0/Q4_K/Q5_K/Q6_K
  unpack and four Tensix matrix phases, validates bounded integer dot
  readbacks, runs the SFPU affine accumulator, and publishes a 16-row FP32
  page. Q5 affine scale/minimum extraction is isolated in the rewrite's
  metadata helper; packed Q5 operand expansion and all projection dots remain
  on the Tensix path. The MMIO emitter latches config/drain failures and
  returns through the queue failure channel instead of parking. The compute
  contract now requires explicit `Bind`. ASan/UBSan host role checks pass,
  and the Haiku RV32IMF probe instantiates the concrete Q5 and full compute
  paths without A-extension instructions or runtime helpers. This is compile
  and instruction-model evidence only: separate runnable device images,
  resident all-core ownership, and silicon validation are still missing. No
  Pioneer access or benchmark was performed.
- 2026-11-20: Added a rewrite-only Q5_K Tensix unpack path. The new panel
  span selects the original `qs` and `qh` byte-major windows from either
  double-buffered weight page; its SFPU instructions combine each low nibble
  with the selected high bit and write the existing lossless integer overlay
  into Dst for the matrix source path. The offline vector-layout machine
  matches the scalar reference across 2,048 panels and 524,288 operands, and
  the Haiku RV32IMF compile probe instantiates the Q5 path. These checks cover
	operand decoding and compile lowering only at that checkpoint: no native
	image or silicon qualification existed then. No Pioneer access or benchmark
	was performed.
- 2026-11-20 status review: the rewrite is at the pushed NoC-role checkpoint
  `eb6518f8a7`; no Tensix compute backend, runnable role images, or resident
  all-core owner exists yet, so this rewrite has no valid tokens/sec result.
  At the time of that review, the immediate model blocker was Q5_K: the
  rewrite vector unpacker
  accepts Q8_0/Q4_K/Q6_K, while the only Q5_K panel packers are explicitly
  scalar/offline references. Since Qwen3.5-4B Q4_K_M uses Q5_K tensors, those
  helpers cannot be put on the Work path. No Pioneer access, benchmark, or
  assistant integration was performed during that review.
- 2026-11-20: Added version-2 per-core descriptors with separate bank route
  tables for NoC 0 and NoC 1. Each table is checked for valid, unique XY
  routes and copied into the immutable core invocation; the future device
  owner must provide independently audited routes for both fabrics. Added
  `PioneerQwen35DataflowBlackholeIO.h`: its reader operations issue bounded,
  counter-proved NoC 0 reads for the core's activation fragments and assigned
  weight tiles, and perform exact Q8 activation packing; its writer operations
  issue bounded, counter-proved NoC 1 writes for only the core's assigned
  output rows. This follows the Blackhole BRISC/NoC 0 and NCRISC/NoC 1 data
  movement split described in the [TT-Metalium watcher guide](https://github.com/tenstorrent/tt-metal/blob/main/docs/source/tt-metalium/tools/watcher.rst)
  and [Blackhole NIU memory map](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/MemoryMap.md).
  The host role test passes under ASan/UBSan, and the Haiku target compile
  check now instantiates these actual NoC transfer paths. Its matching route
  fixtures are offline geometry only, not measured NoC 1 routes. Reader and
  writer operations are not yet installed as separate device images; the
  Tensix compute backend and resident all-core program owner remain to be
  implemented. No card work or benchmark was performed.
- 2026-11-20: Replaced the queue's compare-exchange first-error word with
  per-role first-error slots. Each Reader, Compute, or Writer image is the
  sole writer to its role slot, and queue polling checks all slots. The L1
  queue-state reservation now uses 32-byte aligned records. RV32 builds use
  explicit `fence` instructions and aligned volatile 32-bit loads/stores, so
  the queue protocol does not require the RISC-V A extension or a libatomic
  helper. Host checks retain compiler atomics for concurrent test coverage.
  The returned queue error uses fixed Reader/Compute/Writer priority when
  multiple roles fail; each role's first error remains preserved in its slot.
  On RV32, the four failure slots are scanned with one acquire fence after the
  reads, keeping the normal polling path from issuing four fences per check.
  The concurrent role exercise and Q5 panel check pass under ASan/UBSan. A
  compile-only adapter now instantiates the three role templates using the
  Haiku cross-toolchain with the Work image's `rv32imf_zicsr` flags; the
  repeatable check is `tests/run_pioneer_qwen35_dataflow_target_compile_test.sh`.
  The ELF32 object has no unresolved helpers or A-extension instructions, and
  its acquire/release fences are present in disassembly. This checks the role
  contracts' target lowering, not a device backend or runnable image.
- 2026-11-20: User confirmed the work order: finish the Tenstorrent-style code
  rewrite first; only then proceed to hardware benchmarking and assistant
  integration. Keep this file updated as the rewrite's checkpoint log. No
  benchmark or assistant-integration work belongs in the current phase.
- 2026-11-20: Moved the immutable per-core invocation from `0xa0000` to
  `0xaa000`. The prior address overlaps the existing Tensix destination-init
  source window (`0xa0000..0xa03ff`); the new 4 KiB invocation region ends at
  `0xab000`, before root scratch at `0xb0000`. The circular-buffer area still
  ends at `0xa0000`.
- 2026-11-20: Changed queue state publication to acquire/release atomics. A
  producer publishes page writes before advancing `produced`; a consumer
  publishes completion before the producer can reuse the page. Failure
  propagation atomically preserves the first role error. The offline test
  checks the first-error rule and verifies the queue refuses further work
  after failure. A separate ASan/UBSan SPSC stress check exchanged 50,000
  ordered pages between concurrent producer and consumer threads without loss
  or reordering.
- 2026-11-20: Added Q5_K (`type=13`) to the offline dataflow-program and
  activation-packing checks. Its projection tiles use the same Q8_K activation
  packing as Q4_K/Q6_K; Q8_0 remains a separate activation format. This closes
  the Q5 descriptor/activation-plan gap, while its Tensix matrix operation is
  still an unimplemented backend gate. Expanded the descriptor check across
  all eight Qwen3.5-4B quantization/width combinations and rebound every shape
  to all 120 core invocations.
- 2026-11-20: Added an offline-only Q5_K panel-layout reference that preserves
  exact integer weights and FP32 coefficient/minimum metadata in the Tensix Q8
  panel-zero layout. A compile guard keeps this scalar reference out of Work
  code; the eventual backend must produce the layout with device Tensix
  operations. The offline test decodes the synthetic tile through the Tensix
  panel decoder and checks every operand plus metadata.
- 2026-11-20: Reworked the per-core L1 map after finding that two adjacent
  maximum Q6_K weight pages would overlap the existing affine metadata at
  `0x91200..0x91280`. Weight pages now ping-pong between `0x90000` and
  `0x98000` with a checked `0x8000` stride, matching the Qwen35 tile-prefetch
  plan; activation cache and queue state occupy `0x80000..0x89030`, and
  input/output queues start at `0x92000` and `0x92800`. Both the full program
  and copied core invocation reject any page
  layout that overlaps affine metadata, queue state, activation cache, or
  another buffer. Added a rewrite-only vector-unpack entry that accepts the
  current page address and validates both ping-pong slots; its offline check
  verifies every Q8/Q4/Q6 panel source address. Reserved `0x90fc0..0x91280`
  for the Q5 repacker's Q8 panel-zero tile plus affine metadata; compile-time
  offset checks bind its coefficient/minimum fields to `0x91200`/`0x91240`,
  and only Q8 panel zero may unpack from that staging address. The compute Ops
  adapter still does not call these helpers.
  The dataflow and Q5 panel tests both pass under AddressSanitizer and
  UndefinedBehaviorSanitizer after the L1 layout change. A negative compile
  check also confirms the scalar Q5 reference is rejected unless its
  test-only macro is set.
- 2026-11-20: Added `PioneerQwen35DataflowKernels.h` with separate reader,
  Tensix-compute, and writer role contracts over two-page single-producer,
  single-consumer local queues. The compute template requires an explicit
  Tensix-matrix-engine capability, and each role validates its core's shard
  descriptor before touching device data. Role setup errors publish a queue
  failure so a neighboring role can leave its bounded wait.
- 2026-11-20: Added a host-only concurrent reader/compute/writer exercise over
  a complete Q5_K shard using the real queue and role loops. It checks packed
  activation chunks, weight reads, Tensix-call counts, output-page counts, and
  queue drain. This exposed a reader address bug: groups of 16 rows must
  advance by `16 * rowBytes`; the reader now bounds each group and block with
  that stride. The mock Ops validate choreography only and do not perform
  inference. The exercise passes with AddressSanitizer and
  UndefinedBehaviorSanitizer.
- The reader packs each 256-value activation chunk once in local memory and
  reuses that packed chunk across its assigned output groups. Q8_0 weight
  blocks map eight-at-a-time to the same 256-value activation chunk.
- Added `DataflowCoreInvocation`: the full 120-core schedule remains host-side,
  while each core gets only its own immutable shard assignment in the bounded
  4 KiB local region at `0xaa000`. The per-core object is size-checked and
  revalidates its local buffers, queue layout, and bank ranges.
- Added a device-side local-L1 binder for the invocation, queue controls,
  circular buffers, and activation cache at their fixed addresses. It is
  compile-checked only; no Blackhole role image consumes it yet.
- Added reader, compute, and writer entry wrappers that bind that local
  context and publish a role failure to shared queue state if binding fails.
- Expanded the offline program test to compile all three role interfaces and
  check 9,216-row output shards and a 9,216-value Q4_K input across all 120
  per-core materializations. It also checks Q6_K and Q8_0 plans, shard
  coverage, queue address binding, queue publish/consume, ticket wraparound,
  in-place activation packing, Q8 block mapping, and malformed-descriptor
  rejection. The host test passes with AddressSanitizer and
  UndefinedBehaviorSanitizer.
- The existing Work dispatcher and ROOT gather path are retained as the
  checkpoint; no changes to that path are part of this rewrite phase.
- Added `PioneerQwen35DataflowProgram.h`: a 120-core program descriptor and
  builder that maps each projection shard to its weight bank, sharded
  activation input/output ranges, and double-buffered reader/compute/writer
  circular-buffer layout.
- The lower-level concurrent reader/compute/writer loops remain offline
  contracts. The current device implementation is the resident BRISC service
  image and per-core projection worker described below; it overlaps the next
  NoC 0 weight prefetch with Tensix work. The host PCI image loader and physical
  mailbox/recovery adapter are not implemented.

## Rewrite gates

1. Complete offline descriptor and per-core worker checks.
2. Build the concrete resident worker image with its NoC and Tensix operations
   for the Haiku RV32 image, with no CPU inference or fallback.
3. Wire the new host owner to the physical PCI transport so it loads the image
   across all 120 cores, materializes per-core descriptors, drives the mailbox,
   and preserves sharded activations across dependent operations.
4. Complete offline target and full-graph correctness gates. Only then begin
   the hardware benchmark and assistant integration phases.

## Latest rewrite work

- 2026-11-20: Added `PioneerQwen35DataflowBlackholeWorker.h`, a concrete
  one-BRISC-worker-per-core projection path. Each core reads its input and
  weight shard over NoC 0, packs Q8 activation chunks locally, alternates its
  two L1 weight/input pages, issues each output group through the Tensix
  matrix engine, and writes the sharded result over NoC 1. It validates the
  fixed per-core queue state before use and publishes bounded Reader, Compute,
  or Writer failure slots before returning. No host dot-product or inference
  fallback exists. The target compile probe now instantiates this worker, and
  the Haiku RV32IMF cross-compile passes without A-extension instructions or
  runtime helpers. At this initial worker checkpoint, the service loop and
  standalone image builder had not yet been added. No Pioneer access,
  benchmark, or assistant integration was performed.
- 2026-11-20: Added a 64-byte per-core mailbox ABI and a resident BRISC service
  loop. The service verifies its local XY route against the immutable core
  assignment, executes one projection invocation, publishes completion or a
  role failure, then accepts another only after host acknowledgement. On that
  acknowledgement it verifies the local queues are drained and clears their
  error slots. Two weight pages now overlap a NoC 0 prefetch with Tensix work.
  The standalone image builder passes and emits a 25,024-byte RV32 image with no
  unresolved imports; the target compile probe still passes without A-extension
  instructions or runtime helpers. The offline mailbox/route check also passes
  under AddressSanitizer and UndefinedBehaviorSanitizer. This is offline image
  evidence only: the physical host launch/acknowledgement owner is missing,
  and the image has not been loaded or run on the card. No benchmark or
  assistant integration was performed.
- 2026-11-20: Added `PioneerQwen35DataflowOwner.h`, a host-side projection
  coordinator contract. It checks all 120 workers, materializes and reads back
  every immutable invocation before publishing any command, waits for terminal
  receipts from the whole shard set, and only acknowledges after the backend
  audit succeeds. Ambiguous partial publication moves the owner into a faulted
  state; `Recover()` requires an all-core backend recovery audit before it
  acknowledges completed workers and resumes with a higher mailbox sequence.
  ASan/UBSan mock coverage passes normal two-projection execution and partial
  launch recovery, including workers resynchronizing across skipped sequence
  values. This is the orchestration contract and mock only: its physical PCI
  transport, image load, recovery proof, and all-core graph are still missing.
  No Pioneer access, benchmark, or assistant integration was performed.
- 2026-11-20: Added `PioneerQwen35DataflowServiceOwner.h` to own the resident
  image lifecycle around that projection coordinator. It requires an idle
  session and a checked 120-route map, captures all core state before writes,
  holds every core, stages the worker image in bounded 256-byte chunks, and
  reads back code, queue controls, mailbox, and boot word before starting any
  worker. It waits for every worker's ready receipt, then delegates projection
  commands. Close stops and audits the fleet, restores and verifies all
  captured L1 and boot state while cores remain held, then resumes each core's
  captured reset state only after the whole fleet matches. The ASan/UBSan mock
  passes full-fleet startup, preflight refusal, partial-start close, stop-audit
  retry, projection, restoration, and original running/held-state recovery. A
  Haiku RV32IMF compile of the service owner also passes. This is still an IO
  contract and mock: the Haiku PCI/L1/reset
  adapter, generated-image packaging, full model graph, and physical recovery
  proof remain unimplemented. No Pioneer access, benchmark, or assistant
  integration was performed.

The pre-rewrite Work baseline is `1b2b57c561`; the branch tip immediately
before adding the BRISC worker was `830ec1011e`. Rewrite checkpoints are pushed
incrementally to `codex/qwen35-4b-p100a`; use that branch's current tip for the
latest checkpoint.

The Haiku RV32 cross-compiler is available at
`/Volumes/HaikuBuildLocal/generated.riscv64`; use
`tests/run_pioneer_qwen35_dataflow_target_compile_test.sh` for the checked
worker compile. The local Apple Clang 17 build has no RISC-V target.

The lower-level queue role templates remain offline contracts. The concrete
per-core path is `PioneerQwen35DataflowBlackholeWorkerEntry`, which binds
BRISC-side NoC operations and Tensix matrix operations into one bounded worker.
It prefetches the next weight tile over NoC 0 while the matrix engine processes
the current tile. Tenstorrent's open `tt-lab` worker pattern is the closest fit
for this freestanding Haiku image ([project](https://github.com/tenstorrent/tt-lab),
[BRISC firmware](https://github.com/tenstorrent/tt-lab/blob/main/src/brisc.cpp)).
The resident all-120-worker owner, driver command path, offline generated-image
validation, full decoder sequence, and request-level greedy generation are now
implemented. Silicon correctness and performance evidence remain open.

## Current checkpoint: 2026-11-21

Checkpoint `55df940409` is pushed with device-side greedy token selection and
the full decoder sequence. The current local rewrite adds the bounded
request-level generation ioctl: token IDs go in, generated token IDs come out,
and prompt and decode time are returned separately. Prompt timing includes the
first output token; decode timing covers later output tokens, with throughput
reported as `(output_count - 1) / decode_time`. Tied projections use all 120
Tensix matrix engines. The BRISC argmax worker scans logits; all 120 resident
workers take part in each command barrier and receipt protocol. The driver
uses build tag `2026112100010102`.

Offline ASan/UBSan tests cover deterministic tie selection, non-finite-logit
rejection, argmax authorization and owner barriers, full decoder sequencing,
prompt generation, EOS, context bounds, and phase timing. The RV32IMF target
compile and variant-60 driver, resident service image, reload-guard, and
default-control builds pass. The resident service text is 52,900 bytes and
fits the 57,344-byte region. Tag0105 was the first hardware install attempt for
this request-level client. It uploaded and bound the model but failed before
worker readiness, so no silicon output, prompt time, or decode throughput was
produced. The verified failure and post-cycle recovery state are recorded in
`benchmarks/qwen35-dataflow-tag0105-20261121/`.

The offline request/generation loop and standalone benchmark client are
implemented. The initial preflight verified that the prior 27B diagnostic
had exited and stable driver55 was idle. The first tag0105 hardware trial
reached service startup and reported worker-readiness failure. Stable driver55
has been restored at generation2 with the recovery lock removed. Next, add
per-worker readiness diagnostics and bounded startup rollback before another
guarded hardware trial.

The dense-attention baseline `e533c033ef` includes
Tensix matrix projections, device-side Q6_K token embedding and hidden RMS
normalization, manifest-bound graph projections, SiLU×Up, residual save/add,
feed-forward sequencing, recurrent convolution, Alpha/Beta projections,
delta-state update, fused per-head RMS normalization plus SiLU gating, and
dense attention/KV. RecurrentOut uses the Tensix projection path with an
explicit input offset into the retained QKV V channels. Recurrent and dense
layer sequences are implemented. Mixed dense/recurrent full-model
orchestration, vocabulary readout, and token selection remain incomplete. No
hardware test or decode rate is available.

Checkpoint `6e0f12b4ff` adds exact authorization and readiness
tracking for the dense QueryAndGate, Key, and Value Tensix projections. The
projection test now checks the required QG -> K -> V order, workspace role and
layer ownership, and all 120 active Tensix assignments for QueryAndGate. The
focused host test passes with ASan/UBSan, and the Haiku RV32IMF compile check
passes without A atomics or runtime helpers. Dense RoPE, K/V cache packing and
writes, attention, AttentionOut, mixed-layer sequencing, vocabulary readout,
and token generation remain open; there is still no model output or decode-rate
measurement.

Checkpoint `e533c033ef` adds the dense-attention path. A manifest-bound
descriptor reserves the model's rotary table, per-layer KV cache, Q/K norms,
QKV output, gate input, and attention-output scratch. BRISC workers perform
Q/K RMS normalization, RoPE, half-precision K/V cache packing, and grouped
query attention with tiled NoC reads and online softmax; the AttentionOut
matrix projection remains on the Tensix path. Every command materializes and
reads back all 120 worker invocations and audits receipts before committing
authority state. Four workers own the K/V heads and sixteen own query heads;
the all-core barrier keeps the resident service synchronized.

`DataflowDenseLayerSequence` now composes residual save, attention norm,
QueryAndGate/Key/Value projections, K/V cache writes, attention, AttentionOut,
residual add, and the existing FFN sequence. Driver commands `0x8844` and
`0x8845` expose the dense-attention primitives and full dense-layer sequence.
ASan/UBSan checks pass for the dense math, worker, program/authority/owner, and
sequence fixtures. The math fixture matches the pinned operators exactly for
RMS, RoPE, F16 conversion, and online softmax. The RV32IMF target compile
passes without A atomics or runtime helpers. Offline resident-service-image,
reload-guard, and default-control builds pass; service text SHA-256 is
`1166102743dc8017be249c1d6a83a7a55642bc425ebbfa3b8eda74e4dbc98a59`,
reload guard SHA-256 is
`d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229`, and
default-control SHA-256 is
`5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d`.
These are offline checks only. Mixed recurrent/dense model orchestration,
vocabulary readout and token selection remain before full-model output.
No Pioneer run or benchmark has been performed.

The graph workspace participates in the projection path. The request can
select the Up, recurrent QKV, or recurrent gate sidecar; the authority maps
each selector to its exact model role and rejects recurrent targets on dense
layers. It tracks readiness per scratch tensor, independently from activation
buffers and from other scratch tensors. ASan/UBSan projection and authority
checks, the Haiku RV32IMF role compile, and driver60, reload-guard, and
default-control builds pass. The driver60 SHA-256 is
`35c2c1e0f5ae691f5943ec56aa18a21a90e3a5bb69c3c691cb919696bf08b90a`.
These selectors make QKV and recurrent-gate projections available through
the existing 120-core Tensix path. No legacy Work changes are part of this
step.

Checkpoint `00b34e48eb` adds the recurrent output projection. Its descriptor
keeps the whole manifest-bound QKV RowSpace and selects the 4096-row V slice
through `inputBase`; the offset is copied to every core invocation and the
reader maps it across source fragments. Only the exact same-layer RecurrentOut
tensor and a successfully completed delta step are accepted. The authority
invalidates the destination before work and consumes QKV after a successful
commit or audited abort. ASan/UBSan delta-authority tests cover the exact V
offset, rejection of a shifted offset, all 120 matrix-worker assignments, and
one-time completion. Existing projection/authority checks pass. The RV32IMF
role compile passes without A atomics or runtime helpers. The resident service
image is 44,224 bytes with no unresolved imports and fits the fixed 57,344-byte
region. The offline driver60 build at tag `2026112000010099` passes; its SHA-256
is `e39be4fb33c5d614cc8959d7dd8a5c60510fe33c4ce6960d73c759c1c0af378d`.
Service text SHA-256 is
`7eea8370b070336d98ffd28897e2ce44fec519249f533b1e83506f80da58f950`; reload
guard and default-control builds pass. These remain offline checks; there is no
Pioneer output or performance measurement.

Checkpoint `cd6c25e985` adds a rewrite-only recurrent layer sequence and ioctl
at `0x8843`, using new driver build tag `2026112100010100`. It composes the
attention norm, four exact graph projections, recurrent convolution and delta
step, V-slice RecurrentOut, attention residual add, and the existing FFN
sequence. The sequence rejects dense layers and invalid positions, preserves
the first failing stage, and performs no tensor math itself. ASan/UBSan tests
cover operation ordering, invalid dense-layer requests, and stopping after a
delta failure. The Haiku RV32IMF role compile passes without A atomics or
runtime helpers. The offline driver60 build passes with SHA-256
`057b487b6543f4103934e8c24bc4f05a0af939b35d2013f4fe74b4e53c0772bc`; reload
guard and default-control builds pass. This is still offline-only: dense
attention/KV and final token generation remain before end-to-end model runs.

The device-side SiLU×Up step is implemented and pushed in `8ed176883c`. Its
manifest-bound schedule assigns all 576 sixteen-row panels over the 120
resident workers (96 workers handle five panels and 24 handle four). Each
worker reads its gate and graph Up panels over NoC, computes SiLU(gate)×Up on
the device, and writes the gate activation in place. The authority requires a
completed exact Up projection and initialized gate activation, invalidates
both inputs before execution, and commits the gate only after the owner audits
all worker receipts, invocation readbacks, and queues. A root-only `SILU`
control opcode exposes this primitive; it does not add generation or a CPU
fallback.

ASan/UBSan checks pass for the SiLU approximation, 120-core descriptor
coverage, exactly-once worker simulation, authority state transitions, and
owner orchestration. The Haiku RV32IMF compile passes with no A atomics or
runtime helpers. The offline driver60, reload-guard, and default-control
builds pass; driver60 SHA-256 is
`d8435142c513f9b94290a706a17d440449fea8b8dd957c27ece2bf128a9873f2`, reload
guard is `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229`,
and default-control is
`5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d`.
These are offline checks only.

The residual save/add operation is implemented and pushed in `019b7fb29e`. The
manifest-bound schedule assigns all 160 hidden-width panels across the 120
workers (40 workers handle two panels and 80 handle one). `SAVE` copies an
initialized Hidden-row activation into residual scratch; `ADD` adds that saved
residual into the activation in place. The authority invalidates residual
readiness before save and consumes it before add, then commits only after
all-core invocation readback, terminal receipts, runtime/queue audit, and
acknowledgement. The root-only `RESIDUAL` opcode exposes the two explicit
modes without host math or fallback.

ASan/UBSan checks pass for exactly-once coverage and save/add arithmetic on
all 160 panels, authority save/add transitions, and owner orchestration across
both modes. The RV32IMF compile passes without A atomics or runtime helpers;
the offline driver60, reload-guard, and default-control builds pass. The new
driver60 SHA-256 is
`5b1aff677a77cc86b75e725e0bd16c2e2a2c34aeb3b277cabbe5164be1c0961a`; the
service text SHA-256 is
`279cac1dddbc27f0c6621fec07b248d267714073355fb0cf83b812d6cc3cf9fe`.

The rewrite step in `2a2405752a` adds `DataflowFfnSequence`, which composes
those device operations into a complete feed-forward sublayer: residual save,
post-attention RMS, Gate and Up projections, SiLU×Up, Down projection, and
residual add. A rewrite-only root opcode exposes this sequence and reports the
failing stage. The coordinator only builds authorized programs and submits
resident worker commands; it performs no tensor arithmetic. Host ASan/UBSan
checks cover complete ordering, invalid requests, and stop-on-first-failure.
The RV32IMF sequence compile check passes without A atomics or runtime helpers.
The offline driver60, reload-guard, and default-control builds pass; the
driver60 SHA-256 is
`5b59a99ea3346af23399ec273bd810c3f9730bb4f5aa8c8cce0c131264045ed9`.
This is an offline-checked, pushed rewrite checkpoint; recurrent/dense
attention, state updates, final vocabulary readout, and token generation
remain unimplemented. No Pioneer access or benchmark was performed.

Checkpoint `ef1279b585` extends the validated graph-workspace sidecar to six
FP32 tensors: Hidden residual, FFN Up, recurrent QKV (`ConvChannels`),
recurrent gate (`DeltaInner`), and 32-row Alpha and Beta vectors. Manifest
validation requires all six regions in allocator order; the model-bind ABI is
now version 3. This gives the recurrent projection and state operators
reserved, bounded output storage. Checkpoint `cd35b0db45` adds the first
recurrent projection targets (QKV and gate); the projection state machine now
commits or invalidates each target independently. ASan/UBSan projection and
authority checks pass, as does the RV32IMF role compile. The offline driver60,
reload-guard, and default-control builds pass. The updated driver60 SHA-256 is
`35c2c1e0f5ae691f5943ec56aa18a21a90e3a5bb69c3c691cb919696bf08b90a`.
Recurrent convolution, delta-state math, dense attention, KV/state IO, final
vocabulary readout, and token generation remain unimplemented. The GGUF
fixture test was not run because no local pinned-model path is available. No
Pioneer access or benchmark was performed.

2026-11-20 rewrite progress: added the manifest-bound recurrent convolution
operator and its resident worker command in checkpoint `a769eea54c`. It reads
the completed recurrent QKV sidecar, the model's four-tap convolution weights,
and the selected
ping-pong history over NoC 0; each of the 120 cores owns four or five of the
512 channel panels and writes updated history plus the SiLU result over NoC 1.
The projection feeding this operation remains the rewrite's Tensix matrix
path; this convolution step is channelwise firmware math, not a matrix
projection. The new authority transition invalidates QKV while state is
updated and commits it only after all-core completion, invocation readback,
queue/runtime audit, acknowledgement, and ready-state checks. A rewrite-only
driver ioctl exposes the command. The offline fixture checks exact position-0
and warm-position arithmetic, ping-pong history, complete 120-core coverage,
authority transitions, and owner sequencing. ASan/UBSan worker/owner and
existing dataflow tests pass, the Haiku RV32IMF compile passes without A
atomics/runtime helpers, and the offline driver60, reload-guard, and
default-control builds pass. Driver60 SHA-256:
`15e433e1b5c15fa1c2bba19255d9268c8b0e2f91e1e6b4826f518ed5383b18c2`.
This is offline-only evidence. No Pioneer access, silicon validation,
generated output, benchmark, or assistant integration was done. Delta-state
math and Alpha/Beta projections, dense attention, state/KV IO, vocabulary
readout, and token generation remain to be implemented before benchmarking.

Checkpoint `281ca54aab` connects the reserved recurrent Alpha and Beta
workspaces to their exact model roles in the existing Tensix projection
builder. The scheduler now permits fewer than 120 output panels: all 120
resident workers still receive, validate, and acknowledge the command, while
the two workers owning a 32-row gate projection issue matrix work and the
remaining 118 complete as bounded no-op workers. The graph authority tracks
each sidecar independently. Offline tests verify Alpha/Beta role and
workspace binding, active/inactive invocation coverage, and the all-core
owner barrier. ASan/UBSan projection, owner, and recurrent-convolution tests
pass; the RV32IMF role compile passes without A atomics or runtime helpers.
Offline driver60 SHA-256:
`a001c424f14e1c09dc8c67dba326732bcc7dc33b475941b1477b8cc971e2e59c`;
service-image text SHA-256:
`507aa854644964ec091fb7a19271d87edcad0dbf794481125ff0eda593db76b2`.
The driver, reload-guard, and default-control package builds pass. These
remain offline checks; no hardware work or performance result exists.

Checkpoint `307c9d134b` adds per-layer recurrent position tracking to the
authority. A convolution can run only at that layer's next committed position;
replay, skipping, and out-of-order requests are rejected. The new
`ResetRecurrentState()` request boundary resets position tracking and
invalidates recurrent scratch without writing the large resident state
regions. This is safe because position-zero convolution substitutes zero
history instead of reading the old ping-pong slot; the delta-state worker
likewise substitutes zero state at position zero. ASan/UBSan tests cover successful advance,
replay rejection, reset, and reprojection before a new position zero. The
RV32IMF role compile passes, and offline driver60, reload-guard, and
default-control builds pass. Driver60 SHA-256:
`a7cd35960b48c7844910e8f2f0df72b661ce19dd014d8b5ca4c1f96d4a715d15`.
No hardware validation or throughput evidence was produced.

## Delta-state worker `221022cc34` (2026-11-21)

At checkpoint `221022cc34`, the manifest-bound delta program validated QKV,
gate, Alpha/Beta, learned decay/bias vectors, and state ping-pong ranges. It
partitioned 256 sixteen-value panels across all 120 workers. The BRISC worker
performed Q/K normalization, decay and beta gates, state update, and readout;
projections remained on the Tensix matrix path. The following checkpoint
extends this step with per-head RMS normalization and SiLU gating.

The projection authority permits a delta command only after matching QKV,
gate, Alpha, and Beta projections plus the same-position convolution have
committed. It invalidates QKV and consumes Alpha/Beta while the command is in
flight, then advances the delta position only after all 120 invocations,
worker receipts, runtime/queue audits, and acknowledgements pass. Position
zero ignores stale state and writes the alternate slot. Driver opcode
`0x8842` exposes this operation through the resident service.

The `061aa5c8ef` checkpoint assigns each of 32 recurrent heads to one BRISC
worker. Each owns all eight state panels plus the complete 128-value RMS
normalization group; gated values are stored back after all source panels for
that head are read. The 120-worker command/receipt/acknowledgement barrier is
unchanged, and projections still use all 120 Tensix matrix engines.

Offline ASan/UBSan fixtures pass for zero-state and warm-state recurrence,
head gates, bit-for-bit RMS-plus-SiLU comparison with the root operator,
descriptor coverage, authority transitions, and the all-core owner barrier.
The Haiku RV32IMF worker compile passes without A-extension instructions. The
size-optimized service image is 43,964 bytes, has no unresolved imports, and
fits the fixed 57,344-byte region. Offline driver60 SHA-256:
`d523169a4c46c51406a7d2c5b63847383095648e23681866b81b230ff4596578`;
service text SHA-256:
`8363fdcc559620eee9553a37c8061ab8ce9e7166595e90120dec1d277eca3d0f`;
reload guard SHA-256:
`d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229`;
default-control SHA-256:
`5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d`.
These are offline checks only. The operation has not run on the Pioneer and
has no silicon correctness or performance evidence.

The earlier checkpoints below describe implementation history. Hardware
correctness, warm-run output, measured throughput, and assistant integration
remain separate milestones. Do not report legacy Work measurements as rewrite
performance.

## Current rewrite progress (2026-11-21)

The tag0106 silicon trial authenticated, tokenized, uploaded the full 4B model,
and bound its manifest, then stopped at worker 0 startup with the local-core
route check failing. It produced no inference output or model-load, prompt, or
decode timing. The post-cycle recovery helper restored stable driver55 at
generation2 and removed the recovery lock. A read-only check confirmed the
Pioneer is idle, with both the 4B and 27B model files intact.

The next rewrite change drains startup results from all 120 workers after the
first readiness failure, retaining the earliest worker diagnostic. This gives
later workers time to reach a stable ready or startup-fatal state before the
existing all-core stop audit runs. The focused owner and PCI-memory tests pass
with AddressSanitizer and UndefinedBehaviorSanitizer, the RV32IMF role compile
passes without A atomics or runtime helpers, and the offline driver/package
build passes. The rebuilt driver SHA-256 is
`531f419a9c6d213cdf75da0afd0e27e327c6750086cda0375d59e3684d8fd420`; the
service text is unchanged. These checks do not establish silicon recovery.
At that checkpoint the route mismatch and exact cleanup failure still needed
hardware evidence; tag0107 later captured raw route `0x312` and a resume
failure, as recorded below. No warm output or decode rate exists; the 100
tokens/s goal remains unmet. Assistant integration remains deferred until the
rewrite and benchmark are complete.

### Tag0107 recovery and route diagnostics

Tag0107 now records the phase-2 worker's raw local route, waits for the full
120-core startup scan, and reports the stop-audit stage/core. Its special
recovery proof admits only that exact route-fatal state when the selected
physical core, service image, captured NoC sources, raw route, queues, and
idle NoC are stable; it does not admit the route for inference. The protocol
is ABI 2. Owner and PCI-memory ASan/UBSan tests pass, the RV32IMF service
compile passes without A atomics/runtime helpers, and the offline package
build passes. Driver SHA-256:
`176aacf58aeb972aabbe05d5c3ca6f89f5af1a1f3648ba96d7c932abea085668`; client
SHA-256: `232a98fd41c77f3e559e5de983a156febb65ff483c16da5181dcc28f268d72cd`.
The guarded tag0107 trial and post-cycle recovery helper were run on the card.
The trial authenticated, tokenized the prompt, uploaded
the complete 4B model, and bound its manifest. Startup again stopped at worker
0: phase 2 rejected raw local route `0x312` (786). No prompt execution or model
load/prompt/decode timing was produced. Cleanup stopped after the client
reported `close_failed result=18` (resident-service resume failure); the
client descriptor closed, but the guarded wrapper correctly refused to swap
drivers while the candidate reported `cold1 active_owner1`. The current card
state is tag0107 driver60, generation3, `cold1 active_owner1 open_count1`, with
the recovery lock retained. The stable driver has not yet been restored.

This run confirms the route-fatal branch is reached and the driver does not
silently fall back to CPU inference, but it does not verify the attempted
no-reboot cleanup path. The next rewrite checkpoint needs a precise
restore/resume failure stage and core, then a corrected worker identity/route
contract before another model run. No valid output, warm run, or decode rate
exists; the 100 tokens/s goal remains unmet. The 27B setup was verified intact
at preflight and was not accessed by this 4B run. Assistant integration remains
deferred until the rewrite and benchmark are complete.

### Tag0108 route and cleanup rewrite (2026-11-21)

Tag0108 no longer derives a logical worker number from the firmware's raw
local NoC register. The kernel writes the selected core ID into the startup
mailbox; each worker validates that assignment and checks invocation ownership
against it. During the all-core preflight the host now captures a stable,
unique local route for each selected core and requires that route to remain
unchanged during inference. Projections still have no CPU fallback and use
the Tensix matrix engine across the 120-core service fleet. Close now reports
the exact core and substage when restoring or resuming the fleet fails.

The new mailbox/client protocol is ABI 3. Five focused sanitizer tests pass,
the RV32IMF compile passes without A atomics or runtime helpers, and the full
offline package build passes. Tag0108 driver SHA-256 is
`62d64043cf5f3e2110b96446839efdfe2c61a945c8d87cd21d9d9a52072de1dc`; client
SHA-256 is `cbee1483b73fa8e08754fbd04d1f78376e725ae7dc6e078ad271fdc2a58a256a`.
The guarded two-warm and post-cycle recovery scripts are prepared.

The guarded tag0108 trial passed preflight from stable driver55 generation2
idle, installed candidate generation3, and uploaded all 2,740,498,432 model
bytes. Authentication took 100.027643 s, vocabulary/prompt tokenization
13.277220 s, and model load/upload 334.815789 s. The first generation ioctl
returned `EINVAL`; no generation completed, so there is no valid output,
prompt-processing timing, decode timing, warm-run rate, or tokens/s. The
client's prefix/repeatability messages are failure checks after the rejected
request, not generated text.

Cleanup returned `DataflowServiceResumeFailed` (result18), diagnostic
stage9/core0, resume phase3, resume detail2 (the final idle-core proof failed);
unload was not clean, and the wrapper correctly kept
the lock and stable backup while tag0108 reported `cold1 active_owner1`.
After verifying the client had exited, one authorized power cycle and the
pinned SD-free Linux-to-Haiku handoff allowed the tag0108 recovery helper to
quiesce the fresh generation1 candidate and restore stable driver55 at
generation2. A read-only postflight confirmed stable55 idle, the lock absent,
both model files intact, and 64 CPUs/full RAM. Raw logs and exact post-recovery
hashes are in
[`benchmarks/qwen35-dataflow-tag0108-20261121/RUN.md`](benchmarks/qwen35-dataflow-tag0108-20261121/RUN.md).

The next code-only rewrite step is to add a precise receipt for the rejected
generation request and resolve the core0 resume phase3 failure before another
hardware trial. The 100 decode tokens/s target remains unmet. Assistant
integration remains deferred until the rewrite and benchmark are complete.

### Tag0119 NoC response-counter diagnostics (2026-11-21)

Tag0119 added a read-only NoC snapshot to the existing 64-byte worker mailbox:
an eight-slot completion-counter match mask, four-initiator busy mask, and
32-slot outstanding transaction-ID mask. It also fixes worker receipt cleanup
to clear exactly the six declared reserved words, preserving `logicalCore`.
The full variant-60 driver/service/client build and focused owner,
embedding-worker, and runtime-recovery sanitizer fixtures pass. The compiled
service uses no A-extension atomics and fits its fixed region.

The one-run hardware diagnostic authenticated, tokenized, and loaded the full
4B model, then failed on prompt embedding read stage 7/index 36. The response
counter observed `0x00029066` where `0x00029067` was expected; the match mask
was `0xfe`, initiator-busy mask `0x0`, and outstanding-ID mask `0x00000001`.
The raw tag0119 client called that last field `failed_read_queue_busy_mask`;
that label was incorrect. Per the Blackhole counter map, bit 0 is outstanding
transaction ID 0. Together with the missing read-response counter and the
advanced command-accepted/request-sent counters, this shows the read request
was sent but its response was not received by the initiating NIU; it does not
yet distinguish a target-address/route issue from a missing return response.
This is a read-only observation; no counter reset, request retry, or Tensix
reset was issued. Authentication took 100.664302 s, tokenization 13.757776 s, load
333.142334 s, and the failed prompt 0.615593 s. No output token was produced,
so decode throughput is unavailable and 100 tokens/s remains unmet.

The tag0119 candidate cold-latched and clean unload failed; the trial preserved
the stable-driver backup and recovery lock. The 4B and preserved 27B model
files were verified intact, and the client process had exited. Haiku accepted
a reboot request, but both pinned SSH profiles later timed out; the existing
serial console showed shutdown-device teardown without a new boot banner.
The user's accidental power reset was reported during this recovery window.
No relay pulse was sent while the current board power state remained unclear.
Stable driver55 generation2 must be verified restored before any next
hardware trial. The two-warm throughput benchmark and assistant integration
remain deferred. Full evidence is in
[`benchmarks/qwen35-dataflow-tag0119-20261121/RUN.md`](benchmarks/qwen35-dataflow-tag0119-20261121/RUN.md).

### Tag0120 request-identity receipt (2026-11-21)

Tag0120 clarifies the tag0119 counters and extends the failed-read receipt.
The fixed worker mailbox remains 64 bytes and the control request remains 36
bytes. The six existing reserved words now preserve the failed read's remote
address, target route, byte count, stage/index, observed and expected
completion counter, counter/initiator masks, and 32 outstanding transaction-ID
slots. A new read-only `QUERY_READ_REQUEST` action returns the read address,
route, and size; it performs no NoC transaction, reset, retry, or counter
clear. The normal owner and 120-worker command barriers are unchanged.

The correctly tagged variant-60 driver/service/client build passes with
runtime tag `2026112100010120` and a `15712/18432`-byte conservative kernel
stack. The RV32IMF role compile has no A-extension atomics or unresolved
helpers. The embedding-worker and service-owner ASan/UBSan fixtures verify
request identity and receipt propagation; the runtime-recovery fixture passes.
Driver SHA-256 is
`153748972239101302603e71f4ba3a3cd665fb6e63d51a7cb4094df91c538d7c`, client
SHA-256 is `73cd7e9b47711866d67bd50eb8cb0ad358138760d2bb3d5cda54cfdf8802a60b`,
and service text SHA-256 is
`2025572629309596d727bbc3aec55b458f643930fbe0f1766633e168c20c4dcd`.

Tag0120 was installed and run after stable55 generation2 had been recovered
and verified idle. Its diagnostic reproduced the missing core0 embedding-read
response and left the candidate cold-latched at generation3 with an active
owner. The recovery lock and stable55 backup were retained. The SD-free NVMe
Linux-to-Haiku kexec handoff is verified and remains the recovery route; no
SD-card steps are required. The driver has no verified in-place ASIC reset
and post-reset initialization path yet. No token output, decode rate, warm-run
result, or clean unload has been established; 100 tokens/s remains unmet.
Assistant integration remains deferred. Build and run evidence are in
[`benchmarks/qwen35-dataflow-tag0120-20261121/BUILD.md`](benchmarks/qwen35-dataflow-tag0120-20261121/BUILD.md).

### In-place recovery guard foundation (2026-11-21)

The official `tt-kmd` Blackhole driver exposes an ASIC reset that sends the
PCIe interface timer interrupt after setting a reset marker. Its chardev
serializes the operation, invalidates the reset generation, zaps mappings,
reclaims TLB windows and iATUs, then marks hardware initialization and firmware
readiness as required. A separate POST_RESET wait/poll path gates reuse until
firmware is ready. Sources: [Blackhole reset callback](https://github.com/tenstorrent/tt-kmd/blob/6d84325ad6ba3c032c01adac2be01c9eb642fd1c/blackhole.c),
[reset ioctl lifecycle](https://github.com/tenstorrent/tt-kmd/blob/6d84325ad6ba3c032c01adac2be01c9eb642fd1c/chardev.c),
[PCIe reset marker and timer trigger](https://github.com/tenstorrent/tt-kmd/blob/6d84325ad6ba3c032c01adac2be01c9eb642fd1c/pcie.c).
This is the design reference, not yet a Haiku ASIC-reset implementation. The
new resident proof gate is compiled and unit-tested, and an isolated
fake-PCI-tested helper encodes the three-write reset trigger. Neither one is
connected to a driver operation: they do not reset the ASIC, restore PCI
state, rebuild mappings, prove firmware readiness, or clear the currently
loaded tag0120 guard. A safe port must also account for Haiku's retained Qwen35
owner, host pages, mapped PCI windows, and persistent cold latch. It must not
merely clear the latch or retry the stuck NoC request.

The running board is reachable over Haiku SSH and the failed client has exited;
tag0120 remains cold-latched at generation3 with one owner and the recovery
lock retained. Both model files are intact. The stable55 driver is not loaded
or idle. No board-side reset or accelerator command has followed the failure.
The next permitted boot/recovery route is Fedora on the existing NVMe followed
by kexec to Haiku. No SD cards are used.
