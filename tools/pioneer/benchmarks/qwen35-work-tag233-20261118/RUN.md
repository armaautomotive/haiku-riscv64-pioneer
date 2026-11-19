# Qwen3.5-4B Work tag233 run

Run date: 2026-11-18. Admission passed with stable041 generation2 idle,
no inference process or shared lock, all 64 CPUs/full RAM, and both model
files intact. The staged driver/client/script hashes matched. The guarded
wrapper exchanged stable041 for tag233 driver60 generation3.

| Measure | Result |
| --- | ---: |
| File authentication | 100.890229 s |
| Vocabulary and prompt tokenization | 13.185885 s |
| Prompt tokens | 18 |
| Resident spans / bytes | 3,450 / 2,740,498,432 |
| Model load and upload | 224.100909 s |
| Prefill / decode / committed tokens | Not reached / not reached / 0 |

The first Work request failed before prefill or a token commit. The client
reported `state=3`, `pc=0x40060`, `progress_magic=0x31503542`, `prefilled=0`,
`root_trace=0xd0000350`, `fault_marker=0x31463551`,
`dispatcher_stage=0x54000350`, and `dispatcher_detail=0x010723c4`.
The entry marker proves `q35_dispatch_matrix` was reached. Stage `0x350`
decodes to peer fault reason `0x50`, worker 1; source maps reason `0x50` to
`panels != count` after the peer's `q38_compute_once()`. No decode rate or
valid output was produced.

The client exited 1 and was absent from the process list. The candidate was
`cold1 active_owner1`, so the wrapper correctly stopped before restore. The
stdout, stderr, exit and state files were copied before recovery:

| Artifact | SHA-256 |
| --- | --- |
| `work.stdout` | `6b143e99b688906236b5acc5edc68b7eec8851d9ccb9d28c273fed1f56b9f87a` |
| `work.stderr` | `99bc5dfb6ad41c65750e9bd2f9c9dbf0e029c15468799becdfecefa93627be72` |
| `work.exit` | `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865` |
| failed `state` | `fbd1473a9abb5073bbd33c4b496a571ea742253449bef0484ab678f9efd1c9c4` |

After the client exited, one OFF pulse was followed by a full three minute
settle interval and one ON pulse. Fedora had no inference process or
Tenstorrent module, 64 CPUs were online, and the card was unbound. The saved
Haiku helper (`8e85463e4de06fc8b35d0970e697c310f9532df30747c58cc892a1e9fdd9adff`)
and UART companion hashes were verified from a volatile `/tmp` staging
directory before the separate prepare/boot actions. Haiku returned with tag33
generation1 `cold0 active_owner0`; the guarded recovery verified all staged
hashes, both model files, CPUs and RAM, then restored stable041 generation2
`cold0 active_owner0 guard1` and removed the shared lock. The 27B setup remains
intact.

No 100 tokens/sec criterion was met. The next diagnostic should record the
actual `panels` and expected `count` values from the first peer failure; this
run only established that they differed after the matrix worker path.

Remote trial stage: `/boot/home/pioneer-qwen35-work-233.Gc43x`.
