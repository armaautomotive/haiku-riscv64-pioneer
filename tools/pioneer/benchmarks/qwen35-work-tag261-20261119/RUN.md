# Qwen3.5-4B Work tag61 hardware run

Date: 2026-11-19. The guarded tag61 client ran on Haiku hrev99999, loading
from stable041 generation8 at candidate generation9. The wrapper restored
stable041 at generation10 without reboot.

## Load and diagnostic

Authentication took `67.641308` s. Vocabulary/tokenization took `12.638489` s
for the 18-token prompt. All 3,450 resident spans (`2,740,498,432` bytes)
loaded in `223.297109` s.

The first Work run faulted before prefill with zero committed tokens. The
outer checkpoint again recorded that the ROOT matrix returned zero bands:

```text
state=4 commits=0 prefilled=0
dispatcher_stage=0x5e000007 dispatcher_detail=0
dispatcher_context0=0 dispatcher_context1=0
```

The tag61 matrix image had internal shape/scale/activation receipts, but the
dispatcher replaced its stage/detail while recording the zero return. Thus
this run does not identify which internal validation returned zero. The
current ROOT job and worker119 snapshot are preserved in
`evidence/work.stderr`. No valid output, prefill, or decode rate was produced.

## Recovery

STOP passed the safe-idle proof, the client unloaded cleanly, and stable041
hot-reloaded at generation10:

```text
P100A_Q35 unload_clean=1 state=2 active=0 cold=0
P100A_RELOAD driver55 tag2026102100010012 generation10 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY
SAFE_FAULT_RECOVERY_VERIFIED
```

Both model files remain intact, no inference client or shared lock remains,
and the board did not reboot. Stable041 is idle.

## Evidence hashes

| File | SHA-256 |
| --- | --- |
| `work.stdout` | `944e248d2cc30dabfae05b177d2d4d5d3baf1814d94c66c81908f4422a1fa095` |
| `work.stderr` | `7d32e5b9ff0062f98970dd1a6ba8e53f7b020eea220b443b6b9546a89a2823f5` |
| `work.exit` | `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865` |
| `state` | `507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e` |
| `postflight.log` | `6cb3d4d4d94a9086d83c93f43d72698a082aa30ae63352942011f9635c62f6e6` |
