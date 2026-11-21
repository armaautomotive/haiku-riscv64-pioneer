# Tag0109 hardware run

## Preflight

The guarded preflight passed before installation:

- Stable driver55 tag `2026102100010012`, generation2, was idle with
  `quiesced0 cold0 active_owner0`.
- The process list contained no inference client or 27B diagnostic.
- Haiku reported 64 CPUs and `137,034,936,320` bytes maximum RAM.
- The 4B model was `2,740,937,888` bytes; the preserved 27B model was
  `18,973,870,528` bytes.
- Candidate driver SHA-256:
  `b87123aab5ff32e7080f14addf7e8e8a945c100e6380032eb52745e6a0387f84`.
- Client SHA-256:
  `d55bed03a653529c78383c04d19584def654fda83d874257e06ab48e406be4a0`.

## Model setup and generation attempt

Tag0109 installed as generation3 and uploaded the entire model to resident
memory.

| Phase | Result |
| --- | ---: |
| Authentication | 100.467732 s |
| Vocabulary/prompt tokenization | 13.243073 s |
| Prompt IDs | `760,6511,314,9338,369` |
| Model load/upload | 334.616263 s |
| Resident bytes | 2,740,498,432 |
| Prompt processing | No completed measurement |
| Decode | No completed measurement |
| Valid warm outputs / decode rate | None |

The first generation ioctl `0xAF56` returned `EINVAL`. The added request
diagnostic reported `bytes=80 version=5 prompt_count=5 max_new_tokens=8
input_slot=0 eos=248046`, with prompt buffer `0x00000033847e1000` and output
buffer `0x00000033847e1020`. The user's expected-prefix check then failed; no
generated text, prompt time, decode time, warm-run rate, or tokens/s are
claimed.

## Close and recovery state

Close returned result18 (`DataflowServiceResumeFailed`), stage9, core0,
detail3 (`DataflowServiceResumeIdleProofFailed`), with worker route `0x2`.
The client reported
`unload_clean=0 state=0 active=0 cold=0 service_closed=0`. The guarded wrapper
refused restoration and left tag0109 at generation3 `cold1 active_owner1`,
retaining the exclusive lock and the stable-driver backup.

After the client and 27B diagnostic exited, a read-only snapshot confirmed no
inference process, unchanged model file sizes, installed candidate hash
`b87123aa…`, and stable backup hash
`041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403`. After
one authorized OFF/ON and the saved disk-identity-aware, SD-free
Linux-to-Haiku handoff, the guarded recovery helper observed fresh candidate
generation1, quiesced it, and restored stable driver55 generation2. Postflight
confirmed `cold0 active_owner0`, no exclusive lock, and matching installed
and saved stable driver hashes
`041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403`. No
inference or 27B process remained; Haiku reported all 64 CPUs and
`137,034,936,320` bytes maximum RAM. The 4B and 27B files remain at their
verified sizes. The post-recovery snapshot is in
[`evidence/post-recovery.txt`](evidence/post-recovery.txt). The 100 decode
tokens/s goal remains unmet.
