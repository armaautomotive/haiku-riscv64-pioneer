# Tag81 guarded hardware result

Candidate: Qwen35 batch-fetch of adjacent original weight tiles. The wrapper
verified the prior diagnostic had exited, stable driver55 was idle at
generation40, the 27B model remained intact, and candidate/model hashes
matched. It hot-reloaded without reboot, ran two warm prompts, unloaded Work,
and restored stable driver55 idle at generation42.

| Phase | Result |
| --- | --- |
| Preflight | Passed; stable driver55 generation40 idle; 27B stat `0:1:600:18973870528` |
| Authentication / vocabulary and tokenization | 67.754298 s / 13.029521 s |
| Model load | 225.980095 s; 3,450 spans / 2,740,498,432 bytes |
| Warm run 1 prompt / decode | 23 tokens / 178.655797 s; 16 tokens / 129.706268 s = 0.123356 tok/s |
| Warm run 2 prompt / decode | 23 tokens / 178.653339 s; 16 tokens / 129.712341 s = 0.123350 tok/s |
| Output | `A B C D E F G H I J K L M N O P` on both runs |
| Clean unload and stable-driver restore | Passed; `unload_clean=1 state=2 active=0 cold=0`; driver55 generation42 idle |
| 27B model preserved | Passed postflight; stat unchanged |

Each run reported 5,920 full-core waves and 10,366 projection waves. The peer
completion interval (peer execution plus completion proof) was about 100.447
billion cycles per run. Compared with Tag80's 0.12461 tok/s, this is about 1%
slower; the target remains over 810 times above the measured rate. Raw client
output, exit status, state, preflight/postflight records, and hashes are in
`evidence/`.

Raw output, exit status, preflight/postflight state, and checksums will be
stored in `evidence/`.
