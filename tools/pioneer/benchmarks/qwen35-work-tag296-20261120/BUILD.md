# Qwen3.5-4B Work Tag96 build

Tag96 retains Tag95's all-peer vocabulary argmax program and fixes the Tag95 completion fault. The resident sentinel selector accepts the normal 16-group maximum by default; for an argmax header only, the Work dispatcher passes the vocabulary-wide group bound (`248,320 / 16`). The helper still selects the largest shard as a scheduling hint and `WaitPeers` still gathers status, physical PC/reset, source, and counter proof from every peer.

## Offline verification

- The sentinel selector unit test passed 15,232 cases, including default 16-group refusal and argmax shard sizes 130–132 with a 15,520-group ceiling.
- The Work schedule test passed its coverage/balance cases.
- The exact persistent peer image replay matched local argmax value/token against the full-logit reference, with completion/source/counter proof.
- The extracted `WaitPeers` budget test passed the original per-worker poll cap, shared global budget, and all-peer audit checks.
- The full candidate cross-build passed fixed image, import, disassembly, driver-tag, and client gates.

```sh
c++ -std=c++17 -Wall -Wextra -Werror tools/pioneer/tests/pioneer_q38_poll_sentinel_test.cpp \
  -o /private/tmp/pioneer-q38-poll-sentinel-tag96
/private/tmp/pioneer-q38-poll-sentinel-tag96

c++ -std=c++14 -Wall -Wextra -Werror tools/pioneer/tests/pioneer_qwen35_work_schedule_test.cpp \
  -o /private/tmp/pioneer-qwen35-work-schedule-tag96
/private/tmp/pioneer-qwen35-work-schedule-tag96

sh tools/pioneer/tests/run_pioneer_qwen35_argmax_peer_image_test.sh \
  /Volumes/HaikuBuildLocal/generated.riscv64

python3 -B tools/pioneer/build_qwen35_work.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-qwen35-work-tag296-candidate
```

## Artifact identities

| Artifact | SHA-256 | Bytes |
| --- | --- | ---: |
| ROOT boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` | 4 |
| ROOT text | `1e69aab2f3ff22dbaff63b300122d0547613407af23c02d0db90066b7e85882b` | 54,536 |
| Peer boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` | 4 |
| Peer text | `5936aaf8d19749d2b625b6106018e4b077bfcf44cd0454363d8854adcd715390` | 7,988 |
| Candidate driver55 Tag96 | `5b11300b42e8eb69372e6440056b015cf6a9556d3262799b0f2b9ec2370052aa` | 252,352 |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` | 7,176 |
| Default43 control | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` | 934,080 |
| Work client | `79792acf48caa979628553656390b5d70b67628d0a8cb9f9dd3db74d1f8e3473` | 309,352 |

The physical trial is pending. See [`RUN.md`](RUN.md) and the guarded runner `pioneer_qwen35_work_trial.sh`. Tag95's failure and clean recovery are recorded in [`../qwen35-work-tag295-20261120/RUN.md`](../qwen35-work-tag295-20261120/RUN.md).
