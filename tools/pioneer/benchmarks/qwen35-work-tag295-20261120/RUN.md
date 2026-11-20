# Tag95 P100A hardware result

The guarded preflight found no 27B diagnostic/inference or Qwen Work process.
Stable driver55 was idle at generation 6, with no active owner or cold latch;
the runner verified the pinned 4B model and preserved 27B model. Tag95 loaded
as driver64 at generation 7 without reboot. Authentication took 67.424 s,
tokenization took 13.307 s, and model load took 224.218 s for 3,450 resident
spans (2,740,498,432 bytes).

The first request reached `prefilled=23` and dispatched the vocabulary
projection with one root group and 119 peer shards. It then faulted before the
first generated token. The captured worker 119 job had 130 groups, while
`PioneerQ38SelectPollSentinel` still rejected every group count above 16; the
Work sentinel selection therefore returned false before collecting completion
proofs. This matches the root's last checkpoint (`0x5d000004`, 119 workers)
and the peer's valid wait-loop status. No valid output, completed prompt-time
record, or decode rate was produced.

The client unloaded cleanly (`unload_clean=1 state=2 active=0 cold=0`). The
runner restored stable driver55 at generation 8 without reboot. Strict
postflight passed and classified the run as `SAFE_FAULT_RECOVERY_VERIFIED`.
The Tag95 fault is preserved as a diagnosis, not a throughput result. The
Tag96 follow-up widens the sentinel range only for the vocabulary argmax job,
then repeats the full hardware validation.

Raw process output, fault snapshot, exit status, and restored state are in
[`evidence/tag95-card/`](evidence/tag95-card/). Their SHA-256 values are:

- `state`: `507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e`
- `work.exit`: `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865`
- `work.stdout`: `509f1b094f5258018b0f12963542ea24e9e338b27bb211dfdfa5b87fe112be69`
- `work.stderr`: `84e129bc68c66d5d8127e0f61162a3d506726c63d17320c20a8190c51034ef45`
