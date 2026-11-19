# Qwen3.5-4B Work tag59: locate the ROOT projection fault

Tag `2026111800010059` changes only diagnostic checkpoints in the Qwen35 ROOT
image. Transfer admission failures now use stage `0x5c...`, a bounded NoC
transfer timeout uses `0x5b...`, and stages `0x5d000001` through `0x5d000005`
mark peer doorbell completion, ROOT projection, and peer completion. Normal
transfer and Work ownership behavior is unchanged.

The warnings-as-errors cross-build passed:

```sh
python3 -B tools/pioneer/build_qwen35_work.py --transfer-diagnostic \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-259b
```

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55 | `19a086d84c756d2b78a7f0fa61c6abfe278bbf4410bee967563162f0d46eb3c9` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `6e0f0e5246ddabca408034be2ea92afef1180413765965cb3a051ed0e83b9451` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `ee3b913d14551b642d64db709247c6f33b4b71dc35b557be8ef15e413b889a03` |
| Work client | `6184f2aa1ff825172efd6ef89c1751ad07167d71a6fc0d66a990bfab16fac654` |

The guarded hardware run and its recovery result are in [`RUN.md`](RUN.md)
and [`evidence`](evidence). The 4B model still faults before prompt prefill;
no valid output or decode rate was produced.
