# Qwen3.5-4B Work tag232 run

Run date: 2026-11-18. The staged trial passed admission with stable041
generation2 idle, no inference process/shared lock, all 64 CPUs/full RAM, and
the Qwen4B and Qwen27B model files intact.

| Measure | Result |
| --- | ---: |
| File authentication | 98.367249 s |
| Vocabulary and tokenization | 14.242949 s |
| Resident spans / bytes | 3,450 / 2,740,498,432 |
| Model load and upload | 223.450006 s |
| Prompt tokens | 18 |
| Prefill / decode / generated tokens | Not reached |

The first Work request failed before prefill or any token commit:
`state=3`, `commits=0`, `pc=0x40064`, `progress_magic=0x31503542`,
`prefilled=0`, `root_trace=0xda000077`, `fault_marker=0`,
`dispatcher_stage=0`, and `dispatcher_detail=0`. Root trace `0xda000077` is the
last trace immediately before `q35_dispatch_matrix()`, with 119 workers
requested. No checkpoint or peer fault reason was visible.

The client exited with code 1 and the trial wrapper correctly stopped at
`before_restore` because the candidate was `cold1 active_owner1`. No output or
decode rate was produced. The client process was absent before recovery. The
logs were copied to this directory before the power cycle.

The board was power-cycled once using the project relay procedure, returned
through the SHA-pinned Fedora-to-Haiku helper, and the tag232 recovery script
restored stable041 generation2 `cold0 active_owner0 guard1`; the shared lock is
absent. The guarded recovery rechecked both model file sizes and preserved the
27B setup.

| Artifact | SHA-256 |
| --- | --- |
| `work.stdout` | `dd34b5273fc577b9e70a6a5926a3e5083bef32758a21765f6920c28db9d75e16` |
| `work.stderr` | `ab927921307f64f1487d2947e2807ca824fa8fbcae668367439838d0c7279540` |
| `work.exit` | `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865` |
| failed `state` | `fbd1473a9abb5073bbd33c4b496a571ea742253449bef0484ab678f9efd1c9c4` |
