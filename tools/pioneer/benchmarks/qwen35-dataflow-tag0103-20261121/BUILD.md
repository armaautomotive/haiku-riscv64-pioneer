# Qwen3.5 resident dataflow diagnostic checkpoint 0103

## Offline build

Built with:

```sh
python3 tools/pioneer/build_qwen35_dataflow.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-dataflow-20261121g
```

This keeps the variant-60 resident generation architecture and adds a
diagnostic result to the model-bind request. The driver reports whether bind
failed in manifest validation, dispatch preparation, all-core route proof,
root reselection, or authority binding. Fleet-route failures also report the
exact stage and core/bank. The validation predicates remain unchanged.

| Artifact | SHA-256 |
| --- | --- |
| Variant-60 driver, tag `2026112100010103` | `06153f5c7bfb81179f9b5f3b3d51e176a1ea5465393023a94060edaf345c442c` |
| Haiku benchmark client | `16cecfc510dec0a046aaa230fe8bfffaf7487b231b3abb7ef00441fa5bfbd888` |
| Resident service text | `2e0e144040f9c29f9679d5300ff923847c75216b3bf0e577366be4e929e5f8c8` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Default-control driver | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` |
| Guarded trial wrapper | `0410236fc062287d37eb7762f392df88ad0fd8fa1c25c6fca6877e971f7bfa68` |

## Hardware status

Tag0103 was loaded after stable driver55 generation 16 was verified idle. The
full model upload completed, then bind diagnostics identified the redundant
zero-peer-nonce rejection on core 1. No service or generation was run. The
client closed cleanly, and the wrapper restored driver55 at generation 18
without reboot. Evidence is archived in `evidence/tag0103-peer-nonce/`.

The tag0104 code removes only that redundant rejection. Its updated route
proof still requires a valid idle peer receipt with the exact status words,
marker, and waiting PC.

The 27B setup remains preserved. Before the next card trial, rerun the
wrapper's read-only preflight to verify no diagnostic or inference process is
running and stable driver55 remains idle at its current generation.
