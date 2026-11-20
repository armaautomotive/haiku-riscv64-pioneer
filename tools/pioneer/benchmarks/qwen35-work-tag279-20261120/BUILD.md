# Qwen3.5-4B Work Tag79: corrected fused Gate+Up

Tag79 fixes Tag78's double-offset error by appending each tensor's bank
fragments at its logical base group. The scheduler accepts up to 14 fragments
for Gate plus Up, while preserving the distinct source ranges and output
buffers. Activation packing occurs once, all projection math remains on the
Tensix matrix path, and the peer image and quantized math are unchanged.

Build command:

```sh
python3 -B tools/pioneer/build_qwen35_work.py --device-opt-level 2 --peer-opt-level s \
  --peer-tile-prefetch /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-qwen35-tag79-composite-mapping-fix
```

The 14-fragment work schedule test, the Qwen35 token-core test, and the
cross-build passed. `StageBands` uses 4,784 bytes of stack. The ROOT image
includes `ProjectPair` and `StageBands`; the peer image is byte-identical to
Tag77.

| Artifact | SHA-256 |
| --- | --- |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `feed219b3d11b8025b0bb31ef3e8f9de79a56c2bc7a62344138540f5c231268a` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `8e165c2bdbc754106f51e8ab84203645bca0da1e6f663d8da4e4dda31eae72d9` |
| Candidate driver55 | `996409a022fdea26f5d49036fde571d4a99f4133e408357f91ce9a0062a29427` |
| Work client | `09f3f083c53f36cb6201c420090cf28f9e6f6f82f24fd202f822015f7b9d199f` |
