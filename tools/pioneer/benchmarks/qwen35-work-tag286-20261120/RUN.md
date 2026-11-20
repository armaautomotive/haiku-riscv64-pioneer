# Tag86 guarded hardware result

Tag86 completed two valid warm Qwen3.5-4B Q4_K_M Work runs. The output in each run was exactly `A B C D E F G H I J K L M N O P`. The three-tile Q4_K fetch change did not improve decode speed; the 100 decode tok/s goal remains unmet.

| Phase | Warm run 1 | Warm run 2 |
| --- | ---: | ---: |
| Authentication | 68.175162 s | — |
| Vocabulary/tokenization | 12.696884 s | — |
| Model load (one time) | 225.315081 s; 3,450 spans / 2,740,498,432 bytes | — |
| Prompt processing | 23 tokens / 176.680959 s | 23 tokens / 176.679377 s |
| Decode | 16 tokens / 128.242955 s = **0.124763 tok/s** | 16 tokens / 128.240979 s = **0.124765 tok/s** |
| Output | `A B C D E F G H I J K L M N O P` | `A B C D E F G H I J K L M N O P` |

Mean decode rate was 0.124764 tok/s, about 801 times below 100 tok/s. Tag85 averaged 0.124823 tok/s, so this Tag86 run was 0.047% slower within run-to-run noise. Increasing Q4_K batches from two to three reduced expected weight-read command count, but did not produce a measurable end-to-end gain. The trace still reports roughly 44.7B ROOT matrix-issue cycles and 97.7B peer-wait/proof cycles per 16-token decode; the peer critical-path counter is about 58.78B cycles across 4,170 samples per run. The next performance change needs to address the projection dataflow and residency/dispatch cost, rather than another small read-batch adjustment.

All 120 Tensix cores remained assigned to the matrix projection path. Both runs reported 5,920 full-core waves and 10,366 projection waves. Work mode kept CPU inference and fallback disabled.

| Safety / restoration | Result |
| --- | --- |
| Preflight | Passed before PCI access: prior diagnostic absent; stable55 generation50 idle, `active_owner0`; both 4B and 27B model identities exact |
| Clean Work unload | Passed; client exit 0; `unload_clean=1 state=2 active=0 cold=0` |
| Stable driver restore | Passed; stable55 generation52, `quiesced0 cold0 active_owner0 open_count1 guard1`; exact stable driver SHA restored |
| 27B model preserved | Passed; stat remains `0:1:600:18973870528` |
| Trial lock | Absent after completion |
| Reboot | None |
| Candidate wrapper exit | 1 because the requested 100 tok/s threshold was not met; valid outputs and restoration passed postflight |

Raw client stdout/stderr, client exit status, wrapper state, preflight, and final board state are preserved in `evidence/`.
