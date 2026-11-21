# Qwen3.5 dataflow rewrite tag0116

Tag0116 adds stage-level diagnostics for the embedding weight read that failed
on core0 in tag0114 and tag0115. The resident NoC read now records where it
stopped, the status-register or counter index, and the observed 32-bit value.
For a failed embedding read, the worker publishes this through two existing
reserved words in its fixed 64-byte mailbox. The host includes a compact
stage/index code and the captured value in the existing query diagnostic.
The worker failure code stays `EmbeddingWeightRead`; the mailbox and ioctl
request sizes stay unchanged. No command is cleared, and no core is reset.

The NoC stage values are declared in
`PioneerQwen35DataflowBlackholeIO.h`. The low six index bits identify a NoC
status register (0..35), completion counter (36..43), initiator doorbell (44),
command word (45..54), route identity (55), or invalid descriptor (56). The
recovery detail's bit10 marks this packed form; the adjacent 32-bit query
result contains the raw value captured by the worker at failure time.

## Build outputs

Built offline with:

```sh
python3 tools/pioneer/build_qwen35_dataflow.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-dataflow-20261121-tag0116-rewrite22-build
```

| Artifact | SHA-256 |
| --- | --- |
| Variant-60 driver, tag `2026112100010116`, dataflow ABI 3 | `2acf08ecf6578adcd3529c65259489eb92b4ceca5ffdcb78f6e75f1f0034e710` |
| Haiku dataflow client | `52097a6efb88e89181b96a7476a291fb8afb1d7c828cc9bbf03c1c401c857106` |
| Resident service boot word | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Resident service text | `e48aba4107df19fd5c1e5c888f772459a3d69ba4f53a8dc83f9efe24f62352bd` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Default-control driver | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` |

## Offline validation

- Full variant-60 driver, resident service image, and Haiku client build pass.
- Conservative kernel stack use is `15712/18432` bytes; no Qwen35 frame is
  above 8192 bytes. Generation remains inlined into the measured ioctl frame.
- Runtime recovery, embedding worker, projection owner, and service-owner
  fixtures pass with ASan/UBSan. They cover the packed transfer detail and
  preserve stuck NoC state without writes.
- The role contracts compile as RV32IMF with no A-extension atomics or
  unresolved runtime helpers.

Tag0116 has not been installed on the card. Haiku is running the preserved
stable55 driver at generation2, idle, after the power reset. The next device
benchmark remains gated on completion of the rewrite checkpoint. Tag0115's
hardware failure and recovery evidence are in
[`tag0115 RUN`](../qwen35-dataflow-tag0115-20261121/RUN.md).
