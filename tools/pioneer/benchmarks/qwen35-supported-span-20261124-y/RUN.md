# Qwen3.5 supported-counter preload qualification, stage y

The one-shot preflight verified idle stable55 generation2, both model hashes,
exclusive-lock absence and the staged candidate/recovery files. Driver60 tag
`2026112300030016` loaded at generation3. The exclusive client received a
failed first-bank upload receipt with `started=0`, `completed=0`, `nonce=0`
and `recovery=1`. The [serial trace](serial-trace.txt) reached resident-lease,
bulk-buffer and wide-buffer setup and entered span preparation, but did not
reach the first source-verification marker. The driver retained the owner
`cold1 active_owner1`; the wrapper retained the recovery lock. See [client
receipt](qualify.stderr), [wrapper output](wrapper.stdout), and [state](state).

In this exclusive-takeover build, `MapQ38Panel()` skipped all panel snapshot
allocation, while `PioneerQ38SpanPrepare()` requires the root snapshot before
it can proceed. The field begins null and has no other assignment in this
build. An extracted test of the actual mapping implementation fails on the
old code and passes after allocating one root preload snapshot; no per-worker
snapshots are introduced. The fix also preallocates the snapshot before device
reservation, allowing that allocation failure to return without touching the
card or retaining an owner. This is simulated evidence for that specific
failure path, not general no-reboot hardware recovery.

The staged recovery files matched their pinned hashes. A single authorized
OFF/ON cycle has been sent and Fedora boot is in progress; stable-driver
restoration is not yet confirmed. Tag0016 is consumed. No model output,
matrix projection or throughput was qualified.
