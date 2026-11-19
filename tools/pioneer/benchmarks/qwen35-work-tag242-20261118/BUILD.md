# Qwen3.5-4B Work tag242: safe terminal-fault recovery

Tag `2026111800010042` adds an in-place recovery path for a terminal ROOT Work
fault. The driver admits recovery only when the ROOT remains parked at the
known `WorkFaultPc`, the completion marker is `RootFault`, the dispatch ledger
is stable, and all 119 peers pass source, code, nonce, wait-PC, and quiet-NoC
checks. STOP repeats the proof, arms and holds only ROOT, reconciles peer
nonces, restores saved ROOT L1/checkpoint/command state, and then uses the
existing dispatch close path. A failed proof keeps the existing cold
quarantine; Work has no CPU inference fallback.

## Offline build and checks

Built with the local Haiku RISC-V64 toolchain:

```sh
python3 -B tools/pioneer/build_qwen35_work.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 /private/tmp/pioneer-qwen35-work-offline-242b
```

The driver, reload guard, default control driver, ROOT/peer images, and Work
client compiled and linked. The owner regression passed with `-Wall -Wextra
-Werror`; both guarded shell scripts pass `bash -n`; `git diff --check` is clean.

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55 | `cd189c9d3a2fee3bae91fe2ebca7a51d3fe857e0b566763c5ff64706486e1c34` |
| Work client | `46a8b6490882a967b13fcac7c0196a5f84500a9309a4fd1a4732dbf31eea8e17` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `7cb5bbc499639721a678f49225cc3347ed4e0c09f020fd84f26e95c289a33fd3` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `feef2e513367a83f93c11142037c59e2a6af5aed1d7921d9cf031daa4fe63047` |
| Trial wrapper | `41ab1913a75d473f4ca421d485e39a0b1149d3855ca92cb05aee0e676f987011` |
| Power-cycle recovery helper | `02942d93f60bdf5ad42153a62c6e778aa66494405fbd9bbe3c99ab6dba2b6bb6` |

The tag241 hardware fault record showed `RootFault` at `pc=0x40060` before
prefill. The tag242 trial will check whether STOP can recover this same class
of parked fault without rebooting. Before the trial, a read-only Haiku check
confirmed the prior inference/27B diagnostic had exited, the stable driver was
idle at generation 2 (`cold0 active_owner0`), the exclusive lock was absent,
and both model files retained their pinned sizes.

The planned Haiku staging directory is
`/boot/home/pioneer-qwen35-work-242.K7rP7`. Trial results will be appended
here after the hardware run.
