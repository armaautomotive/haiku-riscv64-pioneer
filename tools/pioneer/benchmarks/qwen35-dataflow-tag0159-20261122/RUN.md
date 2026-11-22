# Qwen3.5 dataflow graph qualification, tag0159

One guarded correctness diagnostic ran after confirming the previous 27B and
inference diagnostics were absent, the exclusive lock was absent, and stable
driver55 was idle at generation2. The candidate was driver60 tag
`2026112100020159`. Authentication took 98.317830 s, vocabulary setup 13.184888 s,
and model loading 611.696275 s for 2740498432 resident bytes.

The first prompt produced no token after 30.932572 s. Worker0's independent
progress receipt reported step7, phase3 (execute): layer0 recurrent convolution.
Steps 0–6 are embedding, residual save, attention normalization and the four
QKV, gate, alpha and beta projections. The marker is diagnostic, so it does not
prove numerical correctness or safe device idleness. It did not identify the
operation within convolution. No decode time or tokens/sec was measured.

Settled-fault recovery returned result13 and unload failed. The candidate stayed
installed with driver60 generation3 `cold1 active_owner1` and the exclusive
lock retained. The captured stdout, compressed stderr, wrapper, and state are
in `evidence/`. One authorized NVMe-path power cycle restored stable driver55 generation2
with `cold0 active_owner0` and no lock. The installed stable-driver hash and
recorded 4B/27B file sizes were independently verified; no SD card operation
was used.
