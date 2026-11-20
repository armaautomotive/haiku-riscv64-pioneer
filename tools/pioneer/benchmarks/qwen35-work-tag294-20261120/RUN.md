# Tag94 P100A hardware result

The Tag94 candidate ran on the pinned Pioneer at `10.0.0.247`. Before the
driver exchange, the process audit found no 27B diagnostic, inference worker,
or Qwen Work client. Stable driver55 was idle at generation 4, with
`cold0 active_owner0 open_count1`; the system reported 64 CPUs and the expected
137,034,936,320-byte memory profile. The runner also verified the pinned 4B
model and preserved 27B model identities before installation.

Tag94 loaded as driver64, tag `2026112000010094`, generation 5. The request
completed two warm runs with the exact output `A B C D E F G H I J K L M N O
P`. Each run used 23 prompt tokens and generated 16 output tokens. All-core
wave counters were present on both runs (`full_core_waves=5920`,
`projection_waves=10366`), and the down projection consumed the resident,
bank-sharded FFN activation buffer.

| Phase | Warm run 1 | Warm run 2 |
| --- | ---: | ---: |
| Authentication | 67.551 s | included once before runs |
| Vocabulary/tokenization | 13.295 s | included once before runs |
| Model load | 224.122 s | model remained resident |
| Prompt processing (23 tokens) | 180.166 s | 180.168 s |
| Decode (16 tokens) | 130.772 s | 130.768 s |
| Decode rate | 0.122351 tok/s | 0.122354 tok/s |

The run demonstrated correct output and clean unload. It did not meet the
100-token/s target. The nearly identical per-run rates show that the Tag94
activation-buffer connection did not materially improve decode throughput;
the next implementation needs to move the projection dataflow and scheduling
onto a persistent per-core device program.

After the task, the client reported
`unload_clean=1 state=2 active=0 cold=0`. The guarded script restored stable
driver55, tag `2026102100010012`, at generation 6 without reboot. Its strict
postflight passed and confirmed the clean stable state. The original 27B
model remained present and unchanged.

Raw card output, diagnostics, exit status, and restored state are preserved in
[`evidence/tag94-card/`](evidence/tag94-card/).
