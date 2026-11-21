# Qwen3.5 dataflow tag0113 hardware run

## Outcome

The guarded tag0113 trial loaded the full packed Qwen3.5-4B model and entered
prompt embedding. Core0's worker then failed while reading its embedding
weight shard (`failure=7`, `phase=5`). No token was produced. QUERY/RECOVER
returned pending after the all-core audit reported runtime instability at
stage6/core0. CLOSE also failed the core0 stop audit, so the wrapper retained
the tag0113 driver and exclusive recovery lock instead of replacing a
cold-latched, active-owner driver.

This was a failed generation and unload, not a power-reset-induced failure.
Stable55 was idle before the trial, the candidate reached generation2, and
the 4B and 27B model files retained their expected sizes. The candidate
reported `cold1 active_owner1 open_count1`; the wrapper retained its lock and
the exact stable55 backup, SHA-256
`041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403`.

The same-owner in-place recovery did not work. After preserving the trial
logs, an authorized restart returned the board to Fedora. The pinned,
SD-free kexec handoff returned it to Haiku, and the guarded recovery helper
restored stable55 at generation2 and cleared the lock. The 4B and 27B files,
64-CPU profile, and full RAM profile were verified afterward. This restored
the board but does not satisfy the requested recover-without-reboot behavior.
Tag0114 adds a detailed runtime-audit subcheck to identify the first
recovery-proof failure before changing that safety path.

No valid warm output exists, so the second warm run was not started. The
100 decode tokens/s gate remains unmet.

## Preflight and timings

The post-reset Haiku preflight found no 27B diagnostic or inference process.
Stable55 tag `2026102100010012`, generation1, reported
`quiesced0 cold0 active_owner0 open_count1`. The guarded wrapper verified the
reload guard, stable-driver and candidate hashes, the expected 4B file size
of 2740937888 bytes, and the preserved 27B file size of 18973870528 bytes.

| Phase | Result |
| --- | ---: |
| GGUF authentication | 98.782252 s |
| Vocabulary and prompt tokenization | 14.065504 s; IDs `760,6511,314,9338,369` |
| Model load and resident setup | 330.909273 s; 2740498432 resident bytes |
| Prompt processing | Failed after 0.612365 s; partial time only |
| Decode | Not reached; no decode time or tokens/s |

The generation request returned `result=31`, `stage=4`, `state=2`,
`recovery=1`, `output=0`, and `decode_us=0`. The worker receipt was core0,
state4/failed, embedding weight read failure7, phase5, pending sequence1.
The client attempted QUERY/RECOVER before CLOSE; RECOVER returned
`result=13` with recovery still pending. CLOSE returned `result=15`; its
stop audit failed at core0. The wrapper's `before_restore` proof therefore
failed and left `state=RECOVERY_REQUIRED_UNTIL_STABLE_RESTORED`.

## Guarded recovery evidence

The fresh Haiku boot reported tag0113 generation1 with `cold0 active_owner0`.
The recovery helper quiesced that candidate, atomically restored the saved
stable55 image, verified generation2, and removed the lock. Postflight found
stable55 idle, no inference process, unchanged 4B/27B sizes, 64 CPUs, and
137034936320 bytes of RAM.

## Evidence

Raw stdout, stderr, process status, driver metadata, and preflight captures
are in [`evidence/`](evidence/). Their SHA-256 values are:

| File | SHA-256 |
| --- | --- |
| `haiku-preflight.txt` | `9cd3f4abfb4a1391803400a1797e65cb8f0365b7b484b1ae54eb7fa7598eeace` |
| `haiku-prestage.txt` | `72e2cede1b6d19f66de3fc493a4d66dc7c65d58e81a9bdbbe940742d03b79ebc` |
| `haiku-wrapper-preflight.txt` | `54d0607eac475f87e361e34e96520e69379f28c574de73da01b918a26fc5c8b3` |
| `haiku-trial-run.txt` | `f0d8d523d3a1a7a8db45ffb3bdad8a9da21dbc6516ee27373f22e4b52d1394a8` |
| `haiku-trial-work.stdout` | `d4259db92512b40d21cf07b4e60419a568c0dd7a6ce6bcbfcc4d368f7b0b5ae4` |
| `haiku-trial-work.stderr` | `0bf0cc943f3f1ea3c5879ee004bc7d35a2a588800e4a971490bb0fbcc3f27963` |
| `haiku-trial-work.exit` | `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865` |
| `haiku-post-failure-state.txt` | `f54a363602fee58179fd5c585a46383f31cc12374950b63db2dbda880f0e1651` |
| `haiku-after-restart.txt` | `b6b071e5543a6c5d59860d2a2a5f548afa081fd3de977b2e8f3603b41e702d4d` |
| `haiku-recovery.txt` | `308b079fa0d30a9765d5bf2035181d53e4aac5ec693b4dc45dec576586fc46ed` |
| `haiku-post-recovery.txt` | `e726e8af63d40c09ba217a2086faceca9fda4ff1ca7422dd0b68f66668e4a06d` |
| `linux-reentry-prepare-2.txt` | `95cf859cc02961cbc1a5517ad1f0ad8b2855d666a2a426c9e432b42bca2dcc5e` |
| `linux-reentry-boot-2.txt` | `f798b1173092ff709a8ac7a945a11c8985a8dbbef019357fa6bd7c3eec422598` |
| `linux-reentry-boot.txt` | `76924807efe93864de60391988918badb2416097d7a355159baa2f2de939c3d6` |
| `linux-reentry-prepare.txt` | `eddeaca5b0754257623bfebc529cc2c2e1dede91f82c9a177f3f2fd892f4e1ae` |
