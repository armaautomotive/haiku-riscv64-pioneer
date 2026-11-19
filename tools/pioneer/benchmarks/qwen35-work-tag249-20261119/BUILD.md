# Qwen3.5-4B Work tag249: fixed transfer timeout evidence

Tag `2026111800010049` is a diagnosis-only follow-up to tag248. Tag248
recovered safely without reboot, but the first projection still faulted while
ROOT waited for worker119's transfer (`stage=0x54000102 detail=0x77`). This
build enables the existing failure-only `PIONEER_Q38_TRANSFER_DIAGNOSTIC`
path. It records the worker, direction, remote window, busy/counter mismatch,
and final counter delta after the bounded transfer wait expires. It adds no
NoC packet, retry, peer reset, or CPU inference path. Counter reads during the
wait make this build diagnostic-only; it is not a performance candidate.

## Offline build

Built with the local Haiku RISC-V64 toolchain:

```sh
python3 -B tools/pioneer/build_qwen35_work.py --transfer-diagnostic \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-qwen35-work-offline-249diag
```

The build completed with warnings as errors. It embeds tag248's two sealed
fault-park branches and field-specific STOP proof, plus the dispatcher
transfer-timeout receipt. The planned Haiku stage is
`/boot/home/pioneer-qwen35-work-249.H8T2`; the baseline for the trial is
stable041 generation4.

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55 | `e87d7f1476c7a95241d0567d8d3d7e1146da6d44691428c67e7ceafbc729cc3a` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Work client | `8b51d7cd0810c55fca082aa970621a74da72319b2ceba12c8a2dae45cd4a6acb` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `30cd34913207c26ec3b1de4591d6fb5ee5149a2b8e2ade944b37c6cb8b8d5999` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `25ee4dafbc7979e581951adb7e457aa4bf1a4130d1d850f238534a6bc32b3864` |
| Trial wrapper | `964dbf0f974b566a14b27c476f6c61170124bc33c1a72ec853a04943ecc3d068` |
| Power-cycle recovery helper | `86b4890cb18f81e8e9599412ef5e5ec0b0bba26c77f4393b63b43ba947aa182d` |

## Hardware status

Tag249 has not yet been run. Tag248 already verified rebootless recovery and
restored stable041 at generation4 (`cold0 active_owner0`), with the shared
lock absent and both models intact. The 4B inference fault remains before
prefill; no valid output or decode rate has been measured.
