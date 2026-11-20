# Qwen3.5-4B Work Tag80: same-activation projection groups

Tag80 generalizes Gate+Up fusion to up to four projections sharing one input.
The ROOT packs each activation once per quant type, then maps up to 28 bank
fragments into a single Tensix dispatch for that type. Attention Q/K/V,
recurrent QKV/gate/alpha/beta, and MLP Gate/Up use this path. When source
tensors have different quant types, they remain separate Work dispatches;
there is no host CPU inference or CPU fallback.

Build command:

```sh
python3 -B tools/pioneer/build_qwen35_work.py --device-opt-level 2 --peer-opt-level s \
  --peer-tile-prefetch /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-qwen35-tag80-multirole-fusion
```

The 28-fragment scheduler test, Qwen35 token-core test, and cross-build
passed. The ROOT image contains `ProjectGroup` and `StageBands`; `StageBands`
uses 5,088 bytes of stack. The peer image and checked quantized matrix math
remain unchanged.

| Artifact | SHA-256 |
| --- | --- |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `51ff8d88f26a727f8a4c5a4abca00d6632f3a0ba119928a05241c1582fa5543c` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `8e165c2bdbc754106f51e8ab84203645bca0da1e6f663d8da4e4dda31eae72d9` |
| Candidate driver55 | `25efcd64023910ab438894912116c4a632e01222b9936685db06e6e4bebb02dc` |
| Work client | `09f3f083c53f36cb6201c420090cf28f9e6f6f82f24fd202f822015f7b9d199f` |
