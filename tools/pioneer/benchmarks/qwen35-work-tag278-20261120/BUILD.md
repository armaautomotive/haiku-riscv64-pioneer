# Qwen3.5-4B Work Tag78: fused Gate+Up attempt

Tag78 added a paired Gate+Up projection path. It packed the shared activation
once and submitted the two same-type tensors as one 14-fragment work batch,
while keeping their output buffers and all matrix checks separate. The ROOT
and peer images compiled successfully, and the 14-fragment scheduling and
token-core checks passed.

Build command:

```sh
python3 -B tools/pioneer/build_qwen35_work.py --device-opt-level 2 --peer-opt-level s \
  --peer-tile-prefetch /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-qwen35-tag78-gateup-fusion2
```

| Artifact | SHA-256 |
| --- | --- |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `f59876f03e5beca793a16136f70e299bc7306a05c81ff69cf218ceefd743d632` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `8e165c2bdbc754106f51e8ab84203645bca0da1e6f663d8da4e4dda31eae72d9` |
| Candidate driver55 | `670fa1539d48d5d031d9eaedeb827f733d0bba40b9f755388c4f082eb729ca9e` |
| Work client | `09f3f083c53f36cb6201c420090cf28f9e6f6f82f24fd202f822015f7b9d199f` |

The first fused task was safely rejected because the second tensor's logical
fragment offset was added twice when resolving its bank-local range. Tag79
corrects that mapping. No valid output was produced by Tag78.
