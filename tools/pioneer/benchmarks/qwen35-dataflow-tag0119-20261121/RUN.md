# Qwen3.5 dataflow rewrite tag0119 hardware run

Status: tag0119 was installed and run as one guarded two-token diagnostic. It
failed before producing output and left the candidate driver cold-latched;
guarded recovery is pending a fresh Haiku boot. The 4B and preserved 27B model
files were confirmed intact before recovery began. Tag0118's mismatch and
recovery are recorded in [`tag0118 RUN`](../qwen35-dataflow-tag0118-20261121/RUN.md).

The diagnostic used one prompt with a two-token output. Authentication took
100.664302 s, vocabulary/tokenization 13.757776 s, model load 333.142334 s,
and prompt processing failed after 0.615593 s. Decode time was 0 s because no
output token was produced. The failing embedding read at stage 7/index 36
observed `0x00029066`, expected `0x00029067`. The eight-slot completion-counter
match mask was `0xfe` (slot 0 did not match); the four-initiator busy mask was
`0x0`; the 32-entry outstanding transaction-ID mask was `0x00000001`. The
captured tag0119 client printed this last field as
`failed_read_queue_busy_mask`; that label was incorrect. In the official
Blackhole map, completion-counter index 36 corresponds to NoC0's
`NIU_MST_RD_RESP_RECEIVED`; its expected increment did not occur, while the
command-accepted and read-request-sent counters matched. Bit 0 of the other
mask is outstanding transaction ID 0. This shows a request was sent and its
response was not received by the initiating NIU, but does not establish
whether the target NIU served it or the return path failed. The snapshot is
read-only; no counters, commands, or Tensix resets were modified by the
diagnostic. See the official
[Blackhole NIU counter documentation](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/Counters.md).

The trial exited with result 31, stage 4, state 2, recovery required, and zero
output. Clean close failed. The candidate remained `driver60`, tag
`2026112100010119`, generation 3, cold-latched with `active_owner=1`, and the
exclusive recovery lock plus stable-driver backup were retained. The wrapper
did not overwrite the stable backup. Before the recovery reboot, the model
files were still 2,740,937,888 bytes (4B) and 18,973,870,528 bytes (27B), and
the process list showed no inference process.

Haiku accepted a single `shutdown -rs` request for guarded recovery. The SSH
session then closed as expected, but the pinned Linux profile was not yet
reachable. Repeated checks against both pinned Linux and Haiku profiles timed
out; ARP for `10.0.0.247` remained incomplete. The user reported an accidental
power reset during recovery. No relay button action was sent, because the
current board power state could not be confirmed from SSH/ARP alone. The
post-reboot stable-driver state still needs to be verified and recorded.

Do not start the two-warm benchmark until the read failure is fixed, a
diagnostic yields valid output, and the service unloads cleanly. Build details
are in [`BUILD.md`](BUILD.md). Raw run and pre-reboot state captures are in
[`evidence/`](evidence/).
