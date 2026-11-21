# Qwen3.5 dataflow rewrite tag0118 hardware run

Status: focused diagnostic not run yet. The candidate is built and its staged
runner is prepared, but it has not been installed. The last verified card
state is stable driver55 generation2, idle, with no inference process or
exclusive lock. The 4B and 27B model files retain their recorded sizes.

Tag0118 is intended to repeat the single-prompt, two-token diagnostic only.
It will report the observed and expected completion-counter values on a NoC
counter timeout. Do not start the two-warm throughput benchmark until the
rewrite produces a valid output and can unload cleanly. If the candidate
cold-latches, retain the lock and stable backup and use the guarded recovery
helper only after a fresh Haiku boot.
