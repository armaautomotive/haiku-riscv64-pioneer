# Qwen3.5-4B Work tag237 diagnostic run

## Preflight and run

The guarded launch passed preflight from stable041 generation2 idle. The
previous diagnostic had exited, the shared lock was absent, Haiku exposed all
64 CPUs and full RAM, and the 4B and 27B model files were intact. Tag237 then
loaded candidate driver60 generation3. Authentication took 98.969825 s,
prompt tokenization took 14.004913 s for 18 tokens, and model load/upload took
224.960505 s for 3,450 resident spans (2,740,498,432 bytes).

The run failed before prefill or token output:

```text
state=3 commits=0 pc=0x40060 progress_magic=0x31503542 prefilled=0
root_trace=0xe500000e fault_marker=0x31463551
dispatcher_stage=0x54f002e6 dispatcher_detail=0xa
dispatcher_context0=0xbf61fd40 dispatcher_context1=0xd000a00
scale_before_clear=0xaa74efed scale_after_clear=0
scale_before_dispatch=0xbf61fd40 scale_before_validation=0xbf61fd40
```

The dispatcher rejected padded scale slot10 for a Q5_K projection (type13,
input width2560). The tail slot was nonzero before clearing, zero after
clearing, then nonzero again before dispatch and validation. The write is
therefore between the clear and `q35_dispatch_matrix()`, not in dispatcher
validation. Tag238 should add snapshots after `StageInput`, at `StageBand`
entry, after batch construction, after root job setup, and after worker-table
construction. Keep the snapshots failure-only and preserve scale validation.

No prefill, valid output, or decode throughput was produced. Authentication,
prompt tokenization, and model load are setup measurements, not decode rate.

## Recovery

The inference process exited with status1; the board was still running tag237
driver60 `cold1 active_owner1` with the shared lock present. Local copies of
the run output, failure state, and stable041 backup were checked against the
remote SHA-256 values before recovery.

One OFF pulse was sent, followed by the required 180-second shutdown wait. The
pinned Haiku SSH endpoint timed out after the wait, so one ON pulse was sent.
The pinned Linux endpoint returned. Read-only checks showed the accelerator
unbound, no Tenstorrent module or inference process, and PCI COMMAND `0000`.
The boot helpers were staged and hash-verified:

| Helper | SHA-256 |
| --- | --- |
| `pioneer_linux_boot_haiku_smp.sh` | `8e85463e4de06fc8b35d0970e697c310f9532df30747c58cc892a1e9fdd9adff` |
| `pioneer_linux_uart_keepalive.sh` | `a9056dbc4317cc52bea715d9687ba223dee6957ca5e625d91cba8f6fb7071a22` |

The pinned 64-CPU, 128-GiB, SD-free profile
`memory128-spin-idle7-top-page-nosd-control` was prepared and kexeced to
Haiku. After boot, tag237 was idle at generation1 and the recovery helper
restored the original stable041 driver at generation2. The helper hash is
`bcdefaf2d8c51e2d028aae340fa23ac7ff126f9d53f0e217bc89da5209622441`.
The post-recovery checks confirmed no inference process, no shared lock,
stable041 `cold0 active_owner0 guard1`, and unchanged model sizes:

```text
P100A_RELOAD driver55 tag2026102100010012 generation2 quiesced0
  cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY
Qwen3.5-4B-Q4_K_M.gguf: 2740937888 bytes
Qwen3.8-27B-Q4_K_M.gguf: 18973870528 bytes
```

## Archived artifact hashes

| Artifact | SHA-256 |
| --- | --- |
| Final `state` (`STABLE041_RESTORED`) | `507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e` |
| `state.pre-recovery` | `fbd1473a9abb5073bbd33c4b496a571ea742253449bef0484ab678f9efd1c9c4` |
| `failure.state` | `fbd1473a9abb5073bbd33c4b496a571ea742253449bef0484ab678f9efd1c9c4` |
| `work.exit` | `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865` |
| `work.stdout` | `e5cb14bcb16de94020c35a1fe70b5cf5f545b9d93618804e2d5f056197bfd737` |
| `work.stderr` | `bbcd63c34d7fee021b29867f9c5a99034c04cd8618d2ffc3b83d60a311caf82e` |
| `restore.incoming` (stable041 backup) | `041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403` |
| Candidate driver | `ff0883e59a7bdf16a39d1ec51042ffd460e78709cf4abd1f22fdf11310b9a5f1` |
| Work client | `d7cb9b8065411afa032da7de35b4aeb0e34fac8811dfacfc3cb54170e92b7643` |
| Trial wrapper | `b76e50057227689b982e2a8308d40385052634ae8c592557751a259b98dcf75d` |
| Recovery helper | `bcdefaf2d8c51e2d028aae340fa23ac7ff126f9d53f0e217bc89da5209622441` |

No valid warm output or decode rate has been achieved.
