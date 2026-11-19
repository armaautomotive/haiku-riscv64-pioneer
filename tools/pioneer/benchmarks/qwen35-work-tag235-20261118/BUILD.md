# Qwen3.5-4B Work tag235 diagnostic build

Built on 2026-11-18 from the current workspace with the local Haiku RISC-V64
build at `/Volumes/HaikuBuildLocal/generated.riscv64`:

```sh
python3 -B tools/pioneer/build_qwen35_work.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 OUTPUT
```

Tag235 keeps the tag234 model, seven-bank upload, all-120-core projection
schedule, and Work-only execution mode. It adds a failure-only dispatcher
receipt: every validation path that returns zero records its source line and
small context value in ROOT progress reserved words 2 and 3. The Work owner
prefers this tagged receipt over an empty or stale L1 checkpoint. Successful
requests do not write the receipt fields. The change is intended to identify
which dispatcher validation rejects the scheduled matrix job; it does not
change any validation condition or inference arithmetic.

The Work owner unit test passed, including a fault case where the progress
receipt must win over a stale dispatcher-entry checkpoint. The image build
passed its undefined-import and unsupported-instruction checks. No hardware
run has been made with tag235 yet.

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55, tag `2026111800010035` | `505f697cdf66147e816ef151be8118cfaa5a8810b0245d49358322f5e60beff5` |
| Work client | `2a430e7fc80e41eacf2ccef526e32f2badabfdf83cae19527492fe427f047ea4` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `c466e9cf6209e7fc2ee6651047571babc99e6637e2ff7cf0d6e29b5edc84d1e4` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `feef2e513367a83f93c11142037c59e2a6af5aed1d7921d9cf031daa4fe63047` |
| Trial wrapper | `a53d271c764021d27b570d3073dfefa6f12add4753691aacd9de3e9bd17a2149` |
| Recovery helper | `dd1dc4deece2b68d683110625fc604afd47f05ba17f65600149bd9bb0dced312` |

Offline build output: `/private/tmp/pioneer-qwen35-work-offline-235`.
The candidate is staged by the guarded trial wrapper at
`/boot/home/pioneer-qwen35-work-235.Rt51p`.
