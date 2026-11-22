# Qwen3.5 dataflow graph qualification, tag0160

The single guarded run began with the prior diagnostics absent and stable
driver55 idle at generation2. Authentication took 99.817034 s, vocabulary
setup 13.006603 s, and loading 614.395995 s. The first prompt returned no token
after 30.931748 s. Worker0 reported graph step7 (layer0 recurrent convolution),
group0, stage7 (arithmetic); preceding QKV and weight reads completed. These
markers report progress, not output correctness or safe idleness.

The exact staged worker disassembly contains `fdiv.s` at `0x772fc` within
`DataflowSiluProduct`, which is called by this convolution. Existing BRISC
math avoids that instruction because it is not qualified on the worker. This
is a strong diagnosis of the observed stall, not proof until a corrected run
passes. The next rewrite uses the existing software divide and rejects floating
divide and square-root instructions in the built worker image.

Same-owner recovery remained pending and unload failed; the wrapper retained
driver60 generation3 with `cold1 active_owner1`. The unmodified run receipts,
qualification script, disassembly excerpt and guarded restoration script are
in `evidence/`. An authorized NVMe-path power cycle restored stable driver55 generation2,
with `cold0 active_owner0` and no lock. The installed stable-driver hash and
recorded 4B/27B file sizes were independently verified; no SD card operation
was used. No decode throughput was measured.
