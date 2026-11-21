# Qwen35 resident dataflow checkpoint 0102

## Offline build

Built from branch `codex/qwen35-4b-p100a`, whose history descends from the
requested starting checkpoint `94117b2279`.

```sh
python3 tools/pioneer/build_qwen35_dataflow.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-dataflow-20261121d
```

The standalone Haiku client authenticates the fixed Q4_K_M GGUF, uploads its
packed weights through the resident span path, binds the model and graph
workspace, installs the resident dataflow service, generates twice with the
same prompt, and closes the service and bank session. It requires the known
first two output token IDs `11751,13`, matching the prior independent
vocabulary-only reference. It reports authentication, tokenization, model
load, prompt processing, and decode time separately. The client creates no
llama inference context or CPU fallback.

| Artifact | SHA-256 |
| --- | --- |
| Variant-60 driver, tag `2026112100010102` | `0f9e9b04ebcd66509f05da5053f47bd21c323122fd9e41db9ce5934e532cae7a` |
| Haiku benchmark client | `b2f45646e69e4ae7c83bff3f0572ed60c4ea97d822bebfda158f4456f1a49c08` |
| Resident service text | `2e0e144040f9c29f9679d5300ff923847c75216b3bf0e577366be4e929e5f8c8` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Default-control driver | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` |
| Guarded trial wrapper | `24bd29e63cdc076349cce4d85d8d32a72f58a3387c32ed20a146f1650e41768a` |

## Guarded hardware trial

`pioneer_qwen35_dataflow_trial.sh` is the separate one-shot transaction for
this client and candidate. It checks the pinned stable and candidate hashes,
model identity and 27B file presence, process idleness, RAM/CPU health, and
reload metadata before changing the installed driver. It keeps a recovery
lock until the candidate has released the device and stable driver55 has been
restored. It makes no power or storage changes. The client uses the
five-token reference prompt and requests eight new tokens twice from one
resident model load. The client validates each output's known `11751,13`
prefix and repeatability, then explicitly closes the dataflow service and
bank session.

## First hardware attempt

Pinned Haiku SSH to `10.0.0.247` succeeded. Before the exchange, the supported
Haiku process listing showed no 27B diagnostic or Qwen inference process.
Driver55 reported generation 14, `quiesced0 cold0 active_owner0 open_count1`.
The 27B GGUF and the pinned 4B GGUF were present with their expected sizes.

The guarded transaction installed variant60 tag `2026112100010102` at
generation 15, without reboot. Authentication took 68.232158 s; vocabulary
preparation took 12.463031 s and produced the known prompt IDs
`760,6511,314,9338,369`. All 2,740,498,432 model bytes were transferred into
resident bank storage. The subsequent model-bind ioctl (`0xaf47`,
`TT_PCI_PROBE_QWEN35_DATAFLOW_MODEL_BIND`) returned I/O error before model
authority was bound or the service installed. No prompt processing, output,
or decode-rate measurement was produced.

The client explicitly closed the service and bank session with
`unload_clean=1 state=2 active=0 cold=0 service_closed=1`. The wrapper then
restored stable driver55 at generation 16 without reboot. Final metadata was:

```text
P100A_RELOAD driver55 tag2026102100010012 generation16 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY
```

Raw state and client logs are archived under `evidence/tag0102-bind-failure/`.
The driver-bind branch returns I/O error when dispatch preparation, fleet
route proof, or core selection fails. The next rewrite step is to expose which
preflight substage/core failed while keeping all route checks strict, then
rebuild and take another guarded trial. The 100 tokens/s target remains
unmeasured.
