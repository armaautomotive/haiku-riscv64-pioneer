# Held-fleet inspection, stage m

On 2026-11-23, the separately tagged `2026112300030004` restricted driver
performed the held-fleet inspection in an empty bank session on the Pioneer
in Haiku. Immediately beforehand, the wrapper checked that no model or
diagnostic was running, stable driver55 generation 4 was idle, the exclusive
lock was absent and both model files had their expected metadata.

All 120 dedicated leases were prepared. The ARC L2CPU reset sample, two
held-fleet passes and the host-DMA observation completed. Temporary routes
were restored and explicit BANK END succeeded. The wrapper hot-reloaded
stable55 at generation 6 without a reboot. Independent postflight observed
`cold0 active_owner0`, no matching model or diagnostic process, no exclusive
lock and the expected 4B and 27B file ownership, mode and sizes. It did not
recompute either file's content hash. The receipt is in
[`qualify.stderr`](qualify.stderr).

The preceding [stage-l run](../qwen35-fleet-inspect-20261123-l/RUN.md)
failed the host-DMA observation. This candidate corrected the source check
to compare the root NIU with the owned active lease, including its enabled
bit. The successful repeat supports that diagnosis. This is a point-in-time
inspection: it does not prove that firmware or another agent cannot write
later, nor does it qualify resident role image staging, launch, inference or
fault recovery. No model was loaded, no worker program was launched, and no
decode throughput was measured.

Driver SHA-256:
`fb891e48d4b32b8aeb83a2c5327a9bdc0cac7e9d2e9b2d51fe99da80f4905df6`.
Client SHA-256:
`4e890c6ed33ebeeebb07b6259e78343b722b9de36579e3e8635dbcbdbefad3dd`.
