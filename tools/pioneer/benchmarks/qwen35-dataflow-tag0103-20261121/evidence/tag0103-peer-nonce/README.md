# Tag0103 fleet-route preflight failure

The guarded run installed variant60 at generation 17 after verifying stable
driver55 generation 16 was idle. It authenticated the model in 68.289917 s,
tokenized the prompt in 13.264192 s, and transferred the full
2,740,498,432 bytes. Bind diagnostics reported:

```text
P100A_Q35_DATAFLOW bind_route_failure stage=12 core_or_bank=1
```

Stage 12 originally grouped the root hold check and peer checks. The failure
was the nonzero `lease.peerNonce[0]` predicate for core 1. Dispatch bootstrap
initializes every peer nonce to zero and leaves it zero until the first
dispatch; `PioneerQ38DispatchPeerStatus(..., nonce=0, stopped=false)` is the
existing exact proof of the initial idle peer state. The nonzero predicate
therefore rejected valid bootstrap state before that receipt check ran. The
tag0104 rewrite removes only that redundant nonzero condition; the exact
status words and waiting-PC proof remain required.

No service was installed and no output or decode timing was produced. The
client closed cleanly, and the wrapper restored stable driver55 at generation
18 without reboot. Raw state and logs:

| File | SHA-256 |
| --- | --- |
| `state` | `507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e` |
| `work.exit` | `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865` |
| `work.stdout` | `0ea2ea676518caffd156523d3f428eb2cd6b79cbacee5ac77ce96fedd5e903fc` |
| `work.stderr` | `55f0c948a36a51fa9ea99d41a262cc66af25ff33eb5e84efa27676a6e6c1dfcc` |
