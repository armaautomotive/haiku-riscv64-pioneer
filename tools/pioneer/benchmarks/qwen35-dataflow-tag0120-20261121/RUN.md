# Qwen3.5 dataflow rewrite tag0120 hardware status

Status: tag0120 has a correctly tagged offline build and guarded one-run / post-
power-cycle recovery scripts, but it has not been installed or run on the
Pioneer. It is a diagnostic-only rewrite checkpoint, not a model run or
performance result.

Tag0119's one-run diagnostic failed before output. Its read-request counter
advanced, but the `NIU_MST_RD_RESP_RECEIVED` counter did not; outstanding
transaction ID 0 remained set. The tag0119 log named this mask as a busy queue,
which was inaccurate. Tag0120 corrects the name and preserves the failed
embedding read's remote address, route, and size in the existing 64-byte
mailbox. See [`tag0119 RUN`](../qwen35-dataflow-tag0119-20261121/RUN.md).

After the tag0119 trial, Haiku accepted a reboot request. Both pinned SSH
profiles later timed out, the ARP entry stayed incomplete, and the existing
serial console stopped at shutdown-device teardown without a new boot banner.
The user reported an accidental power reset during this recovery window. At
that check, no relay action was sent while the current board power state
remained unclear.
The tag0119 candidate's recovery lock and stable-driver backup were retained;
stable driver55 generation2 must be verified restored before another hardware
command.

Latest recheck (2026-11-21): both saved, host-pinned SSH profiles time out;
the serial console still ends at shutdown-device teardown with no fresh boot
banner. Relay status was power-button contact open and SD detached. After the
shutdown/reset window, one ON pulse was sent; the relay is released, but after
60 seconds SSH still times out, ARP is incomplete, and serial output is
unchanged. There is still no confirmed boot, so no further power pulse was
sent. Stable-driver recovery and the tag0120 hardware diagnostic remain
pending.

No valid output, prompt completion, decode time, warm run, clean unload, or
throughput result exists for tag0120. The 100 decode tokens/s goal remains
unmet, and assistant integration remains deferred. Build hashes and offline
validation are in [`BUILD.md`](BUILD.md). The runtime-tagged one-run and
recovery scripts are present beside this file; wait for a verified Haiku boot
and stable-driver recovery before staging them.
