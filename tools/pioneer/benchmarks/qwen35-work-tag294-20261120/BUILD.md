# Qwen3.5-4B Work Tag94 build

Tag94 connects the two bank-sharded FP32 activation buffers to the active FFN
path. Gate and up projection results are published to device DRAM; the root
forms the SiLU-gated rows from those resident buffers; the down projection
reads its input from the resident gate buffer into a bounded ROOT staging
span before quantization. The buffers hold 9,216 rows each and are striped
over all seven GDDR banks.

This is an active Work-path consumer, but it is a first porting step: the
existing projection dispatcher still gathers matrix results into ROOT L1, and
the ROOT performs the activation-buffer transfers and SiLU operation. It does
not yet implement per-core output writers or remove the ROOT gather from the
projection boundary. No performance improvement is claimed before the card
run.

## Offline verification

The activation layout/range test passed under ASan/UBSan. The Work owner test
passed under ASan/UBSan. The physical PC/reset audit policy test and Work
schedule test passed. The Tag94 cross-build passed the fixed image, import,
disassembly, driver-tag and client gates.

```sh
c++ -std=c++14 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -fno-omit-frame-pointer tools/pioneer/tests/pioneer_qwen35_activation_plan_test.cpp \
  -o /private/tmp/pioneer-qwen35-activation-plan-tag94-final
/private/tmp/pioneer-qwen35-activation-plan-tag94-final

python3 -B tools/pioneer/build_qwen35_work.py \
  --device-opt-level 3 --peer-opt-level s \
  --batch-fetch --batch-prefetch --peer-exec-profile \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-qwen35-work-tag294-final-candidate
```

## Artifact identities

| Artifact | SHA-256 | Bytes |
| --- | --- | ---: |
| ROOT boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` | 4 |
| ROOT text | `57ff6232866bf4d93b3ec37fff3670cd31c5e84f87ca364c68bc8ae6e4a40516` | 92,468 |
| Peer boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` | 4 |
| Peer text | `d8f143a312ce44877526a88ae11256a8d493ce218be09ddf034d9f0c3c034a82` | 8,164 |
| Candidate driver55 Tag94 | `b9d3f3569c114de4612bd43fe73830075eca37d2a66950f4886a28fc33f2cf3f` | 292,328 |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` | 7,176 |
| Default43 control | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` | 934,080 |
| Work client | `79792acf48caa979628553656390b5d70b67628d0a8cb9f9dd3db74d1f8e3473` | 309,352 |

## Physical validation

Tag94 completed two exact warm outputs and a clean unload on the P100A. The
measured decode rates were 0.122351 and 0.122354 tok/s; model load was
224.121721 s and prompt processing was 180.166362/180.167662 s for 23 tokens.
The run did not meet the throughput target and showed no material decode
improvement over Tag91. The trial restored stable driver55 at generation 6
without reboot. Raw output and diagnostics are in [`evidence/tag94-card/`](evidence/tag94-card/),
with the full procedure and phase breakdown in [`RUN.md`](RUN.md).
