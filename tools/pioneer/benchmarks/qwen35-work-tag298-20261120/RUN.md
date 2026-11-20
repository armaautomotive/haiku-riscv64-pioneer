# Tag98 P100A hardware result

Not run yet. Tag97 restored stable driver55 idle at generation12. Tag98's
guarded runner must start from that generation, confirm no diagnostic or
inference process is active, verify both model identities, and restore stable
driver55 after the run or a safe fault recovery.

Tag98 adds failure-only expected/actual values for the ROOT's exact software
transfer-counter proof. It does not change the proof or qualify output. The
hardware run still needs two valid warm outputs on all 120 Tensix cores,
separate load, prompt, and decode measurements, clean unload, and at least 100
decode tokens/s.
