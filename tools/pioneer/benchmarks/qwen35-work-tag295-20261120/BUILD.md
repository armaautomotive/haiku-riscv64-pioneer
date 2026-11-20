# Qwen3.5-4B Work Tag95 build

Tag95 changes the active tied-vocabulary projection. ROOT and all 119 persistent Tensix peers each own a balanced contiguous range of 16-row groups; peers reduce their local logits in the existing projection scratch and return one `(best_logit, token_id)` pair. The previous path launched up to 16 groups per peer per dispatch wave and transferred every logit back to ROOT. The new path covers the 15,520 vocabulary groups in one all-core program, keeps the matrix projection on Tensix, and retains the checked peer completion/source/counter proofs.

The fixed 8 KiB peer-image limit prevents using Tag94's optional batch-fetch plus batch-prefetch profile with the new argmax path. This candidate uses the standard small-image build settings. The peer image and full driver/client build pass; this is not yet a hardware result.

## Offline verification

The whole-projection schedule test passed. The exact persistent peer image was run in the offline instruction simulator: for a 48-row fixture, the local argmax's value and token matched the full-logit reference exactly, with valid peer proof and source/counter completion. The full Work candidate cross-build passed the fixed image, import, disassembly, driver-tag, and client gates.

```sh
c++ -std=c++14 -Wall -Wextra -Werror tools/pioneer/tests/pioneer_qwen35_work_schedule_test.cpp \
  -o /private/tmp/pioneer-qwen35-work-schedule-tag95
/private/tmp/pioneer-qwen35-work-schedule-tag95

sh tools/pioneer/tests/run_pioneer_qwen35_argmax_peer_image_test.sh \
  /Volumes/HaikuBuildLocal/generated.riscv64

python3 -B tools/pioneer/build_qwen35_work.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-qwen35-work-tag295-candidate-b
```

## Artifact identities

| Artifact | SHA-256 | Bytes |
| --- | --- | ---: |
| ROOT boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` | 4 |
| ROOT text | `8122155aad574c05af31af9c521e4002924cb836d8710091fd8f57fc2db98933` | 54,512 |
| Peer boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` | 4 |
| Peer text | `5936aaf8d19749d2b625b6106018e4b077bfcf44cd0454363d8854adcd715390` | 7,988 |
| Candidate driver55 Tag95 | `350b8ab56d0ef6b27562fa04f4dab38a64216e9e442f0ee38fbb1433b33c84d0` | 252,328 |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` | 7,176 |
| Default43 control | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` | 934,080 |
| Work client | `79792acf48caa979628553656390b5d70b67628d0a8cb9f9dd3db74d1f8e3473` | 309,352 |

The physical trial is pending. See [`RUN.md`](RUN.md) and the guarded runner `pioneer_qwen35_work_trial.sh`.
