# Qwen3.5-4B Work tag60: isolate the ROOT matrix return

Tag `2026111800010060` adds failure-only checkpoints around the ROOT matrix
phase. The guarded run can distinguish an idle check, geometry validation,
command readback, matrix return count, and output publication. The normal
non-diagnostic image path is unchanged.

The warnings-as-errors cross-build passed:

```sh
python3 -B tools/pioneer/build_qwen35_work.py --transfer-diagnostic \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-260a
```

The `0x5e000001` marker starts ROOT admission. `0x5e000010/11` identify the
first busy command/protocol counter, `0x5e000002` is invalid geometry,
`0x5e000004` is a command readback mismatch, `0x5e000003` precedes the matrix
routine, `0x5e000006` records its returned band count, and `0x5e000007` records
a wrong count. `0x5e000008/09` cover post-matrix command verification and
result publication.

| Artifact | SHA-256 |
| --- | --- |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `25c463fa728ef5c67248bab253403336aaa5c5c0cea96653588c68abd33b020a` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `ee3b913d14551b642d64db709247c6f33b4b71dc35b557be8ef15e413b889a03` |
| Candidate driver55 | `06fd7bbc1e2859529fc1e02da51994fd50cfc21303b336e9cc45530bbbf86f5b` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Default43 control | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` |
| Work client | `6184f2aa1ff825172efd6ef89c1751ad07167d71a6fc0d66a990bfab16fac654` |

The hardware run and stable-driver recovery are recorded in [`RUN.md`](RUN.md)
and [`evidence`](evidence).
