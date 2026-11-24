# Qwen3.5 supported-counter preload qualification, stage w

The wrapper checked the idle stable driver, absence of a diagnostic owner and
exclusive lock, pinned files and both full model hashes. Restricted driver60
tag `2026112300030014` loaded at generation 11. Its fixed-transfer client
returned 1 without a success receipt or diagnostic output. The wrapper
quiesced it and hot-restored stable driver55 at generation 12 without a
reboot. A separate postflight confirmed the stable-driver and guard hashes,
idle `cold0 active_owner0`, the missing exclusive lock and no Qwen/27B
diagnostic processes. See [wrapper stdout](wrapper.stdout), [wrapper
stderr](wrapper.stderr), [client exit](qualify.exit) and [state](state).

The client opened the rewrite driver without `O_EXCL`, while the actual
`BeginResidentSession` requires the exclusive cookie for bank sessions. This
is a concrete client defect and a likely explanation, but the client emitted
no stage information, so this run does not prove where it failed or whether
any transfer was attempted. No transfer, model inference, general fault
recovery or decode speed was qualified. Tag 0014 is consumed; do not rerun it.
The next candidate uses a fresh tag, exclusive-open verification and
stage-specific error output. Post-run full model hashes are being checked.
