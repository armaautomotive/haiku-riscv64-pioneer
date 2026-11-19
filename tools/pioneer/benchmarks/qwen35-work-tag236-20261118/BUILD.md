# Qwen3.5-4B Work tag236 diagnostic build

Built on 2026-11-18 with the local Haiku RISC-V64 toolchain using
`tools/pioneer/build_qwen35_work.py` and build tag `2026111800010036`.

Tag236 extends the tag235 failure-only dispatcher receipt. At the padded-scale
validation failure it records the raw scale bits and packed tensor type/input
width in ROOT progress reserved words 4 and 5, and the client prints those
context words in its terminal error record. The code changes only run on a
validation failure; no scale values are cleared and no acceptance condition
is weakened. This will distinguish an active scale outside its allowed range
from a nonzero padded tail and identify the tensor shape.

The Work owner regression test passed with a stale L1 entry checkpoint plus a
tagged failure receipt and context words. The image build passed its
undefined-import and unsupported-instruction checks. Tag236 was run on the
Pioneer after strict preflight. The run, exact setup timings, failure receipt,
artifact hashes, and recovery are in [RUN.md](RUN.md).

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55, tag `2026111800010036` | `0c60f8216c36e835e303324a2149a393df6c1f099e95c6c5f935d5284c438917` |
| Work client | `b85f8f47ea2c2882a5648140659b56fac63590086209fe0be3d9d91b6f37f570` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `856918601d3e2e221b19c17ef2192ebae09fe0fd181718f80dc24c3e39c4c424` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `feef2e513367a83f93c11142037c59e2a6af5aed1d7921d9cf031daa4fe63047` |
| Trial wrapper | `29a196a8c427659988ccc7d31210a4919c83bc3798ee9d6ce058562e5d3ff704` |
| Recovery helper (candidate tag check corrected) | `283fd9b05ae292d862a8b21a1fa17a61ad8d49a52e03826bcc2ee81ba66546bd` |

Offline build output: `/private/tmp/pioneer-qwen35-work-offline-236`.
The guarded trial wrapper uses staging directory
`/boot/home/pioneer-qwen35-work-236.Ux93m`.
