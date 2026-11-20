# Tag97 P100A hardware result

Not run yet. Tag96's guarded trial restored stable driver55 idle at generation
10, so the next guarded run must use generation10 as its starting state. The
runner accepts only the matching Tag97 driver and client hashes in
[`BUILD.md`](BUILD.md), verifies the 4B and 27B model identities, and restores
stable driver55 after completion or safe fault recovery.

The run needs two valid warm outputs, all 120 Tensix cores, separate model-load
and prompt-processing timings, decode tokens/s, clean unload, and at least 100
decode tokens/s to qualify. A successful bounded-poll fix or a safe recovery
alone does not qualify the model.
