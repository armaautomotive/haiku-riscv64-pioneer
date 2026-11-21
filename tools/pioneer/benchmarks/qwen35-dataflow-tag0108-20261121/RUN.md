# Tag0108 hardware result — 2026-11-21

## Preflight

- The pinned Haiku endpoint reported all 64 CPUs and `137,034,936,320` bytes
  maximum RAM.
- Stable driver55 tag `2026102100010012` was idle at generation2 with no trial
  lock.
- No inference or 27B diagnostic process was running.
- The verified 4B model was `2,740,937,888` bytes and the preserved 27B model
  was `18,973,870,528` bytes.
- Candidate driver SHA-256:
  `62d64043cf5f3e2110b96446839efdfe2c61a945c8d87cd21d9d9a52072de1dc`.
- Client SHA-256:
  `cbee1483b73fa8e08754fbd04d1f78376e725ae7dc6e078ad271fdc2a58a256a`.

## Model setup and generation attempt

Tag0108 installed as generation3 and transferred the full 4B model to its
resident dataflow session.

| Phase | Result |
| --- | ---: |
| Authentication | 100.027643 s |
| Vocabulary/prompt tokenization | 13.277220 s |
| Prompt IDs | `760,6511,314,9338,369` |
| Model load/upload | 334.815789 s |
| Resident bytes | 2,740,498,432 |
| Prompt processing | No completed measurement |
| Decode | No completed measurement |
| Valid warm outputs / decode rate | None |

The first generation ioctl `0xAF56` returned `EINVAL`. The client's later
expected-prefix and repeatability checks failed because the generation request
was rejected; no generated text or decode tokens/s are claimed.

## Close and recovery

Close returned result18 (`DataflowServiceResumeFailed`), diagnostic stage9,
core0, resume phase3, detail2 (`DataflowServiceResumeIdleProofFailed`). The
raw client label `audit_detail=3` is the resume phase, while its final
`worker_route=0x0` field is the resume detail. The client reported
`unload_clean=0 state=0 active=0 cold=0 service_closed=0`. The guarded wrapper
refused to restore a candidate that reported `cold1 active_owner1`, retaining
the one-shot lock and stable driver backup.

The client and any Qwen/27B diagnostic were verified exited before one
authorized OFF/ON recovery cycle. The saved disk-identity-aware, hash-pinned
SD-free handoff booted Haiku. The guarded tag0108 recovery helper quiesced the
fresh generation1 candidate and restored stable driver55 generation2. A
read-only postflight confirmed stable55 idle, no trial lock, all 64 CPUs/full
RAM, and unchanged 4B/27B file sizes. Exact artifacts are in
[`evidence`](evidence/).

The next rewrite checkpoint must identify the generation ioctl's validation
failure and the core0 resume phase3 failure before another hardware run. The
100 decode tokens/s goal remains unmet.
