# Qwen3.5-4B actual-model preload qualification, stage aa

The one-shot preflight verified the pinned full hashes of both models, stable
driver55 generation4 idle, the absence of the exclusive lock and diagnostic
processes, and the candidate and recovery files. Restricted driver60 tag
`2026112300030018` was hot-loaded at generation5. The client authenticated
the actual Qwen3.5-4B Q4_K_M GGUF and uploaded 2,740,498,432 bytes through
the strict span path; each completed span required device completion and exact
readback. The client reported `clean_end=1` after ending the span and bank
sessions. Its measured load time was **507.263910 seconds**; this includes
the bank session and upload, but excludes authentication and is not inference.

The [client receipt](qualify.stdout), [progress](qualify.stderr), and
[wrapper output](wrapper.stdout) show success. The wrapper restored stable55
generation6 without reboot. Independent [postflight](postflight.txt) showed
the expected stable driver and guard hashes, `cold0 active_owner0`, no lock or
inference/diagnostic process, and unchanged model metadata. Both full
[post-run model hashes](post-model-hashes.txt) matched their pinned values.

This qualifies actual-model preload and clean teardown only. It does not prove
a projection, model inference, warm outputs, decode throughput, arbitrary fault
recovery, or all-core execution. No token rate was measured. Tag0018 is consumed.
The tested driver SHA-256 was
`0dcabe55d701e1288ad97fb3a8ac61500b9d1b09b804582c36303de644973109` and
client SHA-256 was
`dfb25a19e906bf7031e5dbb22ce093edff12b5cd672cd2dcbd2f09d2ce016842`.
