# Qwen3.5 dataflow matrix trace, tag0163

A guarded run started after the prior diagnostic exited and stable driver55
generation6 was verified idle. Authentication took 68.158975 s, vocabulary
setup 12.616024 s, model load 611.043880 s, and five-token prompt processing
19.239290 s. One subsequent decode took 4.142027 s (0.241428 decode tokens/s).
The output IDs `0,0` (`!!`) do not match verified `11751,13` (` Paris.`), so
this is a failed correctness diagnostic, not an acceptance throughput result.

The independently checked token0 embedding and first RMS samples still match.
At the first layer0 QKV projection, core0's final assigned group (rows48–63),
block0/panel0 packed activation words were `0xc015c03c` and `0xc02ec013`,
and integer matrix dot results were `1530` and `3395`; both pairs match the
independent offline reference. The sampled affine coefficients `0x3b3ef800`
and `0x3b106600` match too. The device activation scale `0xbd3f1666` is one
FP32 ULP from the offline reference `0xbd3f1665`; the reference deliberately
uses ordinary host arithmetic and is not a bit-exact device rounding proof.
The first and final projection output samples and final argmax score remain
zero. Thus the Tensix matrix produced nonzero results for this sample, while
the later accumulation or readback requires investigation; this does not
prove every matrix result is correct.

The source-window initialization and initial destination clear added in
tag0163 did not fix the observed zero output. The client reported
`unload_clean=1 state=2 active=0 cold=0 service_closed=1`; the guarded wrapper
restored stable driver55 generation8 without reboot and reported the output
mismatch. Independent checks confirmed no diagnostic process, owner or lock,
`cold0`, stable SHA-256
`041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403`, and both
models present at their recorded sizes; those are not fresh full hashes.
Exact logs and wrapper are under `evidence/`.
