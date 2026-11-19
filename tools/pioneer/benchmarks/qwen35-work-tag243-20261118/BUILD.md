# Qwen3.5-4B Work tag243: retryable fault proof

Tag `2026111800010043` retains the all-120-core safety checks from tag242 and
adds a bounded recovery state transition. When ROOT is parked at the explicit
`RootFault` marker and known terminal PC but the first fleet proof is
inconclusive, polling reports `WorkRecoveryPending` without resetting hardware
or cold-latching. The user's normal STOP re-runs the entire fleet proof; only a
passing second proof permits ROOT to transition through arm/hold, reconciles
all peer nonces, restores the saved ROOT L1/checkpoint/command state, and closes
the dispatcher. STOP reports a numeric proof reason/detail when it refuses.
Unknown PCs and all unrelated faults remain quarantined. Work mode still has no
CPU inference fallback.

## Offline build and checks

Built with the local Haiku RISC-V64 toolchain:

```sh
python3 -B tools/pioneer/build_qwen35_work.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 /private/tmp/pioneer-qwen35-work-offline-243
```

The driver, reload guard, default control driver, ROOT/peer images, and Work
client compiled and linked. The Work owner regression passed with warnings as
errors; guarded shell syntax and `git diff --check` passed.

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55 | `22b6ba06e2f0a42b8e86b52a54ff728c2602bfbe8a19c4680a9d33d8f8353c7d` |
| Work client | `8b51d7cd0810c55fca082aa970621a74da72319b2ceba12c8a2dae45cd4a6acb` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `7cb5bbc499639721a678f49225cc3347ed4e0c09f020fd84f26e95c289a33fd3` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `feef2e513367a83f93c11142037c59e2a6af5aed1d7921d9cf031daa4fe63047` |
| Trial wrapper | `1623a0cc9107e0ea1df211871a2b0cc128c8d6557ba295baf0f942dd3dc1b561` |
| Power-cycle recovery helper | `eca6e5186d55dc32f5f6f9880aed60af8abd8094bf48f4171be522eec77d5597` |

## Tag242 hardware result

The 4B model authenticated in 101.441872 seconds, tokenized in 13.291608
seconds, and loaded 2,740,498,432 bytes in 223.378064 seconds. Before prefill,
ROOT reported `RootFault` at `pc=0x40064`, with no commits and zero prompt
processing. The first recovery implementation immediately cold-latched after
its proof returned false; STOP reported `Device not ready`. The trial wrapper
therefore stopped before restoring the stable driver. No decode tokens/sec
were produced. Read-only status showed tag242 generation 3, `cold1
active_owner1`; no client or 27B diagnostic process remained. The log set is
preserved in `evidence/`.

This retained state requires one project-authorized board restart to clear
before tag243 can be loaded. The user-provided Pioneer policy authorizes this
restart; no disk, SD, hardware-connection, or mains changes are involved.

The tag243 trial package targets
`/boot/home/pioneer-qwen35-work-243.S9mQ2`. It will be re-preflighted against
the stable driver before installation.
