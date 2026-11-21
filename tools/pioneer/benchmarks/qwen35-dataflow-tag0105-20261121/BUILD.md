# Qwen3.5 dataflow service-install correction 0105

Tag0104 passed model binding but stopped at resident-service installation.
The install-time `DataflowCoreIdleForHold()` had a second nonzero peer-nonce
rejection. Tag0105 removes that condition while keeping the full peer receipt
and waiting-PC proof. The client now prints the service-install result, state,
sequence, and recovery fields whenever installation does not complete.

Built with:

```sh
python3 tools/pioneer/build_qwen35_dataflow.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-dataflow-20261121i
```

| Artifact | SHA-256 |
| --- | --- |
| Variant-60 driver, tag `2026112100010105` | `f4c120e7c00c77d72db492c116468309847521ca067eb2c849469327851991d4` |
| Haiku benchmark client | `08fcae32d9f3c5f3b088bc97918c189f04b80ca91e04255075c7b6117ee33ce9` |
| Resident service text | `2e0e144040f9c29f9679d5300ff923847c75216b3bf0e577366be4e929e5f8c8` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Default-control driver | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` |
| Guarded trial wrapper | `3a6816f9c6564888768293bb45723012d227e60c548aad1885d961fd6808c5dd` |

## Hardware result

The guarded preflight passed at stable driver55, generation20, with the prior
27B diagnostic exited and both model files present. The tag0105 trial passed
model authentication, vocabulary/tokenization, upload of all 2,740,498,432
bytes, model binding, and fleet route proof. Authentication took 67.540150 s;
vocabulary/tokenization took 13.062857 s. Service install returned
`result=11 state=2 next_sequence=0 recovery=1`, which is
`DataflowServiceReadyFailed` after the worker-ready barrier. The client
reported `cleanup=0 fd_closed=1`, but the subsequent status showed
`driver60 tag2026112100010105 generation21 quiesced0 cold1 active_owner1`.
The guarded wrapper retained its recovery lock and exact stable041 backup.

No prompt processing or token generation occurred. Model-load duration,
prompt duration, decode duration, and decode tokens/s are unavailable. The
raw state, exit code, stdout, stderr, and hashes are under
[`evidence/tag0105-ready-failure`](evidence/tag0105-ready-failure/).

An authorized single power cycle and the strict-pinned SD-free Linux-to-Haiku
kexec handoff returned to Haiku hrev99999 with 64 CPUs and
137,034,936,320 bytes of RAM. After that fresh boot, the candidate reports
generation1, `cold0`, and no active owner. The installed candidate and
`restore.incoming` still match their pinned SHA-256 values; the 4B and 27B
model files retain their expected exact sizes. The post-cycle stable-driver
restore is the next recovery step. Do not start another candidate until
stable driver55 is verified idle and the recovery lock is removed.
