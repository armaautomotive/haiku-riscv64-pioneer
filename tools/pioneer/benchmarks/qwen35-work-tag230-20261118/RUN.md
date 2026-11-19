# Qwen3.5-4B Work tag230 run

Run date: 2026-11-18. Candidate driver tag `2026111800010030` was admitted
after the previous diagnostic process exited, stable041 generation2 was idle,
all64 CPUs/full RAM were present, the shared lock was absent, and both model
files matched their expected sizes.

| Measure | Result |
| --- | ---: |
| File authentication | 99.021867 s |
| Vocabulary and tokenization | 12.800956 s |
| Resident model spans / bytes | 3,450 / 2,740,498,432 |
| Model load and upload | 225.632552 s |
| Prompt tokens | 18 |
| Prefill / decode / generated tokens | Not reached |

The first projection faulted before prefill and before any committed token:
`state=3`, `commits=0`, `pc=0x40060`, `prefilled=0`,
`fault_marker=0x31463551`, `dispatcher_stage=0x54000216`,
`dispatcher_detail=0x01073620`. The detail decodes as worker1 and peer
return-address `0x73620`. In the generated peer disassembly this return
address is the shared fault block at the startup checks in
`q38_persistent_main`; it does not yet distinguish which startup predicate
failed. No valid output or decode rate was produced.

The Work client exited1. The guarded in-place restore stopped at
`cold1 active_owner1` and retained the lock. The trial wrapper synced stdout,
stderr, and the exit marker before attempting restore. The Haiku process list
showed the client had exited before recovery. Stable041 recovery and the next
diagnostic are pending a full board power cycle and guarded restore.

Artifacts:

- `work.stdout` SHA-256: `61fa5ade9c937f366ac2b9d61149863a7883be815c6fdd7ff2702a577184fb97`
- `work.stderr` SHA-256: `6e985ac7e2cf39dce6713a6f9300434c29823dd1d14272b327acc3860728798a`
- `work.exit` SHA-256: `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865`
- `state` SHA-256: `fbd1473a9abb5073bbd33c4b496a571ea742253449bef0484ab678f9efd1c9c4`
