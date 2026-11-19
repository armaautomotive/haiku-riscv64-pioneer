# Qwen3.5-4B Work tag238 diagnostic run

## Preflight and run

The guarded launch passed strict preflight from stable041 generation2 idle.
The prior inference process had exited, the shared lock was absent, Haiku
exposed all 64 CPUs and full RAM, and the Qwen3.5-4B and Qwen3.8-27B model
files had their expected sizes. All eight staged tag238 artifacts matched
their local SHA-256 values.

Authentication took 99.328953 s. Prompt tokenization took 13.509440 s for 18
tokens. Model load/upload took 226.217448 s for 3,450 resident spans
(2,740,498,432 bytes). These are setup timings, not decode throughput.

The run failed before prefill or token output:

```text
state=3 commits=0 pc=0x40060 progress_magic=0x31503542 prefilled=0
root_trace=0xe500000e fault_marker=0x31463551
dispatcher_stage=0x54f002e6 dispatcher_detail=0xa
dispatcher_context0=0xbf61fd40 dispatcher_context1=0xd000a00
scale_before_clear=0xaaf6efed scale_after_clear=0
scale_after_stage_input=0 scale_at_stageband_entry=0
scale_after_batch_build=0xbf61fd40 scale_after_root_job=0xbf61fd40
scale_after_worker_table=0xbf61fd40 scale_before_dispatch=0xbf61fd40
scale_before_validation=0xbf61fd40
```

This is the padded slot10 for Q5_K (type13, input width2560). It remained zero
after `StageInput` returned and at `StageBand` entry, then contained
`0xbf61fd40` by the first snapshot after `PioneerQ35BuildWorkBatch`. The
later snapshots kept that value. Tag239 should split `StageBand` between its
initial validation, fragment construction, state validation, and the batch
builder itself. Do not alter validation or projection behavior.

No prefill, valid output, clean Work unload, or decode rate was produced. The
client exited status1 and the wrapper stopped before restore because the
candidate driver reported `cold1 active_owner1`.

## Recovery

The remote state and logs were SHA-256 checked locally before recovery, along
with the stable041 backup. One OFF pulse was sent, followed by the required
180-second shutdown wait. The pinned Haiku SSH endpoint timed out; one ON
pulse brought the pinned Linux host back. Linux checks confirmed the
accelerator was unbound, with no Tenstorrent module or inference process,
PCI COMMAND `0000`, and an empty kexec slot.

The pinned boot and UART helpers were staged and verified:

| Helper | SHA-256 |
| --- | --- |
| `pioneer_linux_boot_haiku_smp.sh` | `8e85463e4de06fc8b35d0970e697c310f9532df30747c58cc892a1e9fdd9adff` |
| `pioneer_linux_uart_keepalive.sh` | `a9056dbc4317cc52bea715d9687ba223dee6957ca5e625d91cba8f6fb7071a22` |

The 64-CPU SD-free profile `memory128-spin-idle7-top-page-nosd-control` was
prepared and kexeced with session `qwen35tag238_20261118`. Haiku returned
with tag238 generation1 idle. The hash-pinned tag238 recovery helper restored
stable041 at generation2 and removed the shared lock. Post-recovery checks
confirmed no inference process, 64 CPUs/full RAM, and unchanged model sizes:

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
| `work.stdout` | `df2c27c58250feaa4471ec051ad9d3b9ee45b54d395f3779ae84811699cd7713` |
| `work.stderr` | `109001d66e1ce1356cc886bd197bea083b7dcedb87206ed1817cca21ca0ef07d` |
| `restore.incoming` (stable041 backup) | `041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403` |
| Candidate driver | `399b5ae39e7a066b7fdc8a10fb50ae68856f5d74cffeaee8514ec2e06685ce66` |
| Work client | `8e70e6fd4b9db0cb62e4f8e7143b6051d297fadcf85dfa758dccdfa6e1839eda` |
| Trial wrapper | `9c8d0836228c37e020b45c348b06972dabd7b7c88b5b6a54002b44481c08abb1` |
| Recovery helper | `00949936354778102d7515ab94f7e2f0e6f09218be4ffbca8b67fe92f0634ab5` |

No valid warm output or decode rate has been achieved.
