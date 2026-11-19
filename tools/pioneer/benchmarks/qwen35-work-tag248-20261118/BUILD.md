# Qwen3.5-4B Work tag248: bounded Q8PF recovery proof

Tag `2026111800010048` extends rebootless Work fault recovery after tag247
still refused explicit Q8PF peers. The Work fault handler saves RA and fences
before publishing its marker, so its terminal loop begins at `0x720a4`. Tag248
adds a second sealed local branch at `0x720a8`; both branches loop to the first
and perform no NoC access. The recovery proof accepts PC snapshots only at
those exact addresses and still requires two stable snapshots, exact code and
source, nonce/status receipts, terminal ROOT, and quiet NoC queues.

The Work status validator now returns field-specific subreasons. Refusal
reasons `71..83` identify invalid nonce ledger, marker/state, unexpected
status fields, handler PC, return PC, and panel-count receipt. The reason
detail carries the worker and rejected value, instead of hiding all cases
behind reason17/Q8PF. The powered-up trial remains no-fallback and accepts a
model result only after its normal output/core-wave validations.

## Offline build

Built with the local Haiku RISC-V64 toolchain:

```sh
python3 -B tools/pioneer/build_qwen35_work.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 /private/tmp/pioneer-qwen35-work-offline-248e
```

The build completed with warnings as errors. The peer image builder verifies
the two exact branch instructions at `0x720a4` and `0x720a8`. The Work owner
regression compiled with `-Wall -Wextra -Werror` and passed its exact park,
handler, peer marker, return-PC, panel-count, and specific-refusal-code checks.
Shell syntax and `git diff --check` passed.

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55 | `9303778b75b15e9debb0a68ed15f5cad8b67b6a3a65743a8b63200a12a3f2b1d` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Work client | `8b51d7cd0810c55fca082aa970621a74da72319b2ceba12c8a2dae45cd4a6acb` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `7cb5bbc499639721a678f49225cc3347ed4e0c09f020fd84f26e95c289a33fd3` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `25ee4dafbc7979e581951adb7e457aa4bf1a4130d1d850f238534a6bc32b3864` |
| Trial wrapper | `5e0bc16340a5092ed56a590dfa1539e9a47f95ff034052efe74b2a5a5ff6a292` |
| Power-cycle recovery helper | `0c83f2d52fb89be86ad58a0b52c6a94b98d236b800d0a641d38e783775a5968d` |

## Hardware status

Tag248 has not yet been run. Tag247 authenticated in `100.777613` seconds,
tokenized the prompt in `14.373964` seconds, and loaded/uploaded the model in
`225.068757` seconds before the same prefill failure. Its first STOP proof
returned reason17/detail `0x1503851`, and the candidate remained quarantined at
generation3 `cold1 active_owner1`; no output or decode rate was produced.
Tag247 logs are preserved in the preceding benchmark directory. Stable041
and the 27B model remain preserved.
