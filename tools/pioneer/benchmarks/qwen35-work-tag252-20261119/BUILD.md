# Qwen3.5-4B Work tag52: explicit error cleanup

Tag `2026111800010052` removes the failed tag51 close-time I/O path. Client
close retains the owner without touching the card; recovery stays in explicit
Work STOP and BANK END. The client now tracks whether a bank session, span
upload, or Work PREPARE may have taken effect and attempts the matching
explicit cleanup before closing after a load or inference error.

Offline warnings-as-errors cross-build:

```sh
python3 -B tools/pioneer/build_qwen35_work.py --transfer-diagnostic \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-252-final
```

The existing `PioneerQwen35WorkOwner` regression executable also passed. Tag52
has not been installed on hardware. Tag250 already verified that the same
Work STOP/BANK END recovery reproof cleans up the worker119 ROOT fault and
hot-reloads stable041 without a board reboot. Tag251's attempt to move that
card I/O into `close()` failed and remains documented as a historical failure.

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55 | `38ffe867b2f13df2874b18ee4f86c91a4410117d5be8a6dfbd970c0570100dd4` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `4f104ead3ca0e16159937a95313767c27ac9e2628e2c2cdda6fad0102d8d7c57` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `25ee4dafbc7979e581951adb7e457aa4bf1a4130d1d850f238534a6bc32b3864` |
| Work client | `19f1a2745cc6d208866c41b47f847f01dcd40f7a2001af6a20a1c75a82be29fc` |

This build is diagnostic only. The pending worker119 doorbell transfer still
prevents model prefill, so it has produced no valid output or decode rate.
