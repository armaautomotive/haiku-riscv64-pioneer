# Qwen3.5-4B Work tag240 hardware run

## Preflight and setup timing

Strict pinned preflight passed from stable041 generation2 idle before staging
or driver exchange. Haiku had no inference process, no shared lock, 64 CPUs
and full RAM. The Qwen3.5-4B and Qwen3.8-27B model files were intact at
2,740,937,888 and 18,973,870,528 bytes.

Tag240 (`2026111800010040`) authenticated in 99.230719 s, tokenized the
18-token prompt in 12.842984 s, and loaded/uploaded 3,450 resident spans
(2,740,498,432 bytes) in 223.801107 s. These are setup measurements, not
decode throughput.

The driver exchanged cleanly to tag0040 at generation3. After model load,
Work faulted before prefill and before producing any token:

```text
state=3 commits=0 pc=0x40060 progress_magic=0x31503542 prefilled=0
root_trace=0xe500000e fault_marker=0x31463551
dispatcher_stage=0x54f002e6 dispatcher_detail=0xa
dispatcher_context0=0xbf61fd40 dispatcher_context1=0xd000a00
scale_before_clear=0xa8f4efed scale_after_clear=0
scale_after_stage_input=0 scale_at_stageband_entry=0
scale_after_initial_validation=0 scale_after_fragments=0
scale_after_state_validation=0 scale_before_batch_object=0xbf61fd40
scale_after_batch_object=0xbf61fd40 scale_after_batch_build=0xbf61fd40
scale_after_batch_fields=0xbf61fd40 scale_after_root_job=0xbf61fd40
scale_after_worker_table=0xbf61fd40 scale_before_dispatch=0xbf61fd40
scale_before_validation=0xbf61fd40
```

Unlike tag239, the Work owner correctly captured the 15 failure snapshots
from the request-cleared `RootFault` marker even when the parked PC was
`WorkFaultPc`. The padded Q5_K scale remains zero through the after-state-
validation checkpoint and is already `0xbf61fd40` at the first checkpoint
inside the batch loop, before the batch object is initialized. The precise
write between those observations is not yet identified. No prefill, valid
output, clean Work unload, warm-run result, or decode-rate measurement exists.

The client exited with status1. The wrapper stopped before restore because the
candidate driver reported `cold1 active_owner1`; the client itself had exited
and no inference process remained. The run logs and their pre-recovery hashes
are archived beside this file.

## Recovery

After archiving state and logs, the documented single-cycle recovery was
followed: one OFF pulse (seven-second hold), a 180-second wait, a pinned Haiku
SSH timeout, then one ON pulse (0.75-second hold). The returned Linux host
passed strict checks: Linux 6.1.31 on the recorded NVMe root, CPUs 0-63 online,
empty kexec slot, P100A unbound with PCI `COMMAND=0000`, no Tenstorrent module
or inference process, and no MMC device.

The exact Linux recovery helpers passed their pinned hashes. The
`memory128-spin-idle7-top-page-nosd-control` Haiku image was prepared and
kexeced through normal `systemctl kexec` in session
`qwen35tag240_20261118`. Haiku returned with candidate tag0040 generation1
idle. The hash-pinned tag240 recovery helper restored the original stable041
driver at generation2 and removed the shared lock.

Post-recovery verification confirmed:

```text
P100A_RELOAD driver55 tag2026102100010012 generation2 quiesced0
  cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY
Qwen3.5-4B-Q4_K_M.gguf: 2740937888 bytes
Qwen3.8-27B-Q4_K_M.gguf: 18973870528 bytes
installed stable041 SHA-256: 041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
shared lock: absent
```

## Artifact hashes

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55, tag `2026111800010040` | `2ec4b9598ea57b67cbf0149ac595c178e66f98aed5ea92e284149c6472c9e779` |
| Work client | `dd3e91d51f9cc2f08002494cc55ed478b2272d2e42ec65f450e64377e58bfe2a` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `1ea760acf8db762f0ef7a20cfd755000e529a590e00a56a1285f732f60e40cf6` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `feef2e513367a83f93c11142037c59e2a6af5aed1d7921d9cf031daa4fe63047` |
| Trial wrapper | `b674597e6265a15a48394b29d608395b529ccd953f9485cfe38400e281763412` |
| Recovery helper | `79a5cca432403bcf8ff1dc2d5485a1cc46cda94ac7c6de3976d2a855e48eab8a` |
| Linux Haiku boot helper | `8e85463e4de06fc8b35d0970e697c310f9532df30747c58cc892a1e9fdd9adff` |
| Linux UART helper | `a9056dbc4317cc52bea715d9687ba223dee6957ca5e625d91cba8f6fb7071a22` |
| `state.pre-recovery` | `fbd1473a9abb5073bbd33c4b496a571ea742253449bef0484ab678f9efd1c9c4` |
| `failure.state` | `fbd1473a9abb5073bbd33c4b496a571ea742253449bef0484ab678f9efd1c9c4` |
| `work.exit` | `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865` |
| `work.stdout` | `108b442869dee195b2e47f51ce2770f02a37a1e9ae9ff12891857b9e96f71f81` |
| `work.stderr` | `f006ba0d2d18ec79570aeb562158bb2b0b79753572cb326dd081d9e9088fe25c` |
| `restore.incoming` | `041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403` |
| Final `state` | `507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e` |

The verified stable041 idle status, pre-recovery process list and hashes are
stored in `status.pre-recovery`; the final stable041 status is in
`status.post-recovery`. There is no valid decode-rate measurement yet.
