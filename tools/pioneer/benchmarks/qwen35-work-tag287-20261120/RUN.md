# Tag87 guarded hardware result

Tag87 completed two warm Qwen3.5-4B Q4_K_M Work runs. Both produced exactly
`A B C D E F G H I J K L M N O P`. The model loaded once and remained resident
for both requests. The 100 decode tok/s goal remains unmet.

| Phase | Warm run 1 | Warm run 2 |
| --- | ---: | ---: |
| Authentication | 68.229013 s | — |
| Vocabulary/tokenization | 13.114618 s | — |
| Model load (one time) | 223.795648 s; 3,450 spans / 2,740,498,432 bytes | — |
| Prompt processing | 23 tokens / 227.743221 s | 23 tokens / 227.751134 s |
| Decode | 16 tokens / 163.431528 s = **0.097900 tok/s** | 16 tokens / 163.427951 s = **0.097902 tok/s** |
| Output | `A B C D E F G H I J K L M N O P` | `A B C D E F G H I J K L M N O P` |

Mean decode rate was 0.097901 tok/s, about 1,021 times below 100 tok/s. Tag86
averaged 0.124764 tok/s, so Tag87 was 21.53% slower. The 16 KiB double-buffered
read experiment did not improve end-to-end speed. Both runs still used all
120 Tensix cores: 5,920 full-core waves and 10,366 projection waves per run.
Work mode kept CPU inference and fallback disabled.

| Safety / restoration | Result |
| --- | --- |
| Preflight | Passed before PCI access: no prior diagnostic; stable55 generation52 idle with `active_owner0`; 4B and 27B identities matched |
| Clean Work unload | Passed; client exit 0; `unload_clean=1 state=2 active=0 cold=0` |
| Stable driver restore | Passed; stable55 generation54, `quiesced0 cold0 active_owner0 open_count1 guard1`; exact stable driver SHA restored |
| 27B model preserved | Passed; stat remains `0:1:600:18973870528` |
| Trial lock | Absent after completion |
| Reboot | None |
| Candidate wrapper exit | 1 because the 100 tok/s target was not met; valid outputs and restoration passed postflight |

Raw stdout/stderr, client exit status, wrapper output, preflight, postflight,
and final board state are preserved in `evidence/`.
