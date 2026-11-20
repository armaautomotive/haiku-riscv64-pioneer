# Tag84 guarded hardware result

Tag84 completed two valid warm Qwen3.5-4B Q4_K_M Work runs. The expected
output was exact in both runs, and the wrapper cleanly unloaded Work and
restored stable driver55 without a reboot. The 100 decode-token/s goal remains
unmet.

| Phase | Warm run 1 | Warm run 2 |
| --- | ---: | ---: |
| Model load (one time) | 221.617540 s; 3,450 spans / 2,740,498,432 bytes | — |
| Prompt processing | 23 tokens / 176.613846 s | 23 tokens / 176.612423 s |
| Decode | 16 tokens / 128.191027 s = 0.124814 tok/s | 16 tokens / 128.192363 s = 0.124812 tok/s |
| Output | `A B C D E F G H I J K L M N O P` | `A B C D E F G H I J K L M N O P` |

Authentication took 67.521790 s; vocabulary/tokenization took 12.445252 s.
Both runs reported 5,920 full-core waves and 10,366 projection waves. ROOT
matrix issue was 44,785,369,364 and 44,785,333,589 cycles. The peer
execution-plus-completion interval was 97,605,859,423 and 97,605,779,903
cycles. The concurrent read-only physical audit reduced this interval by
about 0.4% versus Tag83, while decode improved about 0.17%. It is a small
gain, so this audit-only change does not address the 100 tok/s gap.

| Safety / restoration | Result |
| --- | --- |
| Preflight | Passed; no prior diagnostic; stable driver55 generation46 idle |
| Clean Work unload | Passed; exact two warm outputs and exit status 0 |
| Stable driver restore | Passed; driver55 generation48 idle, no active owner |
| 27B model preserved | Passed; stat `0:1:600:18973870528` |
| CPU inference/fallback | Disabled; projections use the distributed Tensix matrix path |

Raw stdout/stderr, exit status, and final state are in `evidence/`.
