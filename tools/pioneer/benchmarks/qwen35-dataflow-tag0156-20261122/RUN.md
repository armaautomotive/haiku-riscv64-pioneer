# Qwen3.5 4B tag0156 graph qualification

The guarded one-run test followed an idle stable-driver and process preflight.
The pinned GGUF authenticated, and the 120-worker resident graph loaded
2,740,498,432 bytes.

| Phase | Result |
| --- | ---: |
| GGUF authentication | 68.767511 s |
| Vocabulary and prompt tokenization | 13.154974 s; 5 prompt tokens |
| Model load and graph setup | 607.662970 s |
| Failed prompt processing | 0.905952 s |
| Decode | No tokens; no decode rate |

Worker 0 failed its first graph-record fetch with `graph_fault=1`, step 0.
The fetch receipt reported stage 3 (NoC read), NoC stage 7 (counter timeout),
index 129 (`0x20c`), match mask `0xfd`, and no busy initiators. The remaining
recorded counters matched their expectations. Tenstorrent's
[Blackhole NIU counter reference](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/Counters.md)
marks the read-data-word counter at `0x20c` as unsupported to read. The next
rewrite excludes that register while retaining the supported counter and idle
proofs; it has not yet been tested on hardware.

Same-owner recovery closed the graph service, and the client reported a clean
model-bank unload. The wrapper restored stable driver55 generation7 without a
reboot; an independent check confirmed its expected SHA-256, `cold0`, no
active owner, no recovery lock, and both model files at their recorded sizes
(not new content hashes). This verifies recovery for this settled fault, not
for every possible stuck device or NoC failure.

Raw output and the wrapper are under `evidence/`; stderr is compressed without
altering its contents. This was a correctness diagnostic, not a throughput
benchmark.
