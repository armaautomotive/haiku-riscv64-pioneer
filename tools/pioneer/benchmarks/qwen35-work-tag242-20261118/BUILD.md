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

## Tag242 hardware result

The 4B model authenticated in 101.441872 seconds, tokenized in 13.291608
seconds, and loaded 2,740,498,432 bytes in 223.378064 seconds. Before prefill,
ROOT reported `RootFault` at `pc=0x40064`, with no commits and zero prompt
processing. The recovery proof returned false, STOP reported `Device not
ready`, and the wrapper stopped before restoring the stable driver. No decode
tokens/sec were produced. Read-only status showed tag242 generation 3,
`cold1 active_owner1`; no client or 27B diagnostic process remained. Evidence
is preserved in `evidence/`.

Evidence SHA-256: `state` `fbd1473a9abb5073bbd33c4b496a571ea742253449bef0484ab678f9efd1c9c4`,
`work.exit` `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865`,
`work.stdout` `51bceea960f63e16c4059ecc82a318abc76cdc3f73d9ad3e0149d4ccfda9a749`,
`work.stderr` `c89f5fd64512ea6d428f38575ca8b9d6bb78197fad05e8b150c388510212df96`.

Tag242's implementation could not retry the proof or report its failed check.
Tag243 adds both behaviors. Its package targets
`/boot/home/pioneer-qwen35-work-243.S9mQ2`; the next trial requires one
authorized board restart to clear tag242's retained owner before tag243 can
be loaded.
