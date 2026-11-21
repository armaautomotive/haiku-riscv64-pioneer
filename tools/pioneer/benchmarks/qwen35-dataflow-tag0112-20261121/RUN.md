# Qwen3.5 dataflow tag0112 hardware run

## Outcome

Tag0112 fixed the tag0111 kernel-stack overflow sufficiently to reach the
generation path without a kernel panic. The guarded run authenticated and
uploaded the model and installed the resident 120-core service. The first
prompt embedding failed on core0 with a reader-role worker failure. It
produced no token. Same-owner recovery and service close both failed their
all-core audits, so the wrapper correctly retained its lock and stable55
backup instead of exchanging drivers while the candidate reported
`cold1 active_owner1`.

Cold recovery restored the card to stable driver55 generation2. Postflight
confirmed the recovery lock is absent, the stable driver hash is unchanged,
the 4B and 27B model files retain their expected sizes, and Haiku again has
64 CPUs and the full 137034936320-byte RAM profile. The saved 27B setup is
intact.

This run produced no valid warm output, no completed prompt-processing time,
no decode time, and no tokens/sec measurement. It did not meet the 100
decode tokens/s target or the clean-unload gate. The lower-level cause of the
core0 reader-role failure is still unknown; tag0113 rewrite work must localize
that failure and make recovery diagnostics actionable before another hardware
trial.

## Preflight and timings

The read-only preflight found the previous 27B diagnostic and inference clients
absent. Stable driver55 tag `2026102100010012`, generation2, reported
`quiesced0 cold0 active_owner0 open_count1`; the exclusive lock was absent.
The guard verified Qwen3.5-4B at 2740937888 bytes and the preserved
Qwen3.8-27B at 18973870528 bytes. Haiku reported all 64 CPUs and
137034936320 bytes of RAM.

| Phase | Result |
| --- | ---: |
| GGUF authentication | 98.363993 s |
| Vocabulary open and prompt tokenization | 13.123478 s; IDs `760,6511,314,9338,369` |
| Model load/setup | 328.264191 s; 2740498432 resident payload bytes |
| Prompt processing | Failed at 0.612929 s; partial time only |
| Decode | Not reached; no decode time or tokens/s |

The model upload/setup completed. The first generation request returned
`result=31` (`DataflowServiceGenerationFailed`), `stage=4`
(`DataflowDecoderEmbeddingRunFailed` at the final prompt token),
`state=2 recovery=1`, `output=0`, and `decode_us=0`. The owner diagnostic
reported `owner_result=8` (`DataflowOwnerWorkerFailed`), core0, embedding
operation, worker state4 (`Failed`), failure1 (`Reader`), phase5, and pending
sequence1. This receipt does not yet distinguish invocation validation, the
NoC 0 read, Q6_K unpack, or output write as the lower-level failure.

The client attempted QUERY/RECOVER before close. RECOVER returned result13
(`DataflowServiceRecoveryPending`), and CLOSE returned result15
(`DataflowServiceStopAuditFailed`) at stage7/core0. No second warm run was
started because the first request failed. The client exited with status1 and
reported `unload_clean=0`.

## Guarded recovery

The wrapper rejected automatic restore because the candidate remained cold
with its active-owner latch set. It retained the exclusive lock and
`restore.incoming`; the administrative quiesce helper refuses a cold or
active-owner driver. The supported same-owner recovery attempt had already
failed, so I preserved the logs and used one authorized cold recovery: one
seven-second OFF button hold, a three-minute shutdown wait, and one
0.75-second ON pulse after both pinned OS SSH identities timed out. Linux
returned on the default NVMe boot. Its hash-verified, SD-free
`memory128-spin-idle7-top-page-nosd-control` kexec preflight passed, then it
returned to Haiku without writing the SD card or changing default boot
storage.

The tag0112 recovery helper verified the fresh candidate generation1,
stable backup, model files, and idle system. It quiesced the candidate,
restored the saved stable driver55, verified generation2, and removed the
lock. Independent postflight confirmed stable55 hash
`041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403`, no
inference/diagnostic process, unchanged 4B/27B file sizes, 64 CPUs, and full
RAM.

## Evidence hashes

| File | SHA-256 |
| --- | --- |
| `preflight.txt` | `2f3e8eeae74ddacc48c26509c9902efc73d79cdf64cd5eadd58ed272a93f3f81` |
| `wrapper.txt` | `6e9c44916379dc2d37bd70765c05070cb9e4b1a6a2ac6f250ece89cd309f4240` |
| `work.stdout` | `0e9844e2c1fb571c3305110499dc1c61f0db3d84f2ce7ebbab13d1dc1c452955` |
| `work.stderr` | `9fcde70b5b109b965359cbea6f47fa5ca2c516c25ed92739361865decd791ca7` |
| `work.exit` | `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865` |
| `pre-recovery.txt` | `245a17c4549a88e29dfa256d727983723a273ba1d3bcc96da033dfa9660dd9b0` |
| `linux-preflight.txt` | `63895b7e58d0dce8ce2bac5a496cad9dcf456fbc82e50bf877a8de29d8413690` |
| `linux-prepare.txt` | `7ca61dbeb610c2f67b8da93a9825edd73aab7c17cc2cdaaf57456e923c37cd22` |
| `linux-handoff.txt` | `4a17d75b25f79ecfafe348adc5b6dbb97cd9ce162327b76bd2fbf6e4c993edcf` |
| `recovery.txt` | `9e7bb61f115d789629b3efe501aecabd74addd817c58e3a1ec3b04d1508cd1bd` |
| `state.post-recovery.txt` | `507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e` |
| `post-recovery.txt` | `1eee000cbef37a0d350e02063e4a0898e9c4540673b12c29943e5a6153447285` |

All raw files are under [`evidence/`](evidence/). The offline build and stack
audit are in [`BUILD.md`](BUILD.md).
