# Qwen3.5 dataflow graph fault and recovery, tag0165

After verifying the prior diagnostic had exited and stable driver55
generation10 was idle, a guarded tag0165 run authenticated the model in
68.192018 s, loaded the vocabulary in 13.542285 s, and loaded the model in
617.019131 s. Generation failed with zero output tokens after 1.339335 s of
prompt processing; there is no decode-throughput result. Core0 reported a
device graph deadline fault at step8 (the first recurrent delta step), without
an identified failed core. The precise cause has not been established.

Tag0165 included the affine-staging and metadata-selector corrections, but
still read SFPU floating-point output using the signed-integer conversion mode.
This makes its numeric inference invalid and means the outcome cannot validate
the corrected model. Tag0166 additionally fixes readback mode and remains the
next candidate.

The same-owner recovery completed without reboot: `recover_result result=17
state=3 recovery=0`, and the client subsequently reported `unload_clean=1
state=2 active=0 cold=0 service_closed=1`. The wrapper restored stable driver55
generation12 and reported the diagnostic failure. Independent checks found no
diagnostic process, owner or lock; `cold0`; stable driver SHA-256
`041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403`; and
both model files at their recorded sizes. Those size checks are not fresh full
hashes. Exact wrapper and logs are in `evidence/`. This is evidence for this
settled graph-fault recovery only, not proof that every device hang can recover
without rebooting.
