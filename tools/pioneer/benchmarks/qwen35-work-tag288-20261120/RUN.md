# Tag88 guarded hardware result

Tag88 completed two warm Qwen3.5-4B Q4_K_M Work runs. Both produced exactly
`A B C D E F G H I J K L M N O P`. The model loaded once and remained resident
for both requests. The 100 decode tok/s goal remains unmet.

| Phase | Warm run 1 | Warm run 2 |
| --- | ---: | ---: |
| Authentication | 67.652396 s | — |
| Vocabulary/tokenization | 13.887950 s | — |
| Model load (one time) | 222.753216 s; 3,450 spans / 2,740,498,432 bytes | — |
| Prompt processing | 23 tokens / 179.674690 s | 23 tokens / 179.681048 s |
| Decode | 16 tokens / 130.496106 s = **0.122609 tok/s** | 16 tokens / 130.492699 s = **0.122612 tok/s** |
| Output | `A B C D E F G H I J K L M N O P` | `A B C D E F G H I J K L M N O P` |

Mean decode rate was 0.122611 tok/s, about 816 times below 100 tok/s. This
ROOT `-O3` build did not improve on Tag86's 0.124764 tok/s; it was 1.73% slower
across these two runs. Tag87's larger batches were slower still. Both Tag88
runs used all 120 Tensix cores: 5,920 full-core waves and 10,366 projection
waves per run. Work mode kept CPU inference and fallback disabled.

| Safety / restoration | Result |
| --- | --- |
| Preflight | Passed before PCI access: no prior diagnostic; stable55 generation54 idle with `active_owner0`; 4B and 27B identities matched |
| Clean Work unload | Passed; client exit 0; `unload_clean=1 state=2 active=0 cold=0` |
| Stable driver restore | Passed; stable55 generation56, `quiesced0 cold0 active_owner0 open_count1 guard1`; exact stable driver SHA restored |
| 27B model preserved | Passed; stat remains `0:1:600:18973870528` |
| Trial lock | Absent after completion |
| Reboot | None |
| Candidate wrapper exit | 1 because the 100 tok/s target was not met; valid outputs and restoration passed postflight |

Raw stdout/stderr, client exit status, wrapper output, preflight, postflight,
and final board state are preserved in `evidence/`.
