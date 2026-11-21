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

The prior tag0102 attempt transferred the full 2,740,498,432-byte model and
failed at model bind with I/O error during dispatch/fleet preflight. Its
clean-unload evidence is archived at
`../qwen35-dataflow-tag0102-20261121/evidence/tag0102-bind-failure/`. The
wrapper restored stable driver55 at generation 16, no reboot. Tag0103 has
only been built offline so far; the model has not been loaded under this
candidate and there is no generation or performance result for it.

The 27B setup remains preserved. Before the next card trial, rerun the
wrapper's read-only preflight to verify no diagnostic or inference process is
running and stable driver55 remains idle at its current generation.
