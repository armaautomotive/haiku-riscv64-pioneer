# Qwen3.5 supported-counter preload qualification, stage x

The wrapper verified stable55 generation12, exclusive-lock absence, staged
recovery files and both full model hashes before loading restricted driver60
tag `2026112300030015`, generation13. The corrected client obtained exclusive
access and reached the first bank's strict span upload, but returned
`failed_stage=span_upload bank=0`. No successful transfer receipt was emitted.
The driver reported `cold1 active_owner1` after the client closed; the wrapper
kept the exclusive lock and state `QUALIFICATION_FAILED_RETAIN_FOR_INSPECTION`
instead of unloading. See [client stderr](qualify.stderr), [wrapper
stdout](wrapper.stdout), [wrapper stderr](wrapper.stderr), [state](state) and
[serial driver evidence](serial-driver-evidence.txt).

The current qualification routes no recovery ioctl, and its retained owner
cannot be retired by normal hot reload. The pre-staged recovery script and
both stable-driver copies matched their expected hashes. A single authorized front-panel OFF/ON cycle and the pinned Fedora NVMe-to-Haiku handoff were used. The staged script restored stable driver55 generation2; independent postflight confirmed idle, the expected stable/guard hashes, no lock and no diagnostic process. See [Linux handoff](linux-handoff.txt), [restore](restore.txt) and [postflight](postflight.txt). Both full [post-recovery model hashes](post-model-hashes.txt) matched the pinned values. Do not reuse tag0015. This does not establish whether device
execution started, the underlying transfer failure, inference correctness,
no-reboot recovery, or throughput.
