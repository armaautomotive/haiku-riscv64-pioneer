# Qwen3.5-4B Work Tag86: three-tile Q4_K batching

Tag86 increases the Q4_K (`type 12`) weight-fetch batch from two original 2,304-byte tiles to three (6,912 bytes), fitting inside the fixed 8 KiB buffer. It also retains four-tile Q8 and two-tile Q5/Q6 batching. The two alternating buffers overlap the next batch read with Tensix matrix work. No CPU inference or fallback is enabled. The existing Work descriptor, source-seal, idle/response proofs, and byte counters remain authoritative.

The batch planner accepts a partial final batch and copies only the requested original tile into the established matrix input address. The bounded `/3` ordinal uses a reciprocal multiply, preserving the Work image's no-integer-divide requirement.

## Verification

The cross-build passed for Tag86, including the fixed-image disassembly checks. The peer image is 8,164 bytes of its 8,192-byte limit (28 bytes spare). The planner passes 5,136 cases, and the reciprocal `/3` ordinal was checked exactly for inputs 0 through 65,535.

| Artifact | SHA-256 | Size |
| --- | --- | ---: |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` | 4 B |
| ROOT image text | `f25ced5aead91b87f8f595116d88748a8e5604b239780a28b2f7bea5db6f81f5` | 60,564 B |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` | 4 B |
| Peer image text | `d8f143a312ce44877526a88ae11256a8d493ce218be09ddf034d9f0c3c034a82` | 8,164 B |
| Candidate driver55 | `a8461592b34ca841ab2d27a6bdc1eef5d1f09ce30727e20259ac3df653036f1d` | 255,816 B |
| Work client | `d8da309cdf9675c3e4e3610d2b6e6b99e0d7a519b091af26dadcf49d227d1c5d` | 304,848 B |

The guarded trial uses a private Tag286 stage, requires stable driver55 to be idle before exchange, runs two warm outputs, enforces the 100 decode tok/s goal, unloads, and restores stable driver55. Runtime results will be recorded in `RUN.md`.
