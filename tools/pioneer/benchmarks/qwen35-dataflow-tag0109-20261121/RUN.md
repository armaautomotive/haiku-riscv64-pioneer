# Tag0109 hardware run

No tag0109 hardware run has been performed yet.

Before touching the card, run the guarded `--preflight` mode from the Haiku
root account with stable driver55 generation and the exact candidate/client
hashes in [`BUILD.md`](BUILD.md). The wrapper verifies that the 27B diagnostic
and inference clients are absent, stable driver55 is idle, both model files
have their pinned sizes, all 64 CPUs and full RAM are available, and there is
no stale exclusive lock. Use the stage name
`/boot/home/pioneer-qwen35-dataflow-0109.d` for the candidate payload.

Run the one-shot `--run` mode only after preflight succeeds. It preserves the
stable driver backup, captures stdout/stderr/exit status, validates two warm
outputs, checks separate model-load, prompt, and decode timings, and restores
stable driver55 only after clean service release. If restoration cannot be
proved, the wrapper retains the lock and backup for the tag0109 guarded
post-cycle recovery helper. Do not report a decode rate unless both warm
outputs pass validation; the target is at least 100 decode tokens/s.
