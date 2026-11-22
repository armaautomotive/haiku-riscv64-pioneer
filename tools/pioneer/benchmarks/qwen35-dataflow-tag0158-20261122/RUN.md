# Qwen3.5 dataflow graph qualification, tag0158

One guarded correctness diagnostic was run on Pioneer after verifying the prior
process had exited, the exclusive lock was absent, and stable driver55 was
idle at generation 9. The candidate was driver60 tag `2026112100020158`.
The diagnostic did not produce a token. Model authentication took 68.239563 s,
vocabulary setup 13.278265 s, and model loading 609.601535 s. Prompt execution
stopped after 8.258036 s; no decode timing or throughput was obtained.

The host timed out waiting for worker 0, which was still active; the available
snapshot did not contain a graph step or RMS detail. Thus this test does not
establish whether the scale-page change completed normalization. The attempted
settled-graph recovery returned result 13, and unload did not complete. The
wrapper retained the candidate and the lock, reporting driver60 generation10
with `cold1 active_owner1`. See the unmodified compressed stderr and other
receipts in `evidence/`. One authorized power cycle and the existing NVMe Fedora-to-Haiku kexec path
restored stable driver55 generation2, with `cold0 active_owner0` and no lock.
The stable driver hash and recorded 4B/27B file sizes were independently
verified. No SD card operations were involved.
