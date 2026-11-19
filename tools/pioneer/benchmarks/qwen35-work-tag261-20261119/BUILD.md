# Qwen3.5-4B Work tag61: preserve matrix validation receipts

Tag `2026111800010061` adds diagnostics-only checkpoints inside the ROOT
matrix image for job shape, each Q5_K scale, and each activation word. The
cross-build now passes the diagnostic selector to that image as well as the
dispatcher. The warnings-as-errors build passed:

```sh
python3 -B tools/pioneer/build_qwen35_work.py --transfer-diagnostic \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-261a
```

The matrix receipts use `0x5f000002` for a shape bitmask,
`0x5f200000|block` for a rejected scale, `0x5f100000|word` for a rejected
activation pair, and `0x5f000010/11/12` for shape, scale, and activation
validation progress. The tag61 run exposed an instrumentation issue: the
dispatcher overwrote the inner ROOT checkpoint when reporting the zero return.
Tag62 should copy the inner stage/detail to the reserved ROOT progress context
before updating the outer marker.

| Artifact | SHA-256 |
| --- | --- |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `5a505a7a6d2f7a6aba46645bc98cab217edfd3d25ad72460052e7d54a5dda7d9` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `ee3b913d14551b642d64db709247c6f33b4b71dc35b557be8ef15e413b889a03` |
| Candidate driver55 | `003abfe63d04f4aa1e3c4784b48248c7fed8f15b89a21020cb7949cc6cf9fc77` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Default43 control | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` |
| Work client | `6184f2aa1ff825172efd6ef89c1751ad07167d71a6fc0d66a990bfab16fac654` |

The guarded run and recovery proof are recorded in [`RUN.md`](RUN.md) and
[`evidence`](evidence).
