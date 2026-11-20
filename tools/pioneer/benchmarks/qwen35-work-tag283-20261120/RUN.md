# Tag83 guarded hardware result

Tag83 completed two valid warm Qwen3.5-4B Q4_K_M Work runs. The corrected
ROOT receipt accepted the peer's alternating final buffer. Both outputs were
exact, and the wrapper unloaded Work and restored stable driver55 without a
reboot. The 100 decode-token/s goal remains unmet.

| Phase | Warm run 1 | Warm run 2 |
| --- | ---: | ---: |
| Model load (one time) | 221.081215 s; 3,450 spans / 2,740,498,432 bytes | — |
| Prompt processing | 23 tokens / 176.912233 s | 23 tokens / 176.914486 s |
| Decode | 16 tokens / 128.402860 s = 0.124608 tok/s | 16 tokens / 128.405694 s = 0.124605 tok/s |
| Output | `A B C D E F G H I J K L M N O P` | `A B C D E F G H I J K L M N O P` |

Authentication took 67.516919 s; vocabulary/tokenization took 13.669213 s.
Both runs reported 5,920 full-core waves and 10,366 projection waves. The
peer execution-plus-completion interval was 98,011,542,340 cycles on run 1
and 98,011,661,776 cycles on run 2. ROOT matrix issue was about 44.79 billion
cycles per run. Alternating peer prefetch preserved correctness but did not
materially improve decode rate over Tag80/Tag81.

| Safety / restoration | Result |
| --- | --- |
| Preflight | Passed; prior diagnostic exited; stable driver55 generation44 idle |
| Clean Work unload | Passed; `unload_clean=1 state=2 active=0 cold=0` |
| Stable driver restore | Passed; driver55 generation46 idle, no active owner |
| 27B model preserved | Passed; stat `0:1:600:18973870528` |
| CPU inference/fallback | Disabled; projections use the distributed Tensix matrix path |

The measured decode rate is roughly 803 times below the 100 tok/s target.
Raw stdout/stderr, exit status, and final state are in `evidence/`.
