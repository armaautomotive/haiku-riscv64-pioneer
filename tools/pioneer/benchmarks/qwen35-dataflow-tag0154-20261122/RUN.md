# Qwen3.5 4B tag0154 graph qualification

The guarded one-run test followed a check that no 27B diagnostic or other
inference client was running and the stable driver was idle. The pinned 4B
GGUF authenticated successfully. The qualification driver used the resident
120-worker graph and loaded 2,740,498,432 bytes.

| Phase | Result |
| --- | ---: |
| GGUF authentication | 100.364879 s |
| Vocabulary and prompt tokenization | 12.840542 s; 5 prompt tokens |
| Model load and graph setup | 618.119882 s |
| Failed prompt processing | 0.912492 s |
| Decode | No tokens; no decode rate |

Generation failed with result `31`, stage `0x5200`: the graph commands were
published, but worker 0 did not pass the completion check. The result does not
identify whether it timed out or reported a specific device fault. No valid
output was produced.

The graph-fault audit reached service restoration, but the restored-fleet
resume audit failed at stage 3, core 1, detail `0x111` (the image was treated
as the dataflow service but its mailbox was not in the required service idle
state). The bounded retries repeated the final audit without restarting the
workers, then left the candidate and lock retained with `cold1 active_owner1`.
This was not a clean unload or successful no-reboot recovery. Source review
found that the service and original dispatcher use the same boot word while
the service-image cache was not invalidated on restoration; the subsequent
rewrite invalidates that proof before held-core writes. That fix requires
hardware qualification.

The raw output and recovery script are in `evidence/`; stderr is compressed
without altering its contents. After an authorized power cycle and pinned
NVMe handoff, the guarded script restored stable driver55 generation2. An
independent check reported `cold0 active_owner0`, no lock, the expected
stable driver SHA-256, and both model files at their recorded sizes (not new
content hashes). The run is a correctness diagnostic, not a warm throughput
benchmark.
