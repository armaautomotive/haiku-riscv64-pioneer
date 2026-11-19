# Qwen3.5-4B Work tag231 run

Run date: 2026-11-18. Candidate tag `2026111800010031` passed admission with
stable041 generation2 idle, no inference process/shared lock, all64 CPUs/full
RAM and both model files intact.

| Measure | Result |
| --- | ---: |
| File authentication | 101.863929 s |
| Vocabulary and tokenization | 12.802406 s |
| Resident model spans / bytes | 3,450 / 2,740,498,432 |
| Model load and upload | 228.542734 s |
| Prompt tokens | 18 |
| Prefill / decode / generated tokens | Not reached |

The request reached the Work root but returned before its first prefill commit:
`state=3`, `commits=0`, `pc=0x40064`, `progress_magic=0x31503542`,
`prefilled=0`, `ready_reason=0`, `fault_marker=0`,
`dispatcher_stage=0`, `dispatcher_detail=0`. The root disassembly identifies
`0x40064` as the second instruction in `q35_work_fault_park`, reached after
`q35_work_main` returned0. The peer startup-specific diagnostics did not
publish a reason, so the next iteration needs root/token-core failure-site
instrumentation before another card run.

The client exited1; the guarded restore stopped at `cold1 active_owner1` and
retained the lock. Stdout, stderr and exit marker were synced by the wrapper
and archived before recovery. No valid output or decode rate was produced.

Artifacts:

- `work.stdout` SHA-256: `5cd577d5bc0ae20d23432179bc4f6080ecd15c901c595513ad841efd774716bf`
- `work.stderr` SHA-256: `fc15c3ab5d4dce9d690ae7f0c26c12f3c5cd2924a9b8b880a6de89db6c2a0947`
- `work.exit` SHA-256: `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865`
- `state` SHA-256: `fbd1473a9abb5073bbd33c4b496a571ea742253449bef0484ab678f9efd1c9c4`
