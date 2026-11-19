# Qwen3.5-4B Work tag236 diagnostic run

## Preflight and run

The guarded launch began with stable041 idle at generation2, the prior
diagnostic exited, the exclusive lock absent, 64 CPUs/full RAM visible, and
the Qwen3.5-4B and Qwen3.8-27B model files intact. Tag236 authenticated in
100.712738 s, tokenized the 18-token prompt in 13.525346 s, and loaded/uploaded
3,450 resident spans (2,740,498,432 bytes) in 227.309557 s.

The run failed before prefill or token output:

```text
state=3 commits=0 pc=0x40060 progress_magic=0x31503542 prefilled=0
root_trace=0xe500000e fault_marker=0x31463551
dispatcher_stage=0x54f002df dispatcher_detail=0x0000000a
dispatcher_context0=0xbf61fd40 dispatcher_context1=0x0d000a00
```

The receipt maps to padded-scale validation in `blackhole_qwen38_dispatch.cpp`.
It rejected scale slot10 for type13 (Q5_K), input width2560: the first padded
tail slot held raw F32 bits `0xbf61fd40` instead of zero. No prefill, valid
output, decode, or rate was produced. Setup timings above are not decode
throughput.

The compiled `StageInput` disassembly confirms the candidate contained a
loop that writes zero to every padded scale slot10–15 and a fence. Its stack
frame is 640 bytes; nested quantization uses 2,112 bytes, and the ROOT stack
starts at `0x70000`, far below the scale cache at `0x94000`. These checks do
not prove the write completed on the card or that no later writer changed the
slot. Tag237 should record the slot after clearing, before matrix dispatch,
and immediately before validation to identify the window.

## Recovery

The client exited, but tag236 remained cold/owned with the shared lock, so no
new diagnostic was started. The run artifacts and stable041 backup were
copied locally before recovery. One OFF pulse was sent; after the full three
minute wait both pinned SSH profiles timed out. One ON pulse was sent. The
pinned Linux host returned with the accelerator unbound, no Tenstorrent
module or inference process, and PCI COMMAND `0000`.

The exact Linux boot helper and UART companion hashes were verified before
preparing and kexecing the SD-free 64-CPU profile
`memory128-spin-idle7-top-page-nosd-control`. Haiku returned with tag36
generation1 idle. The first restore attempt stopped before mutation because
its script expected tag35 in two status checks. Those checks now use the
candidate tag variable; the corrected helper hash is listed in `BUILD.md`.
The second guarded restore quiesced tag36 and restored stable041 at generation2
(`cold0 active_owner0 guard1`), then removed the shared lock.

Post-recovery verification confirmed Haiku, 64 CPUs/full RAM, no inference
process, stable041 idle, the lock absent, and unchanged model files:

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
| `work.stdout` | `bd3127ad4c437b86aee104f4c8b5b3f23b21db5d7e70ffc646f59f96c034c47e` |
| `work.stderr` | `a7619538b9762cecaf808f2de4a47b8a1411d7c62765a2a188d54bd63dd0523c` |
| `restore.incoming` | `041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403` |
| Corrected recovery helper | `283fd9b05ae292d862a8b21a1fa17a61ad8d49a52e03826bcc2ee81ba66546bd` |

No valid warm output or decode rate has been achieved.
