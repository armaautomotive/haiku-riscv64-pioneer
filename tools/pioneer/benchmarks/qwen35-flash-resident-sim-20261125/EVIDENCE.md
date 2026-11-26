# Hybrid attention resident graph simulation, 2026-11-25

The guarded recorder emits two adjacent hybrid dense records per dense layer:
producer, graph barrier, reducer, graph barrier. Each worker's producer writes
committed L1 partials and a local producer receipt. The reducer requires that
receipt for the preceding step, matching epoch, layer, position and core; the
peer transport checks source assignment and packet identity. A graph-local
context checks the run ticket, worker mailbox, activation epoch/step, idle
role mailboxes and clean queues. These checks do not establish a physical card
owner, firmware/DMA exclusion, or safe recovery. The model stream is assumed
sealed and the graph barriers must remain authoritative. Activation-reuse
metadata is invalidated when the producer overwrites the activation cache.

The 120-worker recorder test checks all eight dense layers, producer/reducer
adjacency and identical programs, invalid phases, and position-one dynamic
patching for both 529-step prompt and 532-step decode graphs. The runtime is
strictly compile-only; normal dense-run records are rejected in this variant.
Header packaging rejects its `flash-hybrid.marker` and the simulator requires
an explicit matching flag and fixture marker. Its 10,044-byte extra code
section is linked at `0xd0000`, which overlaps the old ROOT scratch region:
it is **simulator-only**, with no driver save/restore or hardware authority.
Unprofiled simulation is also required. Do not load this image on the card.

The exploratory patched Blackhole simulator executed synthetic dense tests at
positions 2/4 and 33/64 with all 120 producer workers; every KV byte and all
9,216 output floats matched the host operator reference within maximum absolute
error `1.86264515e-09`. Each run completed five graph barriers and deliberately
faulted on record six. Position 33 exercises peer reductions over 30 temporal
shards per KV head. Key/value packing still uses four workers each. Logs:
`/private/tmp/q35-flash-dense-sim-pos2-20261125.log` and
`/private/tmp/q35-flash-dense-sim-pos33-20261125.log`. These are synthetic,
non-concurrent with real hardware, and are not throughput measurements.

The simulator-only image has 56,776 bytes of primary text, 27,992 bytes of
dispatch and 10,044 bytes of extra code. The conservative generic and hybrid
stack frame sums including a 1,024-byte allowance are 7,920/8,192 and
6,240/8,192 respectively, not whole-program stack proofs. The normal profiled
resident image rebuilt byte-identically, and its original synthetic dense
control still passed. The simulator-only `flash.bin` SHA-256 is
`a60a697f9a56c325771c96cbc0fbb801037b0a2da0d868cffa9bd9bdb3af7c03`.
The ordinary resident and qualification packagers reject the marker.

The authenticated 4B GGUF has been recorded into a seven-bank four-layer
fixture (67 completed graph records plus a deliberate fault) and a complete
532-record decode fixture. The full fixture stages 2,740,473,856 model bytes;
its records SHA-256 is
`2d9d43561b387c9e39b9a8263c8f7897f5113671e28ce77389e947798e1d739f`.
At this checkpoint the actual four-layer hybrid simulation is **still running**
(`/private/tmp/q35-flash-recorded-four-layer-sim-20261125.log`); the full
hybrid fixture has not been executed. The separate full-model baseline
simulation is also still running
(`/private/tmp/q35-recorded-full-decode-sim-20261125b.log`). No output or
performance from either in-progress test is claimed.

The simulator is exploratory and includes unqualified matrix changes. No
physical P100A test, complete-model result, two warm runs, clean physical
unload, no-reboot recovery, or decode tokens/sec is established here.
