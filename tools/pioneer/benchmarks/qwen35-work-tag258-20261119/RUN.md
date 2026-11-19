# Qwen3.5-4B Work tag58 hardware run

Date: 2026-11-19. The guarded tag58 client ran on Haiku hrev99999. It started
from stable041 generation2, loaded tag `2026111800010058` at generation3, and
restored stable041 at generation4.

## Model load and fault

Authentication took `99.633210` s. Vocabulary/tokenization took `12.869909`
s for the 18-token prompt. The client uploaded all 3,450 resident spans
(`2,740,498,432` bytes) and model loading took `223.299044` s.

ROOT faulted before prompt prefill with zero committed tokens:

```text
state=4 commits=0 pc=0x40060 prefilled=0
root_trace=0xd0008210
dispatcher_stage=0x58778210 dispatcher_detail=0x4
```

The receipt identifies worker119's 4-byte write to remote doorbell `0x74100`
still pending. The fault occurred before prompt processing, so no prefill
duration, valid output, or decode tokens/s exists for this run. The two warm
output requirement and the 100 tokens/s target remain unmet.

## No-reboot recovery

The tag58 client passed its first safe-idle proof, repeated the proof during
explicit Work STOP, and unloaded cleanly:

```text
P100A_Q35 unload_clean=1 state=2 active=0 cold=0
P100A_RELOAD driver55 tag2026102100010012 generation4 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY
SAFE_FAULT_RECOVERY_VERIFIED
```

The guarded wrapper quiesced and closed tag58, restored the hash-pinned
stable041 driver, and verified the hot reload at generation4 without
rebooting. The exclusive lock was removed, no inference client remained, and
the 4B and 27B model files retained their expected sizes. The hardware run
verified explicit error recovery without reboot; the retryable safe-proof
failure branch is covered by the offline WorkOwner regression, not triggered
by this particular fault.

## Evidence

Raw `work.stdout`, `work.stderr`, `work.exit`, final state, and postflight
output are in [`evidence`](evidence). SHA-256:

| File | SHA-256 |
| --- | --- |
| `work.stdout` | `e2092726a8b03b8263ed5f34777887c664aa40a67307e3cfdc1a6b4ebfdf2705` |
| `work.stderr` | `5859b6690da553040247db9deec2d53bc75de4f0ece1469c848c875b0f12b646` |
| `work.exit` | `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865` |
| `state` | `507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e` |

Candidate driver SHA-256: `6694d4041ce3104a9902f17bd52c61a454edc46235fd70f12f3ef4d00c5605de`.
