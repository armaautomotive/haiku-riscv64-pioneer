# Qwen3.5 dataflow accumulator trace, tag0164

One guarded run began after the previous diagnostic exited and stable driver55
generation8 was idle. Authentication took 68.206893 s, vocabulary setup
13.467188 s, model load 610.548229 s, five-token prompt processing 19.384042 s,
and one subsequent decode 4.174285 s (0.239562 decode tokens/s). The generated
IDs `0,0` (`!!`) failed the verified reference prefix `11751,13` (` Paris.`).
This invalid-output diagnostic is not an acceptance throughput measurement.

As in tag0163, the sampled layer0 QKV input, source coefficient and Tensix dot
results were nonzero and matched the independent offline reference for token0.
Immediately after the first-panel vector accumulator, core0 readback samples
for Dst coefficient row96, minimum row100, and accumulator row108 were all
zero. These samples narrow the failure but do not by themselves prove whether
the metadata unpack, vector arithmetic, or readback was responsible. Source
review found the ordinary vector unpack left config73 at `3`, while the
qualified fixed-format SFPU metadata load assumes `0x13`. A separate source
review also found an overlap that overwrote minima during local staging. Both
fixes are prepared in tag0165 and have not yet been tested on hardware.

The client reported `unload_clean=1 state=2 active=0 cold=0 service_closed=1`;
the wrapper restored stable driver55 generation10 without reboot and reported
the output mismatch. Independent checks confirmed no diagnostic process, owner
or lock, `cold0`, stable SHA-256
`041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403`, and both
model files at their recorded sizes; the latter are not fresh full hashes.
Exact logs and wrapper are under `evidence/`.
