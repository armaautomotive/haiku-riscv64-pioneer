# Qwen3.5-4B Work Tag97 build

Tag97 scales the resident sentinel completion window by the selected shard's
size: `ceil(groups / 16) * 65,536` polls. A 130-group vocabulary shard gets
589,824 local polls instead of 65,536. The original shared
`workers * 65,536` poll budget remains in force, and the root still requires
status, physical PC/reset, source, and counter proof from every peer before
publishing output. The Tag96 transient was observed after this per-peer window
expired, so this is a targeted bounded-wait correction, not a throughput fix.

## Offline verification

- Sentinel budget cases pass for groups 1, 16, 17, 130, and 15,520, plus zero
  and overflow rejection.
- The extracted `WaitPeers` test passes a 70,000-poll 130-group shard, verifies
  the exact 589,824-poll local cap, retains the original global cap, and audits
  all 119 workers before returning success.
- The persistent Tensix peer argmax image replay returns the exact local
  argmax pair for 48 rows and retains source/counter/completion proof.
- The full default Tag97 Haiku cross-build passes image, imports, disassembly,
  driver-tag, and client gates.

```sh
c++ -std=c++17 -Wall -Wextra -Werror tools/pioneer/tests/pioneer_q38_poll_sentinel_test.cpp \
  -o /private/tmp/pioneer-q38-poll-sentinel-tag97
/private/tmp/pioneer-q38-poll-sentinel-tag97

sh tools/pioneer/tests/run_pioneer_q38_wait_peers_budget_tests.sh
sh tools/pioneer/tests/run_pioneer_qwen35_argmax_peer_image_test.sh \
  /Volumes/HaikuBuildLocal/generated.riscv64

python3 -B tools/pioneer/build_qwen35_work.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-qwen35-work-tag297-candidate
```

## Artifact identities

| Artifact | SHA-256 | Bytes |
| --- | --- | ---: |
| ROOT boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` | 4 |
| ROOT text | `e1d27d0e40fc73cfcfdf72b82dc5b0442dafcb16677bb4f166778a75fc7a3dcc` | 54,564 |
| Peer boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` | 4 |
| Peer text | `5936aaf8d19749d2b625b6106018e4b077bfcf44cd0454363d8854adcd715390` | 7,988 |
| Candidate driver55 Tag97 | `42f6ec6f2225acf20328457c7b1dd875ac5959fce7142637305fc3b5a6762ec3` | 252,384 |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` | 7,176 |
| Default43 control | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` | 934,080 |
| Work client | `79792acf48caa979628553656390b5d70b67628d0a8cb9f9dd3db74d1f8e3473` | 309,352 |

The P100A trial is pending. Start only after strict preflight confirms the
previous diagnostic has exited and stable driver55 is idle. The runner must
restore stable driver55 after either success or safe fault recovery.
