# Qwen3.5-4B Work tag244: bounded peer nonce reconciliation

Tag `2026111800010044` adds one narrowly bounded peer nonce case to tag243's
repeatable STOP proof. A peer may report the ledger nonce, one less (it did not
consume the failed dispatch), or one more (it completed before ROOT's ledger
commit). It must still pass the same per-core source/code checks, quiet NoC
proof, stable panel reads, and exact waiting-PC/status check. Distances of two,
regressions below the host's prior nonce, overflow, mirror disagreement, and
unknown PCs remain rejected. The ROOT is armed/held only after all 119 peers
pass. Proof reasons now distinguish panel mirror, nonce relation, and peer
status failures.

## Offline build and checks

Built with the local Haiku RISC-V64 toolchain:

```sh
python3 -B tools/pioneer/build_qwen35_work.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 /private/tmp/pioneer-qwen35-work-offline-244
```

The driver, reload guard, default control driver, ROOT/peer images, and Work
client compiled and linked. The Work owner regression passed with warnings as
errors, including exact, one-behind, one-ahead, two-behind, below-prior,
root-boundary, and range checks. `git diff --check` passed.

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55 | `b2eae244dd72054e69e84d2ff83cb57b1eb62d2e15a148d0e4d095765aadf712` |
| Work client | `8b51d7cd0810c55fca082aa970621a74da72319b2ceba12c8a2dae45cd4a6acb` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `7cb5bbc499639721a678f49225cc3347ed4e0c09f020fd84f26e95c289a33fd3` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `feef2e513367a83f93c11142037c59e2a6af5aed1d7921d9cf031daa4fe63047` |
| Trial wrapper | `08b16047a24719f6392d33565210e882efee91ee7d14f71842ee51a789c8e8fd` |
| Power-cycle recovery helper | `a6aa42fc25c5797e8833738b906cd0a53f980905167d9079814154a90f2c8d3d` |

## Tag243 hardware result

The model authenticated in 100.099082 seconds, prompt tokenization took
14.210897 seconds, and upload/load took 229.292775 seconds for 3,450 spans
(2,740,498,432 bytes). ROOT again faulted at `pc=0x40064` before prefill, with
zero commits. The first proof returned reason 15/detail `0x1` (peer 1's status
or nonce consistency); STOP repeated that proof and returned the same reason.
The driver remained tag243 generation3 `cold1 active_owner1`; no decode rate or
valid output was produced. The client exited 1. Logs and state are preserved
in `evidence/`.

This tag allowed only ledger-equal or one-ahead peer nonces. Tag244 adds the
single lagging-peer case, while still requiring peer 1 to be parked and every
NoC queue quiet. The tag243 retained owner requires one authorized restart
before this candidate can be loaded. Its power-cycle recovery helper will
restore stable041 after the pinned SD-free return to Haiku.

The planned stage is `/boot/home/pioneer-qwen35-work-244.T6nB4`. Trial
preflight will again verify stable041 idle, the prior diagnostic exited, the
27B file intact, and all staged hashes after an explicit `sync`.

## Tag244 hardware result

The guarded run passed preflight, installed tag244 by hot reload, authenticated
in 98.663745 seconds, tokenized in 13.647911 seconds, and loaded all 3,450
spans (2,740,498,432 bytes) in 226.037084 seconds. It faulted before prefill
with zero commits at `pc=0x40060`, ROOT dispatcher stage `0x54000102`,
detail `0x77` (the last peer transfer). Recovery reached peer 1, whose
requested sequence was 1 while its completed sequence was still 0. Tag244's
peer nonce helper allowed a one-count gap in principle but the proof still
required the two panel counters to match, so STOP rejected the proof with
reason 15/detail `0x01000001`. No output or decode rate was produced. The
client exited; Haiku remained reachable, and the driver retained
`cold1 active_owner1`. The wrapper stopped before attempting a driver swap.

Tag245 will wait, without issuing writes, for every outstanding peer request
to finish. It then requires stable status snapshots, exact parked PCs,
unchanged ROOT state, quiet NoC queues, and the existing all-core source and
code checks before recovery can touch the ROOT or close the fleet. It times
out into quarantine if any peer does not settle.

| Evidence file | SHA-256 |
| --- | --- |
| `evidence/state` | `fbd1473a9abb5073bbd33c4b496a571ea742253449bef0484ab678f9efd1c9c4` |
| `evidence/work.exit` | `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865` |
| `evidence/work.stdout` | `18b60766223e6bc77ed072d5e37dd608238d93a70341a40dc847e3a8580e4726` |
| `evidence/work.stderr` | `58e6b58c7e4d4dda717bef2c3d2e364459f98a098e28c215e5b48c31b1b88a3a` |
