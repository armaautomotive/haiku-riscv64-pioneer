# Qwen3.5-4B Work tag234 run

This was an all-core Work-only hardware attempt on 2026-11-18, starting from
checkpoint `94117b2279`. Before launch, the prior diagnostic was gone,
stable041 was idle at generation 2 (`cold0 active_owner0 guard1`), the shared
lock was absent, Haiku reported 64 CPUs and full memory, and both the 4B and
27B model files were intact.

The run authenticated the model in 100.234451 s, tokenized the 18-token prompt
in 13.322161 s, and loaded/uploaded 3,450 resident spans totaling
2,740,498,432 bytes in 220.861532 s. It failed before prefill and before the
first token. The terminal record was `state=3`, `pc=0x40064`,
`progress_magic=0x31503542`, `prefilled=0`, `root_trace=0xe500000e`,
`fault_marker=0`, `dispatcher_stage=0`, and `dispatcher_detail=0`.

`0xe500000e` is `StageBand()` failure reason 14: the dispatcher returned a
worker count different from the scheduled count. The checkpoint was empty, so
the tag233 peer panel-count diagnosis was not reached or did not persist. No
valid output, prompt-processing measurement, decode measurement, or speed
result was produced. The candidate was marked cold/owned and was not reused.

The client exited before a controlled power cycle. Recovery used the pinned
SD-free Linux helper to boot Haiku. Post-recovery, stable041 was restored at
generation 2 (`cold0 active_owner0 guard1`) and the shared lock was absent.
The 27B setup remains preserved.

Logs and hashes:

| Artifact | SHA-256 |
| --- | --- |
| `state` | `fbd1473a9abb5073bbd33c4b496a571ea742253449bef0484ab678f9efd1c9c4` |
| `work.exit` | `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865` |
| `work.stdout` | `95a966b57f91189a168c3bbfe4dd9322eff4bdcbc60b528a58f97535e3052459` |
| `work.stderr` | `2ef0e89059ced61fb9dd8987dcfaeeedc60dcc5c8fc3d0480ac5e66a5d85338f` |

The remote stage was `/boot/home/pioneer-qwen35-work-234.Gc44x`. Build hashes
and the source change are recorded in [BUILD.md](BUILD.md).
