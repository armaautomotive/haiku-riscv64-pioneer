# Tag91 P100A hardware result

Tag91 completed two warm Qwen3.5-4B Q4_K_M Work requests on the Pioneer
P100A. Both returned exactly `A B C D E F G H I J K L M N O P`. The Work
image used the Tensix matrix engines across the 120-core layout, with no CPU
inference or Work-mode fallback.

| Phase | Warm run 1 | Warm run 2 |
| --- | ---: | ---: |
| Authentication | 98.984122 s (once) | — |
| Vocabulary/tokenization | 13.284161 s (once) | — |
| Model load | 226.936587 s; 3,450 spans / 2,740,498,432 resident bytes | — |
| Prompt processing | 23 tokens / 178.641664 s | 23 tokens / 178.651353 s |
| Decode | 16 tokens / 129.790283 s = **0.123276 tok/s** | 16 tokens / 129.777153 s = **0.123288 tok/s** |
| Output | `A B C D E F G H I J K L M N O P` | `A B C D E F G H I J K L M N O P` |

Mean decode was 0.123282 tok/s. This reproduces the Tag89 baseline and remains
about 811 times below 100 tok/s. Each run recorded 5,920 full-core waves and
10,366 projection waves, confirming participation by all 120 Tensix cores.
The 27B timing is not part of this run; its existing model file was preserved.

## Admission and cleanup

| Check | Result |
| --- | --- |
| Preflight | Prior inference/27B diagnostic absent; stable driver55 tag `2026102100010012`, generation 2, `cold0 active_owner0`; 64-CPU Haiku profile healthy |
| 4B model | `0:1:600:2740937888` before and after |
| 27B model | `0:1:600:18973870528` after; unchanged from the preserved setup |
| Candidate | driver64 Tag91, generation 3; exact SHA-256 is recorded in `BUILD.md` |
| Client result | Exit 0; both exact outputs; `unload_clean=1 state=2 active=0 cold=0` |
| Stable restore | Exact stable driver55 restored at generation 4; `cold0 active_owner0` |
| Trial lock / inference process | Absent after completion |
| Reboot | None |
| Trial wrapper exit | 1 only because its 100 tok/s gate was not met; postflight accepted both valid outputs and verified restoration |

Captured card logs and state are in `evidence/tag91-card/`. The trial script is
`pioneer_qwen35_work_trial.sh`.
