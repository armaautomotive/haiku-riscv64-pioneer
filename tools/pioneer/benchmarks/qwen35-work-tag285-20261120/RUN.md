# Tag85 guarded hardware result

Tag85 completed two valid warm Qwen3.5-4B Q4_K_M Work runs and then restored
stable driver55. Both generated the exact expected output. The 100 decode
tokens/sec goal remains unmet.

| Phase | Warm run 1 | Warm run 2 |
| --- | ---: | ---: |
| Authentication | 68.160197 s | — |
| Vocabulary/tokenization | 13.673632 s | — |
| Model load (one time) | 222.290266 s; 3,450 spans / 2,740,498,432 bytes | — |
| Prompt processing | 23 tokens / 176.587185 s | 23 tokens / 176.583342 s |
| Decode | 16 tokens / 128.179857 s = 0.124825 tok/s | 16 tokens / 128.184424 s = 0.124820 tok/s |
| Output | `A B C D E F G H I J K L M N O P` | `A B C D E F G H I J K L M N O P` |

The new peer counter summed the maximum active-peer `mcycle` delta from each
decode dispatch: 58,785,393,854 cycles across 4,170 samples in run 1, and
58,785,389,966 cycles across 4,170 samples in run 2. ROOT's peer-wait/proof
phase was 97,604,888,578 and 97,605,316,934 cycles; this interval also
contains ROOT polling and physical audits, so the peer counter is not the
whole wait phase. ROOT matrix issue was 44,777,804,558 and 44,777,744,574
cycles. Both runs reported 5,920 full-core waves and 10,366 projection waves.

Tag85 averaged 0.124823 tok/s, effectively unchanged from Tag84's 0.124813
tok/s average. Measuring peer compute did not reveal a single small overhead
fix, and the run still falls about 800x short of 100 tok/s. The next
performance attempt needs to change the projection dataflow and residency
pattern substantially; further read-only audit batching is not enough.

| Safety / restoration | Result |
| --- | --- |
| Preflight | Passed; previous diagnostic absent; stable driver55 generation48 idle |
| Clean Work unload | Passed; client exit 0; `unload_clean=1 state=2 active=0 cold=0` |
| Stable driver restore | Passed; driver55 generation50, `quiesced0 cold0 active_owner0 open_count1 guard1`; exact stable driver hash |
| 27B model preserved | Passed; stat `0:1:600:18973870528` |
| Candidate run wrapper | Exit 1 because the requested 100 tok/s threshold was not met; postflight confirmed both valid outputs and a restored board |
| CPU inference/fallback | Disabled; projections use the distributed all-core Tensix matrix path |

Raw stdout/stderr, client exit status, wrapper postflight, and final board state
are preserved in `evidence/`.
