# Qwen3.5-4B Work Tag90 hardware run

Tag90 (`2026112000010090`) was run once on the Pioneer P100A, then rejected.
The following notes preserve the full failure and recovery outcome.

## Preflight and model load

The read-only admission checks passed before the candidate exchange: the prior
diagnostic had exited, the host process inventory was idle, and stable driver55
was tag2026102100010012 generation58 with `cold0 active_owner0`. The 4B model
was exactly 2,740,937,888 bytes and the untouched 27B model exactly
18,973,870,528 bytes. All 120 Tensix workers were admitted. The Work image had
no CPU inference or fallback path.

Tag90 loaded as driver64 generation59. Model authentication took 67.560400 s,
vocabulary and prompt tokenization took 13.213172 s for 18 prompt tokens, and
the model upload took 225.163435 s for 3,450 spans and 2,740,498,432 resident
bytes. These are setup measurements, not inference throughput.

## Work fault

The first request failed in the first projection before prefill and before any
committed or generated token. No warm output, decode time, or tokens/sec value
exists for Tag90. The captured terminal record included:

```text
state=4 commits=0 pc=0x40064 prefilled=0
root_trace=0xd0000390 dispatcher_stage=0x54000390
dispatcher_detail=0x23072544
worker35 projection validation fault reason=0x90
STOP recovery failed proof=60 detail=0x1e retryable=0
unload_clean=0
```

The peer reported projection precheck reason `0x90`. The fault snapshot later
showed a complete descriptor in the peer job slot. This is consistent with
the peer observing word15's sequence wake marker before the earlier descriptor
words from the same packet had arrived, but the logs do not conclusively prove
that ordering. The combined descriptor/doorbell protocol is rejected and must
not be retried.

STOP's initial fleet safety proof passed. Recovery then reset and restarted
the peers; a restarted peer faulted during the readiness audit and the client
reported non-retryable proof60. The kernel retained the Work owner and locked
the driver cold: `driver64/tag90/generation59/cold1/active_owner1`. The model
client exited. No other inference or 27B diagnostic process remained.

## Recovery

The exact failure output, state, and hashes were copied locally before
recovery. The Tag90 install, stable041 backup, and both model identities were
rechecked. One seven-second OFF button press was sent; three bounded SSH checks
through the 180-second settle interval timed out. One 0.75-second ON press
followed. The board booted Fedora Linux from unchanged NVMe; the Linux host pin
and Linux SSH key were both verified strictly.

The pinned, previously tested 64-CPU/full-memory/SD-free Haiku kexec helpers
were hash-verified. Their disk-identity checks passed, and the handoff returned
to Haiku without changing the default boot configuration. On the new Haiku
epoch, Tag90 was idle at generation1. A dedicated fail-closed recovery helper
verified the saved artifacts and both model files, quiesced Tag90, and restored
stable041. It finished at generation2 with `cold0 active_owner0 guard1` and
removed the trial lock. No inference ran during restoration.

Post-recovery checks confirmed the 27B file remained 18,973,870,528 bytes and
the 4B file remained 2,740,937,888 bytes. The installed driver SHA is the
stable041 SHA `041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403`.
See `evidence/tag90-card/` for raw logs and post-recovery status.

## Outcome

Tag90 did not produce valid output, clean in-place unload, prompt timing, or
decode timing. The 100 tok/s goal remains unmet. Tag91 restores Tag89's
separate descriptor-then-doorbell protocol; no claim of a no-reboot recovery
fix is made yet.
