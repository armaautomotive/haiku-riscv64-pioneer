# Qwen3.5-4B Work tag59 hardware run

Date: 2026-11-19. The guarded tag59 client ran on Haiku hrev99999, loading
from stable041 generation4 at candidate generation5. The wrapper restored
stable041 at generation6 without reboot.

## Load and fault

Authentication took `67.653402` s. Vocabulary/tokenization took `13.467099`
s for the 18-token prompt. All 3,450 resident spans
(`2,740,498,432` bytes) loaded in `225.409831` s.

The first Work run faulted before prompt prefill with zero committed tokens.
The new phase marker shows that all peer doorbell transfers returned and the
ROOT entered its projection wrapper with ten tiles assigned:

```text
state=4 commits=0 prefilled=0
dispatcher_stage=0x5d000002 dispatcher_detail=0xa
```

The marker is written immediately before `Q35RootMatrix()`. The remaining
failure is inside that call or one of its initial checks; the marker does not
yet identify which check. The ROOT NoC and worker119 snapshots are preserved
in `evidence/work.stderr`. No valid output, prompt prefill, or decode rate was
produced.

## Recovery

Explicit Work STOP completed, the client unloaded cleanly, and stable041 was
hot-reloaded at generation6:

```text
P100A_Q35 unload_clean=1 state=2 active=0 cold=0
P100A_RELOAD driver55 tag2026102100010012 generation6 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY
SAFE_FAULT_RECOVERY_VERIFIED
```

Both model files remain intact, no inference client or exclusive lock remains,
and no board reboot was used for this recovery. The current board is idle on
stable041.

## Evidence hashes

| File | SHA-256 |
| --- | --- |
| `work.stdout` | `f2756c12618d93e29b342ed69aa40389cb1694e24dea6464242bd1abc48f9764` |
| `work.stderr` | `8d94529edb0ed213a662508167a96f5c0595629d088fe979195686fa6fae2976` |
| `work.exit` | `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865` |
| `state` | `507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e` |
| `postflight.log` | `2b28e050960612662fa518c7d4276a9fea019f09f889d5c8860831d15bc8c0a1` |
