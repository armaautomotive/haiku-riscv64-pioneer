# Tag0106 worker-0 startup failure

The guarded trial started from stable driver55 at generation2 after verifying
the prior 27B diagnostic process was absent, both model file sizes, board health,
and candidate hashes. Variant-60 driver tag `2026112100010106` hot-swapped at
generation3 without reboot. Authentication took 99.351922 seconds and prompt
tokenization took 13.028432 seconds for IDs `760,6511,314,9338,369`.

The client uploaded the full 2,740,498,432-byte model and successfully bound
its authority. Resident service installation failed at the worker-ready wait:

```text
P100A_Q35_DATAFLOW install_failed result=11 state=2 next_sequence=0 recovery=1 diag_stage=2 diag_core=0 worker_state=6 worker_failure=4 worker_phase=2
P100A_Q35_DATAFLOW model setup failed bytes=2740498432 authority_bound=1 install_attempted=1
P100A_Q35_DATAFLOW cleanup=0 fd_closed=1
P100A_Q35_DATAFLOW unload_clean=0 state=0 active=0 cold=0 service_closed=0
```

State6 is the worker fatal state, failure4 is `DataflowFailureQueue`, and
phase2 is the local-core route check. No prompt processing or generated token
occurred. Because installation failed, there is no model-load, prompt, or
decode timing. The target 100 decode tokens/s remains unmeasured.

The guarded wrapper refused to replace the candidate after status reported
`driver60 tag2026112100010106 generation3 quiesced0 cold1 active_owner1`.
It retained the exact stable041 backup and recovery lock. This trial therefore
did not validate the requested no-reboot recovery behavior. The cleanup code
returned failure, but tag0106 did not expose which all-core stop-audit predicate
failed. Next code checkpoint will add bounded startup settling plus stop-audit
stage/core diagnostics before another guarded run.

Raw logs were copied from the Pioneer without alteration:

| File | SHA-256 |
| --- | --- |
| `state` | `fbd1473a9abb5073bbd33c4b496a571ea742253449bef0484ab678f9efd1c9c4` |
| `work.exit` | `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865` |
| `work.stdout` | `c124ada514d4c79f22f1a90a08bffe80dfbd9b88f2196acfbbc84ceab9c6b314` |
| `work.stderr` | `e3e27e3e4908f41615f7c3dff1fb7417698fbb964edef00af4ad57e54e5300a0` |

