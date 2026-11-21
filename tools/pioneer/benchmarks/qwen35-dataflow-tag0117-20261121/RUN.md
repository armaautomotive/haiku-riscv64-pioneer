# Qwen3.5 dataflow rewrite tag0117 hardware run

The guarded one-run diagnostic completed authentication, vocabulary load and
prompt tokenization, and the full resident model load. Authentication took
`99.823865` s, vocabulary/tokenization took `12.747341` s, and model load took
`324.806932` s for `2740498432` resident bytes. The first prompt embedding then
failed after `0.602222` s on core0. No valid output token was produced, so
decode throughput is unavailable; a second warm run was not started.

Tag0117 captured the NoC failure stage and observed value:

```text
recovery_audit_stage=6 recovery_audit_core=0 recovery_audit_detail=1508
failed_read_stage=7 failed_read_index=36 failed_read_status=0x00029066
```

Stage7 is the completion-counter wait; index36 is counter0. Tag0117 did not
capture the expected counter value, so this result cannot yet distinguish a
counter-selection/delta mismatch from a transfer that failed to complete.
The bounded recovery audit retained the candidate at `cold1 active_owner1`,
with the stable backup and trial lock intact.

Following the accidental power reset, the pinned Haiku recovery helper restored
the exact stable driver55 build at generation2 and removed the trial lock. A
strict status check confirmed idle state, no inference process, and unchanged
4B and 27B model sizes (`2740937888` and `18973870528` bytes). Stable driver
SHA-256 is `041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403`.
Tag0118 adds expected-counter capture and a read-only diagnostic query; its
focused hardware diagnostic is pending. Raw tag0117 logs and state evidence are
in `evidence/`.
