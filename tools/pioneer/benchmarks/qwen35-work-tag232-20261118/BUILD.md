# Qwen3.5-4B Work tag232 build

Built locally on 2026-11-18 for the next guarded Pioneer trial. Tag232 keeps
the tag231 Work-only peer failure diagnostics and adds root-token-path tracing.
The progress record's `reserved[0]` slot records the last attempted operation
when a token request fails. Successful `FinishRequest` clears that slot so the
strict result validator still requires it to be zero.

Trace groups:

- `0xA...`: root/operator phase; bits 27:24 are the family, bits 23:16 the
  layer, and the low 16 bits the detail value.
- `0xD1...` through `0xD9...`: embedding, weight read, RoPE read, convolution
  read/write, delta read/write, and KV read/write.
- `0xDA...` through `0xDF...`: matrix dispatch, projection, tied argmax,
  prefill boundary, token commit, and request finish.
- `0xE5...`: `StageBand` validation/result failure codes 1 through 17.

The trace is diagnostic instrumentation. It does not introduce CPU inference:
projection and tied-output work still enters the ROOT/Tensix dispatcher, and
Work admission requires `hostInference==0`.

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55, build tag `2026111800010032` | `7d406f2f390f26add4e4b482e6e6196cfb43ad1eb7417a01b21a5f54ed34e95a` |
| Work client | `2a430e7fc80e41eacf2ccef526e32f2badabfdf83cae19527492fe427f047ea4` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `2fa3aed19ba4628890149f1025d08f9e639a1900781ab5f69f67d7ef8e76c658` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `6f8f3abcb725665b2a9560fedce41d98c9a532064e46f1fcad379e94d0678626` |
| `PioneerQwen35WorkImage.h` | `749be809b05f94edd764a14de13bbb8705b08d0ffa24240a3acbf703dbe1a9ee` |
| `PioneerQwen35WorkPeerImage.h` | `f2ae047af28da039a21b72f4feb556e6884bbae869c7542110e1667103874617` |
| Trial wrapper | `7f5a82a6ad845e4efd120fdcce24b5365ed50282013fd39352f4d67bcc22ecf0` |
| Post-power-cycle recovery script | `d1f9682a1f16cf11e9a368414b76566eadae44a9b8531dd3dc77dfc364d1959c` |

The ROOT image is 43,240 bytes of text; the peer image is unchanged from
tag231. Host token-core tests pass and the image build completed with the
undefined-import and unsupported-instruction checks enabled.

Remote trial stage: `/boot/home/pioneer-qwen35-work-232.Gc42x`.
