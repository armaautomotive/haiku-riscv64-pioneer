# Qwen3.5 dataflow graph numerical trace, tag0162

A guarded diagnostic began after verifying the prior diagnostic had exited,
the lock was absent and stable driver55 generation4 was idle. Model
authentication took 67.472414 s, vocabulary setup 12.822354 s, and model
loading 611.687363 s. Five-token prompt processing took 19.208158 s and one
subsequent decode took 4.134968 s (0.241840 decode tokens/s). The generated
IDs were `0,0` (`!!`), not the verified `11751,13` (` Paris.`). This is a
failed correctness diagnostic, not an acceptance throughput measurement.

The last graph input token was 0. Core0 embedding rows16 and 31 were exactly
`0xbd2f21c0` and `0x3cb4c800`, matching independently decoded samples from a
hash-matched GGUF prefix. The sampled layer0 RMS output rows16 and 31 were `0xc0887261` and
`0x4008147b`; an independent offline calculation from the verified token0
embedding and the layer0 norm-weight slice reproduced both exact bits. Run
`tools/pioneer/qwen35_token0_norm_reference.py` with the hash-checked 32 MiB
prefix and 10,240-byte layer0 weight slice. The sampled final RMS output was
nonzero. The sampled first QKV projection, gate
projection, and final tied-output projection were all zero, and the argmax
score bits were zero. Each sample covers one 16-value core0 output page, not
the entire tensor. This points to the projection path but does not prove which
part of it failed. Exact raw traces are in `evidence/work.stderr.gz`.

The client reported `unload_clean=1 state=2 active=0 cold=0 service_closed=1`.
The wrapper marked the output mismatch as failure, restored stable driver55
generation6 without reboot, and removed the lock. Independent checks confirmed
no diagnostic process or owner, `cold0`, stable driver SHA-256
`041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403`, and both
model files present at their recorded sizes; the size checks are not fresh
full-content hashes. Exact wrapper and output are in `evidence/`.
