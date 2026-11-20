# Qwen3.5-4B Work Tag87: 16 KiB double-buffered reads

Tag87 expands the double-buffered original-weight read slots to 16 KiB while
keeping all 120 Tensix cores on the projection path. The seven-bank Work
layout and existing ownership, source-seal, idle/response, byte-count, and
matrix checks remain in force. No CPU inference or fallback is enabled.

The peer alternates buffers at `0x98000` and `0x9c000`; both end at the
activation boundary `0xa0000`. Batch sizes are 28 Q8 tiles (16,128 bytes),
7 Q4_K tiles (16,128 bytes), and 4 Q5_K or Q6_K tiles (11,264 or 13,568
bytes). Each read starts before the current batch's matrix work and is waited
on only when that alternate buffer becomes current. The ROOT's private final
batch proof still uses its own `0x98000` scratch.

RV32 division remains disabled. For the bounded shapes, Q8 ordinals use
`block*37 >> 10` (checked through 107), Q4_K uses `block*37 >> 8` (checked
through 86), and Q5_K/Q6_K use `block >> 2`. The planner test passes 5,136
coverage and refusal cases. The cross-build passed the ROOT and peer image
layout/disassembly gates; the peer image is 8,176 of 8,192 bytes, leaving
16 bytes before the fixed descriptor. Two earlier builds exceeded the peer
limit; the bounded ordinal implementation brought it under the limit without
changing the descriptor or memory map.

| Artifact | SHA-256 | Size |
| --- | --- | ---: |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` | 4 B |
| ROOT image text | `35e0bb36a547a5114dc08cbc467c7b89895a2aaf9cb8d2a5995b820106f0f98a` | 51,028 B |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` | 4 B |
| Peer image text | `51dc333dab0d54f225f4afe2b2b1bf6e6419d1a3985e18136b4ed11999b0dbb7` | 8,176 B |
| Candidate driver55 | `db3cf5244a70a53f33c8a7fa0c162e5755e7247351df60b026f54a80689cc0c5` | 246,992 B |
| Work client | `d8da309cdf9675c3e4e3610d2b6e6b99e0d7a519b091af26dadcf49d227d1c5d` | 304,848 B |

The client binary is unchanged from Tag86 because the read scheduling lives in
the fixed Tensix images and driver. The hardware result and restoration record
will be added to `RUN.md` after the guarded trial.
