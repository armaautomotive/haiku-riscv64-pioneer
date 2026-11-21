# Tag0105 service-ready failure

The model authenticated and tokenized, all 2,740,498,432 resident bytes were
uploaded, model binding passed, and route proof completed. Service installation
then returned `result=11 state=2 next_sequence=0 recovery=1`, the
`DataflowServiceReadyFailed` path. No prompt or generation ran, so there is no
model-load, prompt-processing, or decode-rate measurement.

The client reported `cleanup=0 fd_closed=1`; the guarded runner retained state
`RECOVERY_REQUIRED_UNTIL_STABLE_RESTORED` and recorded exit code 1. The driver
reported `cold1 active_owner1`, so the wrapper kept the candidate, the exact
stable041 backup, and the recovery lock. After the authorized power recovery
and pinned SD-free kexec into Haiku, the candidate was idle at generation1
with `cold0`. The committed recovery helper
`pioneer_qwen35_dataflow_recover_after_power_cycle.sh` then restored stable
driver55 at generation2 and removed the lock. The final read-only postflight
is in `recovery-after-kexec.txt`.

| File | SHA-256 |
| --- | --- |
| `state` | `fbd1473a9abb5073bbd33c4b496a571ea742253449bef0484ab678f9efd1c9c4` |
| `work.exit` | `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865` |
| `work.stdout` | `9592e9d3f05ea94e435e1bdb4e7ee5bbfa438730837ce4e3721c0c09a15c67bd` |
| `work.stderr` | `bd612b804cb045e266891b4f44b7e9d6fba38b6b66cbf692077d39b3b285e9a8` |
| `recovery-after-kexec.txt` | `f0be6f0076ebc075e52f1f664fcdfaada41914d4b26e7d5ecf70d241f57ba8f8` |
