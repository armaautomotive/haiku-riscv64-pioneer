# Qwen3.5 dataflow graph qualification, tag0161

One guarded run began after confirming that the previous diagnostics were absent
and stable driver55 was idle. The candidate was driver60 tag
`2026112100020161`. Model authentication took 100.791845 s, vocabulary setup
12.649542 s, and model loading 612.118705 s.

The graph completed prompt processing and generated two tokens. Prompt processing
for five tokens took 19.186788 s; the single subsequent decode took 4.130616 s,
or 0.242095 decode tokens/s. The generated IDs were `0,0` (`!!`), which do not
match the verified reference prefix `11751,13` (` Paris.`). This is a failed
correctness diagnostic; its raw rate is not a valid acceptance benchmark. The
result demonstrates that the graph passes the former convolution stall, but it
does not establish numerical correctness or useable model output.

The client reported `unload_clean=1 state=2 active=0 cold=0 service_closed=1`.
The wrapper restored stable driver55 without reboot and marked the run failed
due to the output mismatch. Independent checks found generation4 idle with no
lock, the expected stable driver hash, no diagnostic process and both model
files at their recorded sizes. The exact wrapper and unmodified output are in
`evidence/` (stderr is losslessly compressed). The rounding-independent SiLU
follow-up was developed after this candidate was built and was not used here.
