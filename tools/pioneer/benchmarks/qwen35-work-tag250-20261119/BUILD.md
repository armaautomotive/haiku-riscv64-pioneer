# Qwen3.5-4B Work tag250: preserve the pending transfer identity

Tag `2026111800010050` extends tag249's diagnosis-only build. Tag249 faulted
with the checkpoint still at worker119's generic post-doorbell marker, before
its bounded timeout diagnostic wrote `0x58`. Tag250 records the current
transaction before each doorbell and marks it pending after posting. The
checkpoint encodes worker, read/write direction, and remote window; the detail
carries byte length until the timeout path replaces it with the final counter
delta. This adds no NoC packet, retry, peer reset, or CPU inference path.

The build completed with warnings as errors:

```sh
python3 -B tools/pioneer/build_qwen35_work.py --transfer-diagnostic \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-qwen35-work-offline-250diag
```

The planned Haiku stage is `/boot/home/pioneer-qwen35-work-250.KP8W`; the
stable041 baseline for the hardware trial is generation6. This instrumented
build is for fault diagnosis only and is not a performance candidate.

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55 | `ac72ff20f4dc42efa60b008426756c45f5107e0c201a34c4904a5773bc30f666` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Work client | `8b51d7cd0810c55fca082aa970621a74da72319b2ceba12c8a2dae45cd4a6acb` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `4f104ead3ca0e16159937a95313767c27ac9e2628e2c2cdda6fad0102d8d7c57` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `25ee4dafbc7979e581951adb7e457aa4bf1a4130d1d850f238534a6bc32b3864` |
| Trial wrapper | `92f0c51fec8e381daf0090ab7bc0fb0bff67967d73dd8fc94b5a085db7b1f93d` |
| Power-cycle recovery helper | `8cdd84dd776b40edaf47ce353e01c3a0396b82bf5f9b5933b3a28becb25d4faa` |

## Hardware status

Tag250 has not yet been run. Tag249 restored stable041 at generation6
(`cold0 active_owner0`) without reboot; its recovery evidence is preserved in
the preceding benchmark directory. Both models remain intact.
