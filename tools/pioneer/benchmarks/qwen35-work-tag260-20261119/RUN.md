# Qwen3.5-4B Work tag60 hardware run

Date: 2026-11-19. The guarded tag60 client ran on Haiku hrev99999, loading
from stable041 generation6 at candidate generation7. The wrapper restored
stable041 at generation8 without reboot.

## Load and fault

Authentication took `68.115594` s. Vocabulary/tokenization took `13.258253` s
for the 18-token prompt. All 3,450 resident spans (`2,740,498,432` bytes)
loaded in `220.424219` s.

The first Work run faulted before prompt prefill with zero committed tokens.
Tag60 narrowed the failure to the returned ROOT matrix band count:

```text
state=4 commits=0 prefilled=0
dispatcher_stage=0x5e000007 dispatcher_detail=0
```

The ten-tile Q5_K ROOT job passed idle, geometry, and command readback, then
`q35_work_matrix_band()` returned zero instead of 160 bands. The preserved
job has type13, tile `0xb00`, input count `0xa00`, one group, route `0x492`,
and input address `0x8e85000`. This points to a validation return inside the
ROOT matrix function; the next diagnostic will identify its exact check. The
ROOT NoC and worker119 snapshots are in `evidence/work.stderr`. No valid
output, prompt prefill, or decode rate was produced.

## Recovery

Explicit Work STOP passed the safe-idle proof, the client unloaded cleanly,
and stable041 hot-reloaded at generation8:

```text
P100A_Q35 unload_clean=1 state=2 active=0 cold=0
P100A_RELOAD driver55 tag2026102100010012 generation8 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY
SAFE_FAULT_RECOVERY_VERIFIED
```

Both model files remain intact, no inference client or shared lock remains,
and the board did not reboot. Stable041 is idle.

## Evidence hashes

| File | SHA-256 |
| --- | --- |
| `work.stdout` | `8805430cb9e0d8880a0894e5ca252fda56008e19092286e9ab38573b37bc763a` |
| `work.stderr` | `66d1592660a516fb6cf5da54325f5434f93cf5a51a456d6f114e3205aa35b079` |
| `work.exit` | `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865` |
| `state` | `507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e` |
| `postflight.log` | `2550f0c5bbf2f03069ce5b43e8cb904c99fbbf082734ccd02c7b541bbc3b8a21` |
