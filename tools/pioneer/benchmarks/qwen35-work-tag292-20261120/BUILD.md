# Qwen3.5-4B Work Tag92: proof-gated STOP recovery

Tag92 adds a Work-only recovery path for a failed generic dispatcher STOP at
close phase 30. It accepts cleanup only when two complete physical audits
prove that ROOT is at the dispatcher fault park with a stable fault marker,
all 119 peers are either in their prior wait loop or at the expected STOP
park, every selected core has the exact dispatched code and source state, and
each core's local NoC queues are idle. Only after the complete proof does the
driver hold all 120 cores and restore the saved device state. Any ambiguous
PC, status, source, code, or queue state still quarantines before reset or
restore. The path is enabled only by Qwen35 Work STOP.

This does not recover failures at other close phases or arbitrary retained
owners. It does not change the model image, the peer protocol, matrix math,
prompt execution, or decode scheduling. No Tag92 card run has been made; the
recovery behavior is verified offline only. Tag91 remains the latest physical
run and the performance baseline.

## Verification

- Qwen35 Work ROOT and peer images rebuilt at ROOT `-O3` and peer `-Os`.
- The WorkOwner, dispatcher close, and peer audit policy regressions passed.
- The dispatcher close regression passed both payload configurations under
  ASan and UBSan, including a faulted ROOT with mixed waiting/stopped peers,
  an invalid peer status, and a nonterminal ROOT. Negative cases verified
  quarantine before any core hold or restoration.
- The full Haiku cross-build passed. The ROOT/peer image and client hashes
  match Tag91; the candidate driver has SHA-256
  `d468a96e34fb69d74bbee82191cd1329553faee1c09b9844fa7af8e96f928755`.

The offline dispatcher test does not establish P100A behavior. A guarded
preflight and a deliberate faulted-STOP card trial are still required before
calling this recovery path physically verified.

## Artifact identities

| Artifact | SHA-256 |
| --- | --- |
| ROOT boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT text | `b2a12e1992f92e0804432e9807ea9c764742e6c1926c78476fa25ce7b6640499` |
| Peer boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer text | `d8f143a312ce44877526a88ae11256a8d493ce218be09ddf034d9f0c3c034a82` |
| Candidate driver55 Tag92 | `d468a96e34fb69d74bbee82191cd1329553faee1c09b9844fa7af8e96f928755` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Default43 control | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` |
| Work client | `d8da309cdf9675c3e4e3610d2b6e6b99e0d7a519b091af26dadcf49d227d1c5d` |

The last physical result remains in
[`Tag91`](../qwen35-work-tag291-20261120/RUN.md).
