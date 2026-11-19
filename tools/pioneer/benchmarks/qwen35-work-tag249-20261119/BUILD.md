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

## Hardware result

Tag249 passed preflight from stable041 generation4, `cold0`, `active_owner0`,
with the 27B and 4B files intact. Authentication took `68.086098` seconds,
prompt tokenization took `13.609946` seconds for 18 tokens, and model
upload/load took `222.687418` seconds for 3,450 spans (`2,740,498,432`
bytes`). ROOT again faulted before prefill with zero committed tokens while
waiting on worker119 (`stage=0x54000102 detail=0x77`). The failure-only
timeout receipt `0x58` was not present, so the bounded timeout checkpoint did
not capture this early fault. No valid output or decode rate was produced.

Tag249 passed safe-idle proof and STOP reproof, unloaded Work cleanly, and
hot-reloaded stable041 at generation6 (`cold0 active_owner0 guard1`) without
rebooting; the shared lock is absent. Both models remain intact. Tag250 will
write the fixed transfer identity before issuing its doorbell, so an early
fault still leaves the worker, direction, and remote window in the checkpoint.

| Evidence file | SHA-256 |
| --- | --- |
| `evidence/postflight.txt` | `0f17a2825bcc210918ed0538547be134d937f313ebf7a6d2e0f8e3278e03a074` |
| `evidence/processes.txt` | `f340b8bac7c1ca747927b360711af1e7e62b979818486355e08bbf19b495b3ca` |
| `evidence/state` | `507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e` |
| `evidence/status.txt` | `0e82a219a9374cca21e57a44fdc2cf84da766ad26c12c40f5be62fcb6d30b9e0` |
| `evidence/sysinfo.txt` | `2d691c8e636e11d23380b005c9917a36e61bb356ac2de948e8af8cb6dbefb9c5` |
| `evidence/work.exit` | `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865` |
| `evidence/work.stderr` | `9032ffdba4fd68c57f4dbdbe5ffe322c858e7fa4250b922e297011a0002c3dbb` |
| `evidence/work.stdout` | `002bf417f4e7b2a5207aa8dbf5a1a317cda0f188a826614faa8c8bd1aa57b676` |
