# Qwen3.5-4B Work Tag98 build

Tag98 keeps Tag97's scaled, bounded peer wait and adds failure-only receipts
for the final ROOT software transfer counters. If the exact `reads`, `writes`,
`flits`, or `writeFlits` proof fails, the ROOT records the counter index and
the expected/actual values in the existing Work progress record before
parking. The diagnostic performs no extra NoC transfer and leaves all exact
counter checks enabled.

The full default Haiku cross-build passed its image, import, disassembly,
driver-tag, and client gates.

```sh
python3 -B tools/pioneer/build_qwen35_work.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-qwen35-work-tag298-candidate
```

## Artifact identities

| Artifact | SHA-256 | Bytes |
| --- | --- | ---: |
| ROOT boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` | 4 |
| ROOT text | `91d48b684549ce8b57c1f22ccf1e2272bc4e508be86c218e9daffd5210fac6f4` | 54,676 |
| Peer boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` | 4 |
| Peer text | `5936aaf8d19749d2b625b6106018e4b077bfcf44cd0454363d8854adcd715390` | 7,988 |
| Candidate driver55 Tag98 | `7c755f518d0c215de4161fa2e736b3cf4eabb0423da5dca2741d7002a47ca99f` | 252,496 |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` | 7,176 |
| Default43 control | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` | 934,080 |
| Work client | `79792acf48caa979628553656390b5d70b67628d0a8cb9f9dd3db74d1f8e3473` | 309,352 |

The guarded hardware trial is pending. Begin from stable driver55 generation12
after a fresh idle preflight confirms the 27B setup and model identities.
