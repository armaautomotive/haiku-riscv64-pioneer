# Qwen3.5-4B Work tag245: bounded peer settle before recovery

Tag `2026111800010045` changes the Work fault proof so it waits up to 30
seconds, without issuing device writes, for every peer request already in
flight to finish. It accepts only the last committed peer nonce or its next
nonce, rejects gaps and regressions, requires stable status snapshots and
the exact peer image, then requires all 119 peers at their known wait PCs,
quiet NoC queues, unchanged ROOT state, and the existing source/code checks.
Only then may STOP restore the ROOT and close the persistent peer fleet.
If a peer reports an error, an unknown PC, an invalid sequence, or does not
settle by the deadline, the device remains quarantined.

Work mode still uses the Tensix matrix engines across all 120 cores. This
recovery path performs no CPU inference and never turns an unverified peer
state into a successful model output.

## Offline build and checks

Built with the local Haiku RISC-V64 toolchain:

```sh
python3 -B tools/pioneer/build_qwen35_work.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 /private/tmp/pioneer-qwen35-work-offline-245
```

The tag245 Work owner regression passed with `-Wall -Wextra -Werror`; driver,
reload guard, ROOT/peer images, and Work client compiled and linked. Build
script syntax and `git diff --check` passed.

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55 | `6b812cfc93b8a09c2587bf216f857503ff21cc01a801090b8057755e96eada82` |
| Work client | `8b51d7cd0810c55fca082aa970621a74da72319b2ceba12c8a2dae45cd4a6acb` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `7cb5bbc499639721a678f49225cc3347ed4e0c09f020fd84f26e95c289a33fd3` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `feef2e513367a83f93c11142037c59e2a6af5aed1d7921d9cf031daa4fe63047` |
| Trial wrapper | `8d8c702e4bbc178583029a40d650c7d9471d4689721f32b7cf33c84bceac00d8` |
| Power-cycle recovery helper | `62117b551834d9b0a0de620993a9ef12c72c7d7582061ffea12d353fb9edc391` |

## Prior tag244 hardware result

Tag244 passed the guarded preflight and hot-loaded. Authentication took
98.663745 seconds, prompt tokenization 13.647911 seconds, and model upload/load
226.037084 seconds for 3,450 spans (2,740,498,432 bytes). ROOT then faulted
before prefill with zero commits at `pc=0x40060`, stage `0x54000102`, detail
`0x77`. At proof time peer 1 had requested nonce 1 and completion nonce 0.
Tag244 rejected that split with reason 15/detail `0x01000001`, leaving the
responsive Haiku host at candidate generation3 `cold1 active_owner1`. No valid
output or decode rate was produced. Exact logs and hashes are archived in
`../qwen35-work-tag244-20261118/evidence/`.

The tag244-specific helper at
`/boot/home/pioneer-qwen35-work-244.T6nB4/pioneer_qwen35_work_recover_tag244_after_power_cycle.sh`
is staged and hash-checked. It will restore stable041 after the authorized
cycle; tag245 will be staged separately at
`/boot/home/pioneer-qwen35-work-245.D7mQ5`.
