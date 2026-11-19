# Qwen3.5-4B Work tag53 hardware run

Date: 2026-11-19. The diagnostic client used tag `2026111800010053` and was
guarded by the tag53 trial wrapper. Haiku hrev99999 started idle on stable041
generation2; the prior diagnostic had exited, the shared lock was absent, all
64 CPUs and full RAM were available, and both model files matched their
expected sizes.

## Load and failure

The client authenticated in `99.599927` seconds, tokenized an 18-token prompt
in `12.441273` seconds, and loaded all 3,450 resident spans
(`2,740,498,432` bytes) in `224.169305` seconds. It then faulted before
prefill with zero committed tokens:

```text
state=4 commits=0 pc=0x40060 prefilled=0
root_trace=0xd0008210
dispatcher_stage=0x58778210 dispatcher_detail=0x4
```

Stage `0x58778210` identifies worker119's pending 4-byte write to remote
`0x74100` (the peer doorbell), with the NoC command still busy and no counter
mismatch. No valid output, prompt processing, or decode rate was produced.

The tag53 terminal-fault receipt captured the ROOT NoC command and counters,
plus worker119 status and two PC samples, after the all-core safe-idle proof:

```text
root_noc_snapshot_valid=1
command=0x8e85000,0,0x492,0x90000,0,0x81,0,0,0xb00,0
counter_delta=0xb,0x2b2,0xf9,0xb,0xee,0xee,0xf9,0xee
worker119_snapshot_valid=1
pc=0x720a8,0x720a4
status=0x1,0,0x46503851,0x32503851,0x1,0x723c8,0x50,0,0,0,0,0,0,0,0,0
```

The peer image defines status reason `0x50` as a mismatch between completed
panels and the expected panel count. It writes those values to status words 7
and 8 before parking, but both captured values are zero. A validated job has
nonzero groups and input size, so the recorded detail does not explain the
mismatch. Correlate the ROOT command words with the doorbell before promoting
any dispatch change.

## Recovery

The client reported a fault exit (`1`) but completed explicit Work STOP and
clean unload:

```text
P100A_Q35 unload_clean=1 state=2 active=0 cold=0
```

The wrapper re-proved safe idle, quiesced and closed tag53, restored the
stable041 binary, and hot-reloaded it at generation4. Final state was
`driver55 tag2026102100010012 generation4 quiesced0 cold0 active_owner0
open_count1 guard1;CPU_METADATA_ONLY`. This completed without a board reboot.
The exclusive lock is absent; no inference process remains; the Qwen3.5-4B
and Qwen3.8-27B files remain `2,740,937,888` and `18,973,870,528` bytes.

Raw output, error, exit, state, postflight, system, process, and stable-driver
receipts are preserved in [`evidence`](evidence). Tag250 had already verified
the same explicit STOP/END recovery path without reboot; tag251's blocked
close-time auto-recovery remains reverted and must not be rerun.

No valid model output or decode rate has been measured. Recovery is verified;
the worker119 compute/doorbell fault remains unresolved.
