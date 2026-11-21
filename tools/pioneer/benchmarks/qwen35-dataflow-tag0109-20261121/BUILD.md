# Qwen3.5 dataflow rewrite tag0109

Tag0109 keeps the standalone resident BRISC/Tensix rewrite and adds two
diagnostics aimed at the tag0108 hardware failures:

- Generation ioctl validation now records which request field or service
  readiness check rejected the request. The client prints the request ABI
  fields when the ioctl fails.
- Fleet resume reports whether the captured reset state mismatched or the
  restored core failed the idle proof, and waits a bounded 10,000 pause spins
  after issuing the restore start command before checking readiness.

The request validation does not change the successful request ABI. Projection
and inference remain on the 120 Tensix workers; there is no CPU inference or
Work fallback. This checkpoint is an offline build only. It has not yet been
run on the Pioneer.

Built offline with:

```sh
python3 tools/pioneer/build_qwen35_dataflow.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-dataflow-20261121-tag0109-rebuild5
```

| Artifact | SHA-256 |
| --- | --- |
| Variant-60 driver, tag `2026112100010109`, dataflow ABI 3 | `b87123aab5ff32e7080f14addf7e8e8a945c100e6380032eb52745e6a0387f84` |
| Haiku dataflow client | `d55bed03a653529c78383c04d19584def654fda83d874257e06ab48e406be4a0` |
| Resident service text | `605cebc2507f86ff3feae51f30edb7f4fbdba12aa376f9bc037fc9faeadfd29f` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Default-control driver | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` |
| Service boot word | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Guarded two-warm trial wrapper | `ef3a56734785b2c9953e633ee1b2cc1019efc6965f2c6bf37347ebc71511c32a` |
| Post-cycle recovery helper | `4f78442e8300ca4c1997d7f87cdfd9b54593bc0d21858f153b24f2cd96fe9044` |

The owner and PCI-memory ASan/UBSan fixtures pass, including the restored-core
settle path. The RV32IMF target compile passes without A-extension atomics or
runtime helpers. The complete offline driver, service image, reload guard,
default-control package, and Haiku client build passes. The guarded trial and
recovery scripts pass `bash -n`.

## Hardware status inherited from tag0108

Tag0108 passed guarded preflight from stable driver55 generation2 idle and
uploaded all `2,740,498,432` resident model bytes. Authentication took
`100.027643` s, vocabulary/prompt tokenization took `13.277220` s, and
model load/upload took `334.815789` s. Its first generation ioctl returned
`EINVAL`, so there was no completed prompt processing, valid output, decode
time, warm-run rate, or tokens/s. Close failed during core0 fleet resume.
After one authorized cold recovery, the guarded helper restored stable driver55
generation2; the 4B and 27B model files remained intact. See the tag0108
[`RUN.md`](../qwen35-dataflow-tag0108-20261121/RUN.md) and its evidence.

Tag0109 also passed guarded preflight and uploaded the full model.
Authentication took `100.467732` s, prompt tokenization `13.243073` s, and
model load/upload `334.616263` s. Its first generation ioctl returned
`EINVAL`; close again failed at stage9/core0/detail3
(`DataflowServiceResumeIdleProofFailed`). No prompt processing, valid output,
decode timing, or tokens/s was produced. The wrapper retained the exclusive
lock and stable backup. One authorized cold recovery then restored stable
driver55 generation2 without changing SD media. The stable hash, absent lock,
idle state, both model sizes, and full Haiku CPU/RAM configuration were
verified. See [`RUN.md`](RUN.md) and the raw evidence. Tag0109 has not
validated its diagnostic changes on hardware. The target remains two valid
warm outputs and at least 100 decode tokens/s, with phase timings separated
and clean unload.
