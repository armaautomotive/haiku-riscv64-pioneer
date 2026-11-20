# Qwen3.5-4B Work Tag90 build

Tag90 tests a smaller per-projection dispatch protocol on the Tag89 Qwen35
Work path. ROOT now writes each 64-byte per-peer job to the already-zeroed
`0x74100` slot. The sequence is duplicated in trailing word15 and serves as
the peer's wake marker; peer status begins at `0x74140`. Each aligned job
transfer therefore carries both runtime arguments and the launch signal. The
peer validates the previous/current sequence and both descriptor copies
before reading model weights or starting Tensix work.

This removes the separate 4-byte doorbell write from Work projections. At
STOP, where the prior path also echoed the descriptor, the combined packet
removes that extra read too. Work result proofs, source seals, per-peer counter
checks, finite-output validation, all-120-core admission, and the full STOP
audit stay in place. The change is gated to the Qwen35 Work ROOT/peer images.
The generic dispatcher and 27B image do not define the gate.

This is an incremental port of Tenstorrent's persistent-program/runtime-argument
pattern. It does not yet implement the full reader/compute/writer graph with
sharded intermediate activations and circular-buffer overlap described in
[`QWEN35_TTMETALIUM_REDESIGN.md`](../../QWEN35_TTMETALIUM_REDESIGN.md).

## Offline verification

The peer policy test checks the 64-byte slot and sequence marker layout,
sequence transitions and audit policy. The peer image build also checks its
actual wait-loop disassembly: address `0x7413c` is loaded before the polling
loop. The full Tag90 cross-build passed its fixed-image, import, driver-tag,
and client gates.

```sh
c++ -std=c++11 -Wall -Wextra -Werror \
  tools/pioneer/tests/pioneer_qwen35_peer_audit_test.cpp \
  -o /private/tmp/pioneer-qwen35-peer-audit-tag290
/private/tmp/pioneer-qwen35-peer-audit-tag290
python3 -B tools/pioneer/build_qwen35_work.py \
  --device-opt-level 3 --peer-opt-level s \
  --batch-fetch --batch-prefetch --peer-exec-profile \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-qwen35-work-tag290-build
```

## Build artifact identities

| Artifact | SHA-256 | Bytes |
| --- | --- | ---: |
| ROOT boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` | 4 |
| ROOT text | `4b547ba6365ba47ffe991ac60953fa277011ff6198d17a4cf05877b40ceee524` | 86,964 |
| Peer boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` | 4 |
| Peer text | `68c8fc2b21e0452437b3d1bee30916ac4ad7e389286aaafdc245042587e6297e` | 8,176 |
| Candidate driver55 | `77fbcbf749af2c5c463715c21d7dc92d88593e444dbb091fc7cf9703ed81d6af` | 283,856 |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` | 7,176 |
| Default43 control | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` | 934,080 |
| Work client | `d8da309cdf9675c3e4e3610d2b6e6b99e0d7a519b091af26dadcf49d227d1c5d` | 304,848 |

No hardware run has been made with Tag90 yet.
