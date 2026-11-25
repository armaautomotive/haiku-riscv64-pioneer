# Offline Pioneer pin-service kernel candidate

Two independent runs of `build_qwen35_pin_kernel.py` reproduced the saved
control kernel byte for byte and produced the same candidate kernel hash. The
manifest records source, pin-header, control, candidate and Jam-action hashes.
The candidate has a relocation from the built-in module table to the pin
service. It has not been installed, booted, or used on a card.

The saved kernel and its link objects are from the local
`/Volumes/HaikuBuildLocal/generated.riscv64` build. Jam came from Haiku
buildtools revision `33d46bf5c300bd34cd07c596431e4545c754ac03`. The build
compares the two changed kernel source files against the saved source mirror,
relinks the control using the original saved objects, and refuses to continue
unless the control matches the pinned SHA-256. The candidate recompiles only
those two translation units and retains the experimental pin flag. The
linker reported its existing orphan-section warnings; both links succeeded.

On the Pioneer, read-only inspection found `/boot/system/kernel_riscv64` hash
`38d4a4245db6a8ec4e2baa44c7705cdcdd5d4e66a8d43da97f3157699449a65b`.
That file has an ELF writable-segment offset of `0x194000`. `listimage 1`
reported the running kernel text at `0xffffffc000080000` and data at
`0xffffffc000215000`, a difference of `0x195000`, which matches the saved
control kernel's writable-segment offset. The matching offset is not proof
of the running kernel's hash. The actual boot artifact and rollback path
must be identified before a candidate boot. No reset, driver swap or model
inference was attempted; the idle/27B process check must be repeated
immediately before any card operation.
