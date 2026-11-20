# Tag98 P100A hardware result

Tag98 started from stable driver55 generation12 after its guarded idle/model
preflight. It installed candidate driver64 Tag0098 at generation13 without a
reboot, then safely restored stable driver55 at generation14.

## Result

The model authentication took 68.100968 s. Tokenization took 12.756641 s for
the 23-token prompt. Resident model loading took 227.112987 s across 3,450
spans and 2,740,498,432 bytes. Work reached 22 commits and completed 23/23
prompt tokens before the exact ROOT software flit-counter proof failed. The
Tag98 receipt identifies counter index2 (flits): expected 87,690
(`0x1528a`), actual 71,266 (`0x11662`), a difference of 16,424. The final
worker119 snapshot was complete at sequence `0xda9`.

The client exited 1 and produced no valid output, completed prompt-processing
measurement, or decode-rate measurement. Both warm runs and the 100 decode
tokens/s target remain unmet. The client unloaded cleanly (`state=2`,
`active=0`, `cold=0`). Postflight confirmed stable driver55 Tag2026102100010012
generation14, idle and with no reboot.

This checkpoint preserves the counter evidence; it does not paper over or
relax the failed exact counter proof. The `expected-actual` gap is now
reproducible from the raw log and is the input to the next implementation
step.

## Evidence

Raw card files and postflight output are in `evidence/tag98-card/`.

| File | SHA-256 |
| --- | --- |
| `state` | `507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e` |
| `work.exit` | `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865` |
| `work.stdout` | `1c2c3fe5cc357053954b99672fc3149ecd44468dcb61badeb08443ca8ba860ae` |
| `work.stderr` | `f7d03e25fd02dd6dfb13b398ae1f4ecaeb7b178a08e50f6ff0c1c705a52ad9b7` |
