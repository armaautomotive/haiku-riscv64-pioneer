# Qwen3.5 4B tag0157 graph qualification

The guarded one-run test began after confirming no prior 27B diagnostic or
inference client was running and the stable driver was idle. The pinned 4B
GGUF authenticated; the resident graph used 120 workers and loaded
2,740,498,432 bytes.

| Phase | Result |
| --- | ---: |
| GGUF authentication | 67.528036 s |
| Vocabulary and prompt tokenization | 12.869089 s; 5 prompt tokens |
| Model load and graph setup | 613.444221 s |
| Failed prompt processing | 0.912184 s |
| Decode | No tokens; no decode rate |

Worker 0 completed two graph steps before an execution-uncertain failure at
step 2; no token was produced. The recorded order identifies the first two
steps as embedding and residual save, with RMS normalization next. This is
progress beyond tag0156's first-record fetch failure, but it does not by itself
identify the exact RMS suboperation.

Source review found a definite RMS defect: the scale record was passed to
the NoC from the BRISC stack at `0x70000` and below, outside the permitted
`0x80000..0xa0000` local transfer range. The subsequent rewrite stages that
record in an aligned input page in the permitted range and records RMS/NoC
failure details. The change has not yet been verified on hardware.

Same-owner recovery and model-bank unload succeeded; the wrapper restored the
stable driver without rebooting. Independent checks found stable driver55
generation9, the expected SHA-256, `cold0 active_owner0`, no lock and both
model files at their recorded sizes (not new content hashes). This demonstrates
recovery for this settled graph fault, not every possible device failure.

Raw output and the wrapper are under `evidence/`; stderr is compressed without
altering its contents. This was a correctness diagnostic, not a throughput
benchmark.
