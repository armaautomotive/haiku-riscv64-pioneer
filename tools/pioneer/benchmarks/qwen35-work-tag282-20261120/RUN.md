# Tag82 guarded hardware result

Tag82 loaded the verified Qwen3.5-4B Q4_K_M weights but faulted before
producing a complete warm output. The wrapper observed the terminal Work
fault, ran the STOP safe-idle proof, unloaded cleanly, and restored stable
driver55 without rebooting.

| Phase | Result |
| --- | --- |
| Preflight | Passed; prior diagnostic exited; stable driver55 generation42 idle |
| Authentication / vocabulary and tokenization | 68.092964 s / 13.090489 s |
| Model load | 223.267901 s; 3,450 spans / 2,740,498,432 bytes |
| Prompt processing / decode | No valid timing record; no completed warm run |
| Output | No generated output to validate |
| Work unload / stable-driver restore | Passed; `unload_clean=1 state=2 active=0 cold=0`; stable driver55 generation44 idle |
| 27B model preserved | Passed; stat `0:1:600:18973870528` |

The failure occurred while the ROOT checked the peer's final projection
receipt. Tag82 peers alternated the final-batch destination between `0x98000`
and `0x9a000`, but the ROOT receipt check still expected `0x98000`. For the
type-8, 2,560-input shape, the last batch is in `0x9a000`. Tag83 updates that
ROOT proof to derive the expected slot. Raw client output, error output,
state, and exit status are in `evidence/`.
