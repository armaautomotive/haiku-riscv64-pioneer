# Qwen3.5 dataflow rewrite tag0118

Tag0118 keeps the 64-byte worker mailbox and 36-byte control ioctl fixed. A
failed NoC completion-counter read now preserves both the observed counter
and the expected counter value. The owner obtains the latter from the worker's
stable mailbox before recovery can acknowledge or clear it. The new
`QUERY_EXPECTED` control action reports that value while retaining the existing
query result, recovery detail, and observed counter. Dataflow ABI is 4.

## Build outputs

Built offline with:

```sh
python3 tools/pioneer/build_qwen35_dataflow.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-dataflow-20261121-tag0118-expected-counter-build6
```

| Artifact | SHA-256 |
| --- | --- |
| Variant-60 driver, tag `2026112100010118`, dataflow ABI 4 | `2c7bb5e1e7fc21e0bec3163ad9b6d6eeec26f6e54b0a325fd4cf8f226bd7501f` |
| Haiku dataflow client | `b23a0d675c312c145598ae8e2f8a939759b0cf310d7369e344f6947068e3fd26` |
| Resident service boot word | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Resident service text | `e30883fc9089400ac03839cc069fcabcfb8ecafa5f5ae9b095b4506e20221abc` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Default-control driver | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` |

## Offline validation

- Full variant-60 driver, resident service image, and Haiku client build pass.
- Conservative kernel stack use is `15712/18432` bytes; no Qwen35 frame is
  above 8192 bytes.
- Runtime recovery, embedding worker, owner, and service-owner ASan/UBSan tests
  pass, including expected-counter propagation through the owner diagnostic.
- Worker role contracts compile as RV32IMF without A-extension atomics or
  unresolved runtime helpers.
- Build and card-wrapper scripts pass shell syntax validation.

Tag0118 has not been installed on the card. The focused one-run card diagnostic
is gated on strict stable55 idle preflight. Tag0117's failure and the stable
driver recovery after the accidental power reset are recorded in
[`tag0117 RUN`](../qwen35-dataflow-tag0117-20261121/RUN.md).
