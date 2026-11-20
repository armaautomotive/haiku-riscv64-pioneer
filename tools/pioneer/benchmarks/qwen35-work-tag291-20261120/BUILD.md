# Qwen3.5-4B Work Tag91 build

Tag91 removes the failed Tag90 combined descriptor/doorbell protocol. ROOT
again writes and verifies the complete peer descriptor at `0x74000`, then
writes the separate sequence doorbell at `0x74100`. The peer polls that
doorbell and validates the descriptor sequence before starting the Tensix
projection. ROOT and peer text images, the Work client, and all launch/runtime
code match the previously hardware-tested Tag89 artifacts byte-for-byte; only
the fixed driver build tag is new. This restores Tag89's 0.123276 tok/s
baseline, not a performance improvement.

The Tag90 fault and required cold recovery are documented in
[`../qwen35-work-tag290-20261120/RUN.md`](../qwen35-work-tag290-20261120/RUN.md).
Tag91 itself has not been installed or run on the P100A.

## Offline verification

The policy test passed, and the full cross-build passed its fixed image,
wait-loop disassembly, driver-tag, import, and client gates. The generated
peer disassembly polls `0x74100`; no image references Tag90's unsafe `0x7413c`
sequence slot.

```sh
c++ -std=c++11 -Wall -Wextra -Werror \
  tools/pioneer/tests/pioneer_qwen35_peer_audit_test.cpp \
  -o /private/tmp/pioneer-qwen35-peer-audit-tag291
/private/tmp/pioneer-qwen35-peer-audit-tag291
python3 -B tools/pioneer/build_qwen35_work.py \
  --device-opt-level 3 --peer-opt-level s \
  --batch-fetch --batch-prefetch --peer-exec-profile \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-qwen35-work-tag291-build
```

The cross-build record is `evidence/build.log`.

## Artifact identities

| Artifact | SHA-256 | Bytes |
| --- | --- | ---: |
| ROOT boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` | 4 |
| ROOT text | `b2a12e1992f92e0804432e9807ea9c764742e6c1926c78476fa25ce7b6640499` | 87,188 |
| Peer boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` | 4 |
| Peer text | `d8f143a312ce44877526a88ae11256a8d493ce218be09ddf034d9f0c3c034a82` | 8,164 |
| Candidate driver55 Tag91 | `e9fe17ab361d1e1aea4240c57b7de987c16a660cd4b5d558f9e3858a71d8228b` | 283,856 |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` | 7,176 |
| Default43 control | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` | 934,080 |
| Work client | `d8da309cdf9675c3e4e3610d2b6e6b99e0d7a519b091af26dadcf49d227d1c5d` | 304,848 |

The offline policy output was:

```text
PASS Qwen35 Work physical PC/reset audit policy: every close and all checked dispatches are audited
```
