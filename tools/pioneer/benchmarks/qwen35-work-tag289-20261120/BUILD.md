# Qwen3.5-4B Work Tag89 build

Tag89 isolates one Tenstorrent-style program-boundary change on the Tag88
baseline. The build keeps the same ROOT `-O3`, peer `-Os`, peer profiling, and
8 KiB double-buffered read schedule. The peer image is byte-for-byte identical
to Tag88.

For each active Work projection, the ROOT still verifies peer status,
sequence, source, counters, returned output, and finite values. It no longer
reads each peer's physical PCs and reset register after every projection.
Those reads are retained when the resident session is admitted and when it
closes with STOP. Checked dispatches keep their per-dispatch physical audit.
The Qwen3.8/27B path is compile-time unchanged.

This is a guarded hardware candidate, not yet a performance result. The
offline policy check passed, and the cross-build passed its fixed image,
disassembly, and driver gates.

Build command:

```sh
python3 -B tools/pioneer/build_qwen35_work.py \
  --device-opt-level 3 --peer-opt-level s \
  --batch-fetch --batch-prefetch --peer-exec-profile \
  HAIKU_BUILD /private/tmp/pioneer-qwen35-work-tag289-final
```

The peer image comparison against Tag88 passed with `cmp`.

| Artifact | SHA-256 | Size |
| --- | --- | ---: |
| ROOT boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` | 4 B |
| ROOT text | `b2a12e1992f92e0804432e9807ea9c764742e6c1926c78476fa25ce7b6640499` | 87,188 B |
| Peer boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` | 4 B |
| Peer text | `d8f143a312ce44877526a88ae11256a8d493ce218be09ddf034d9f0c3c034a82` | 8,164 B |
| Candidate driver55 | `d48cb53df14ff97e3ed455c5904eb4d252e085b45678a915930f23ed1d594e8e` | 283,856 B |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` | 7,176 B |
| Default43 control | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` | 934,080 B |
| Work client | `d8da309cdf9675c3e4e3610d2b6e6b99e0d7a519b091af26dadcf49d227d1c5d` | 304,848 B |

Offline policy check:

```text
PASS Qwen35 Work physical PC/reset audit policy: every close and all checked dispatches are audited
```
