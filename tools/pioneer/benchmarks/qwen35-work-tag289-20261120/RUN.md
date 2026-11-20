# Tag89 guarded hardware result

Tag89 completed two warm Qwen3.5-4B Q4_K_M Work runs. Both returned exactly
`A B C D E F G H I J K L M N O P`; model weights were loaded once and stayed
resident for both requests.

| Phase | Warm run 1 | Warm run 2 |
| --- | ---: | ---: |
| Authentication | 67.571456 s | — |
| Vocabulary/tokenization | 12.749779 s | — |
| Model load (one time) | 222.271902 s; 3,450 spans / 2,740,498,432 bytes | — |
| Prompt processing | 23 tokens / 178.644306 s | 23 tokens / 178.641929 s |
| Decode | 16 tokens / 129.791933 s = **0.123274 tok/s** | 16 tokens / 129.787509 s = **0.123278 tok/s** |
| Output | `A B C D E F G H I J K L M N O P` | `A B C D E F G H I J K L M N O P` |

Mean decode rate was 0.123276 tok/s: 0.54% above Tag88, 1.19% below Tag86,
and about 811 times below the 100 tok/s goal. Removing per-projection physical
PC/reset reads made only a small change: peer wait/proof cycles fell from
95.85B to 94.46B, while dispatch issue/root matrix work remained about 45.69B.
This audit change alone does not address the dominant runtime.

Both runs used all 120 Tensix cores, with 5,920 full-core waves and 10,366
projection waves. Work mode used Tensix matrix operations without CPU
inference or fallback.

| Safety / restoration | Result |
| --- | --- |
| Preflight | Passed before PCI access; prior diagnostic absent; stable55 generation56 idle with `active_owner0`; 4B and 27B model identities matched |
| Candidate | driver64 Tag89 generation57 |
| Client exit | 0; both exact outputs and `unload_clean=1 state=2 active=0 cold=0` |
| Stable driver restore | Passed; stable55 generation58, idle with `active_owner0`; exact stable driver restored |
| Trial lock / inference process | Absent after completion |
| 27B model | Preserved; stat remains `0:1:600:18973870528` |
| Reboot | None |
| Trial wrapper exit | 1 because the 100 tok/s threshold was not met; client output was valid and cleanup passed |

Raw client output, progress, exit status, and final state are in `evidence/`.
