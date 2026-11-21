# Tag0104 service-install attempt

Tag0104 installed from stable driver55 generation 18 at candidate generation
19. Model authentication took 67.340612 seconds, vocabulary setup/tokenization
took 13.844491 seconds, and the full 2,740,498,432-byte model upload completed.
Model binding and all-core route proof passed (`authority_bound=1`). The
client then attempted resident-service installation, but setup did not reach
generation. This client version did not print the service-install result;
tag0105 adds that result and state to stderr.

The client reported `unload_clean=1 state=2 active=0 cold=0
service_closed=1`. The wrapper restored stable driver55 at generation 20
without reboot. Inspection found a second nonzero peer-nonce guard in
`DataflowCoreIdleForHold`, which is used by service installation after bind.
Tag0105 removes the same redundant zero-nonce rejection there and retains the
exact `PioneerQ38DispatchPeerStatus` proof.

| File | SHA-256 |
| --- | --- |
| `state` | `507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e` |
| `work.exit` | `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865` |
| `work.stdout` | `ae0907bcf0fd0a236044db90ec088a07ff33ca76aa16fc1371888fbd41f45bc9` |
| `work.stderr` | `e717b27d097bfb90d241be8ae575f70e96203c401659f4b7b5d4713ccc73e84b` |
