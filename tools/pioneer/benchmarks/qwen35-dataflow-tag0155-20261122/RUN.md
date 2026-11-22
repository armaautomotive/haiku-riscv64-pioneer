# Qwen3.5 4B tag0155 graph qualification

The guarded one-run test started after confirming the earlier 27B diagnostic
and other inference clients were absent, the lock was clear, and the stable
driver was idle. The pinned 4B GGUF authenticated successfully. The resident
graph used 120 workers and loaded 2,740,498,432 bytes.

| Phase | Result |
| --- | ---: |
| GGUF authentication | 101.629518 s |
| Vocabulary and prompt tokenization | 12.852283 s; 5 prompt tokens |
| Model load and graph setup | 613.860418 s |
| Failed prompt processing | 0.910686 s |
| Decode | No tokens; no decode rate |

Generation returned result `31`, stage `0x5200`, without a token. The retained
worker-0 receipt reported `state=4 failure=4 phase=5`, graph fault `1`
(`DataflowGraphFetchFault`) at completed step zero. This establishes that the
first graph-record fetch failed, but not which part of the fetch failed.

The same-owner recovery returned service-closed result `17` with no recovery
required. The client reported `unload_clean=1`, bank state closed, no active
owner, no cold latch, and the service closed. Without rebooting, the wrapper
quiesced the candidate and restored stable driver55 at generation4. Its
diagnostic validation failed because generation produced no tokens, as
expected. Independent checks found the stable driver's expected SHA-256,
`cold0 active_owner0`, no lock and both model files at their recorded sizes
(not fresh content hashes). The first independent status showed generation5;
two further status reads stayed at generation5. The reason for that single
generation advance was not established. This proves recovery for this settled
graph fault, not for all possible stuck device or NoC failures.

Raw output and the wrapper are under `evidence/`; stderr is compressed without
altering its contents. This was a correctness diagnostic, not a warm
throughput benchmark.
