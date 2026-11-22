# Qwen3.5 4B tag0153 graph qualification

On 2026-11-22, the previous 27B diagnostic and inference clients were absent,
and the stable driver reported `cold0 active_owner0` before the card was
used. The 4B GGUF authenticated against the pinned SHA-256. The one-run
diagnostic installed the 120-worker graph and loaded 2,740,498,432 bytes.

| Phase | Result |
| --- | ---: |
| GGUF authentication | 101.192867 s |
| Vocabulary and prompt tokenization | 13.030747 s; 5 prompt tokens |
| Model load and graph setup | 607.268323 s |
| Failed prompt processing | 0.902885 s |
| Decode | No tokens; no decode rate |

Generation failed with result `31`, stage `32770` (`0x8002`), graph session
retained and zero output tokens. This result does not distinguish graph
staging, publication, or completion. The graph-fault recovery reached the
restored-fleet resume step and returned `18` (resume failure); the following
close attempt failed stop audit on core 1. The original resume stage and
core were not included in the recovery response. The unload was not clean.
The wrapper correctly left the candidate installed and its recovery lock in
place when status reported `cold1 active_owner1`.

After a filesystem sync, one authorized OFF hold, a settling interval, and
one ON pulse, Fedora booted on the existing NVMe. The hash-verified 64-core
Haiku profile booted through the recorded kexec path. The guarded recovery
script verified the fresh candidate was idle and restored stable driver55.
An independent check found generation2, `cold0 active_owner0`, no lock, and
stable driver SHA-256
`041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403`.
The 4B and 27B files had their recorded sizes; those checks are not fresh
content hashes. This was a cold recovery, not a successful no-reboot recovery.

The raw diagnostic output, wrapper, state and recovery script are under
`evidence/`; stderr is gzip-compressed without altering its contents. This was
a correctness attempt, not a warm throughput benchmark.
