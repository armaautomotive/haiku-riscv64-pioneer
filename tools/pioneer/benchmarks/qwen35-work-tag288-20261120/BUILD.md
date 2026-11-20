# Qwen3.5-4B Work Tag88: ROOT `-O3` comparison

Tag88 keeps the tested Tag86 8 KiB double-buffered read schedule and changes
only the Work firmware optimization level: ROOT and its control-side objects
use `-O3`; peer images remain at `-Os` to respect their fixed 8 KiB descriptor
window. This isolates ROOT software scheduling and device-side control code
generation from Tag87's unsuccessful 16 KiB batch change.

The seven-bank weight placement, all-core projection bands, matrix engine
operations, ownership/source proofs, and Work-mode prohibition on CPU
inference or fallback remain unchanged. The two peer buffers are `0x98000`
and `0x9a000`, each limited to 8 KiB. The planner passes 5,136 plan cases and
checks exact ordinal arithmetic through 65,535 for each supported type.

The cross-build passed its fixed-image and disassembly gates. ROOT text is
87,148 bytes; peer text is 8,164 bytes, within the 8,192-byte limit.

| Artifact | SHA-256 | Size |
| --- | --- | ---: |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` | 4 B |
| ROOT image text | `ad1288c7bc052a24ac5ec795a5b3df7fab5d4a6265e2e03db0b287f35d8f4cc6` | 87,148 B |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` | 4 B |
| Peer image text | `d8f143a312ce44877526a88ae11256a8d493ce218be09ddf034d9f0c3c034a82` | 8,164 B |
| Candidate driver55 | `21d205423928228c0289f385355d5e17154df2d665b4b0cad53fec7a95009da5` | 246,992 B |
| Work client | `d8da309cdf9675c3e4e3610d2b6e6b99e0d7a519b091af26dadcf49d227d1c5d` | 304,848 B |

The guarded hardware result and exact timing/evidence will be recorded in
`RUN.md` after preflight, two warm runs, clean unload, and stable-driver restore.
