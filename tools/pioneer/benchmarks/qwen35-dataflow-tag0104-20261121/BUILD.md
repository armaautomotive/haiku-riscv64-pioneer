# Qwen3.5 resident dataflow route fix 0104

## Build

Built from the rewrite branch with:

```sh
python3 tools/pioneer/build_qwen35_dataflow.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-dataflow-20261121h
```

Tag0103 proved the failure was the check that rejected peer nonce zero before
the initial idle receipt was validated. Bootstrap intentionally leaves peer
nonces at zero until dispatch. Tag0104 removes that redundant nonzero check;
`PioneerQ38DispatchPeerStatus` still verifies all sixteen status words and
both waiting-PC reads for every peer. The model-bind substage/core diagnostics
remain enabled.

| Artifact | SHA-256 |
| --- | --- |
| Variant-60 driver, tag `2026112100010104` | `0babac6632f8e1dd5c06d006ef8b2cd8e397a196b04384f2073884f30b7302ad` |
| Haiku benchmark client | `16cecfc510dec0a046aaa230fe8bfffaf7487b231b3abb7ef00441fa5bfbd888` |
| Resident service text | `2e0e144040f9c29f9679d5300ff923847c75216b3bf0e577366be4e929e5f8c8` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Default-control driver | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` |
| Guarded trial wrapper | `0d7a811dcad3feb901fe4f13ac266752088dfae14a477cdcc953e63251770d09` |

## Hardware status

Tag0104 has built offline. The last card postflight after tag0103 showed
stable driver55 idle at generation 18. Tag0104 has not yet been installed or
run. Recheck that no 27B diagnostic or inference process owns the card and
that stable driver55 remains idle before using the one-shot trial wrapper.
