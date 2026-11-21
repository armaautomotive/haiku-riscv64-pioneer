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

The last postflight restored stable driver55 at generation 20 with no
active owner. Tag0105 is an offline build only so far. Before installation,
the guarded wrapper must recheck process idleness, 27B file presence, health,
and stable-driver metadata. No generated output or throughput result exists
yet.
