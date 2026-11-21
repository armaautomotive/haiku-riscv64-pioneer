# Qwen3.5 dataflow rewrite tag0110

Tag0110 addresses the two confirmed tag0109 defects and improves recovery
evidence:

- `ProbeControl` now routes the full contiguous Qwen35 private ioctl range,
  including the FFN, recurrent, dense, decoder, and generation operations.
  The prior gate ended at `RESIDUAL`, so generation never reached its request
  validator. Compile-time assertions protect the ioctl range boundaries.
- Generation failure logs include the returned request fields, allowing a
  caller to distinguish an outer dispatch rejection from a structured kernel
  validation response.
- Restore diagnostics now carry the specific idle-proof subcheck. The
  reset-held dispatcher root is checked against its full captured state and
  stable reset value; it is not incorrectly subjected to the running-core
  idle proof after other dispatcher workers resume.
- The client retries a failed close up to two times when the driver reports a
  recoverable resume failure, while the original service owner still retains
  its snapshots and file descriptor.

Projections remain on the resident 120-core Tensix service. The standalone
client has no CPU inference or Work fallback.

Built offline with:

```sh
python3 tools/pioneer/build_qwen35_dataflow.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-dataflow-20261121-tag0110-rebuild2
```

| Artifact | SHA-256 |
| --- | --- |
| Variant-60 driver, tag `2026112100010110`, dataflow ABI 3 | `a9b79420fd351bfb7cbcdda2e70565455973a80a154ca54ba985b1d714f3bd5d` |
| Haiku dataflow client | `b90f94175aa8a847f360b9ac2a118596d1cd092d37e475ea7043f0281a9d43fe` |
| Resident service text | `605cebc2507f86ff3feae51f30edb7f4fbdba12aa376f9bc037fc9faeadfd29f` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Default-control driver | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` |
| Service boot word | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Guarded two-warm trial wrapper | `d740f23415dc5087bd14c0311e960331e477dc0a8d064bece8317dde4a2ff233` |
| Post-cycle recovery helper | `a3491ecdb3cb7033997c321fdf13c01ffddc621ac2c34b5d65d5e75a15e1f337` |

The complete offline driver/client/package build passes. The owner and
PCI-memory ASan/UBSan fixtures pass, including close retry and restore
diagnostics. The RV32IMF role compile passes without A-extension atomics or
runtime helpers.

Tag0110 has not been run on hardware. Latest hardware evidence remains
tag0109: model load completed, generation was rejected because of the missing
outer ioctl route, and close failed the held-root idle proof. Stable driver55
generation2 has since been restored. See
[`tag0109 RUN.md`](../qwen35-dataflow-tag0109-20261121/RUN.md). The next phase
is still rewrite work; do not benchmark until the rewrite is complete.
