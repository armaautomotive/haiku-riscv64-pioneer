# One-worker local-RAM restore qualification, stage t

The restricted tag `2026112300030011` candidate ran after both full model
SHA-256 checks, an idle stable55 generation-4 preflight and verification of the
staged files. The wrapper was run once. It captured the images of all 120 held
workers, staged the probe on worker zero and verified the other workers' saved
images. With all TRISCs held, worker zero's B and NC copied and compared their
own private RAM against L1 copies. Each qualification shim deliberately changed
one local word and then ran the exact relocated local restore code, which
restored and verified the saved private RAM before publishing its receipt and
parking. The host checked both parked PCs, held the roles and verified the L1
copies. It restored the handoff, reset-preservation setting and original fleet
images; mappings closed and the bank session ended cleanly. See the
[serial phases](serial-phases.txt), [wrapper output](wrapper-output.txt), and
[independent postflight](postflight.txt).

The wrapper returned success and hot-reloaded stable55 without a reboot. The
independent postflight observed generation 6 idle (`cold0 active_owner0`), the
expected stable driver hash, no model or 27B diagnostic process, no exclusive
lock and expected metadata for both model files. Both full hashes matched before
the trial; a separate postflight content-hash check was started afterward.

This is a restricted one-worker restore qualification, not a complete resident
service, a proof of firmware exclusion, recovery from a real worker fault, or
model inference. It does not establish no-reboot fault recovery or decode speed.
The tested candidate driver SHA-256 was
`28f938ef92e680ee3d51092b379af828925cdaa939538ee511597c8a452d13e1` and client
SHA-256 was `67b0e73d0d2bb4a86ff8537179ef598e87e741d8038e70e4a60fe4fc2d547c02`.
The final offline dataflow and reload suites passed, and an independent
cross-build reproduced both binary hashes.
