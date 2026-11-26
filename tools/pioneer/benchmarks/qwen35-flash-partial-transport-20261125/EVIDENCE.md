# Hybrid attention partial transport witness, 2026-11-25

The guarded candidate reserves four fixed partial packets in each worker's
existing activation-cache L1 region, plus two local header-read buffers. A
packet has a 64-byte header and 1,024-byte sum payload. Header identity covers
the epoch, **producer** graph step, dense layer, token position, producer,
query head, and its aligned lane slice. An FNV checksum detects the tested
corruptions; it is not a cryptographic authenticity proof. Commit words are
cleared first, payload and header stores are made L1-visible, and the commits
are published last. Readers check a header, read the payload, read the header
again, and reject unstable, stale, non-finite, out-of-slice, or bad-checksum
results. The reducer step directly follows the producer step and names that
preceding step in its packet identity.

A guarded Blackhole IO adapter uses fixed local copies for its own packet and
NoC1 peer reads for remote packets. Its peer predicate limits the caller to a
reducer's exact KV/output group, source worker route, and the three fixed
header/payload destination ranges. The NoC primitive additionally checks local
worker identity, transfer shape and completion. The adapter requires an
external `FlashContextMatches` authority, an all-fleet producer barrier and a
consumer barrier. None is connected to the graph or driver. In particular, the
source L1 packets must remain immutable until all reducers and NoC transactions
finish, epoch/step identities must not recur without held-fleet clearing,
activation-sharing/role queues must not overlap this layout, and ownership and
physical visibility must be proven. These prerequisites are not established
by checksums or simulated reads.

`sh tools/pioneer/tests/run_pioneer_qwen35_flash_decode_partial_tests.sh`
passed ASan/UBSan. The protocol test exercises 2,552 allowed fixed peer routes
across ten positions and rejects wrong peers, addresses, lengths, identities,
torn headers, corrupted values, failed reads and changing contexts. The actual
producer, packet protocol and reducer were composed with sequential simulated
L1/NoC at positions 1 and 33 across all 120 worker assignments; a missing
barrier and a corrupted packet failed before output, then all output lanes
matched the prior sequential reference. No concurrent fabric or hardware
visibility was simulated. Partially effective producer publication requires
fleet retention and is not retried.

The guarded adapter and producer/reducer cross-compiled for RV32IMF. The probe
intentionally leaves the context authority unresolved; `memcpy` and `memset`
are also unresolved until linking. Individual producer/reducer frames were
256/160 bytes and publish/receive frames were 160/336 bytes; these are not a
whole-program stack bound. The normal profiled resident boot, text and dispatch
were rebuilt byte-for-byte identical to the prior control. No adapter image was
linked or run and no physical card was accessed. Full-model output, two warm
runs, clean unload, recovery without reboot and throughput remain unverified.
