# Qwen3.5 dataflow first correct output, tag0166

After the prior diagnostic exited and stable driver55 generation12 was idle,
a guarded single-run diagnostic authenticated the model in 68.926384 s,
loaded the vocabulary in 14.810262 s, and loaded the model in 617.160613 s.
The five-token prompt took 19.503739 s; the one subsequent decode took
4.190600 s, or **0.238629 decode tokens/s**. The generated token IDs
`11751,13` detokenized to ` Paris.`, matching the pinned reference. This is
one correct warm diagnostic, not the required two-run qualification or the
100 tokens/s goal.

The raw FP32 trace showed nonzero first and final projection outputs. In the
sampled first QKV panel, the SFPU Dst coefficients matched their L1 source
values and the separately calculated model weights; the Dst minima also
matched the independently decoded Q5_K reference. This confirms the two
metadata fields reached the sampled vector rows on this run.

The client returned `work.exit=0` and reported `unload_clean=1 state=2
active=0 cold=0 service_closed=1`. The wrapper restored stable driver55
generation14 without reboot and removed the lock, but its final validation
returned failure because its awk program contained an invalid line break.
Thus the wrapper itself did not certify the result; the recorded client
output, exit status, and separately checked stable-driver state establish the
single-run observation. The corrected two-run wrapper and its synthetic
validator self-test are preserved in `evidence/`; the self-test passed locally
and on Haiku before use.

Independent post-run checks found no diagnostic process, owner or lock,
`cold0`, stable SHA-256
`041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403`, and
both model files at their recorded sizes; these are not fresh full hashes.
The exact single-run guard and client outputs are under `evidence/`.

## Two verified warm runs

A separate guarded qualification using the same tag0166 driver and client
loaded the model once, then ran the five-token prompt twice. Both runs
produced the verified `11751,13` (` Paris.`) output, and the corrected guard
reported `TWO_WARM_OUTPUTS_VALIDATED`. Authentication was 69.030922 s,
vocabulary setup 13.081918 s, and model load 614.806798 s.

| Warm run | Prompt seconds | Decode seconds | Decode tokens/s |
| --- | ---: | ---: | ---: |
| 1 | 19.469486 | 4.183633 | 0.239027 |
| 2 | 19.464716 | 4.184250 | 0.238991 |

Each decode time covers one token after the first output token, which is
included in prompt time. The two-token outputs are a correctness check and a
short decode measurement, not a long-run throughput benchmark. They meet the
two valid warm outputs requirement but remain far below 100 decode tokens/s.

The client exited 0 and reported clean unload. The wrapper restored stable
driver55 generation16 without reboot. Independent checks found no diagnostic
process, owner or lock; `cold0`; stable driver SHA-256
`041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403`; and both
model files present at their recorded sizes. Size checks are not fresh full
content hashes. Exact logs, the corrected guard, and the Haiku-tested validator
self-test are in `evidence/warm/`. The tested runtime code was pushed as part
of checkpoint `3c0fbc8acb`; later commits preserve evidence and add a separate,
not-yet-hardware-tested timing candidate.
