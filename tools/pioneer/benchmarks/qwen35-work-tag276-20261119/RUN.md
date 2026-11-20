# Tag76 guarded hardware run

The required preflight passed before the card trial. The prior inference and
27B diagnostic processes were absent, stable driver55 was idle at generation
30, system health was normal, and the original 27B model file was intact.
Tag76 driver64 loaded at generation31 without reboot.

Authentication took `67.552273` s, vocabulary/tokenization took `13.395265`
s, and the 3,450 model spans (`2,740,498,432` bytes) loaded in `221.537886`
s. The first Work request faulted before prefill, so no warm output completed
and no prompt-processing or decode-rate measurement is available. The client
reported `unload_clean=1 state=2 active=0 cold=0`.

The peer fault detail was `0x230725d0`; the peer PC `0x725d0` maps to the
`FetchBlock` path in this tag76 image. Review showed that the Qwen35 prefetch
branch returned after starting its next-tile read but then fell through to
the legacy synchronous `FetchBlock` call in the common dispatch path. The
next candidate must omit that legacy fetch when the Qwen35 prefetch path has
already issued and proved the current tile. This correction is not included
in tag76.

The wrapper verified terminal fault recovery, unloaded the client, and
restored stable driver55 at generation32 without a reboot. The final status
was `cold0 active_owner0`; the client and trial processes exited. The 27B
model file remained `0:1:600:18973870528`.

Raw evidence is under `evidence/`. SHA-256:

| File | SHA-256 |
| --- | --- |
| `state` | `507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e` |
| `work.exit` | `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865` |
| `work.stdout` | `a0f7f5d5d86d13cf42a2dbaafcfff6c7d6c1abc452f5832f7dc9d4c4e5587904` |
| `work.stderr` | `26cb1e4acabe5587df0e278850dd0cc8af1e53a25f4e77121345b1dda8833f16` |
