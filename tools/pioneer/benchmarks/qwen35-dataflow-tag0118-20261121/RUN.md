# Qwen3.5 dataflow rewrite tag0118 hardware run

## Outcome

The guarded tag0118 diagnostic loaded the full Qwen3.5-4B model and entered
the first prompt embedding, but core0's NoC completion-counter read timed out.
The new receipt captured both sides of the mismatch: counter0 at index36 was
`0x00029066`; the expected value was `0x00029067`, one count higher. The run
produced no token and did not reach decode. This is a failed diagnostic, not a
valid generation or a throughput result.

The driver reported `result=31`, stage4, `output=0`, prompt operation
`615329` us, and `decode_us=0`. The worker receipt was core0, state4/failed,
failure7 (`EmbeddingWeightRead`), phase5, pending sequence1. QUERY/RECOVER
reported audit stage6/core0/detail1508. CLOSE failed its core0 stop audit at
stage7. The candidate remained `cold1 active_owner1` at generation3, so the
guarded wrapper retained its lock and stable-driver backup.

After the user's accidental power reset, the board was recovered through its
normal Linux boot and the pinned, SD-free Haiku kexec path. The guarded
post-boot helper verified the candidate, backup, model sizes, and Haiku health;
it quiesced driver60, restored stable driver55 at generation2, and removed the
exclusive lock. The post-recovery snapshot confirms no inference process,
stable55 idle, 64 CPUs, full RAM, and intact 4B/27B model files.

Do not start the two-warm throughput benchmark until the rewrite produces a
valid output and unloads cleanly. The next rewrite diagnostic should expose a
read-only completion-counter match mask and neighboring NoC/counter state to
determine whether the command was issued and observed on the intended route.
Do not reset counters, clear NoC commands, or reset Tensix cores as part of
that diagnostic.

## Phase timings

| Phase | Result |
| --- | ---: |
| GGUF authentication | 68.186682 s |
| Vocabulary and prompt tokenization | 13.766015 s; IDs `760,6511,314,9338,369` |
| Model load and resident setup | 334.004328 s; 2740498432 resident bytes |
| Prompt processing | Failed after 0.615329 s; partial time only |
| Decode | Not reached; no decode time or tokens/s |

The two-warm benchmark was not started. The requested 100 decode tokens/s and
clean Work unload gates remain unmet.

## Recovery evidence

`haiku-post-failure-state.txt` records the cold candidate, retained lock,
stable backup hash, and unchanged model sizes. `haiku-recovery.txt` records
the guarded quiesce, stable55 restore, generation2 verification, and lock
release. `haiku-post-recovery-state.txt` independently confirms the final
idle state and intact model files. The raw client stdout/stderr and both
process exit codes are retained alongside them. No wrapper log was produced.

The build and offline checks are recorded in [`BUILD.md`](BUILD.md).
