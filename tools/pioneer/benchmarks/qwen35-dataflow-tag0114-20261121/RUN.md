# Qwen3.5 dataflow rewrite tag0114 hardware run

The guarded trial loaded the complete Qwen3.5-4B model, then the first prompt
embedding failed on core0 with worker failure7 (`EmbeddingWeightRead`),
phase5. Authentication took `99.340053` s; vocabulary load and prompt
tokenization took `13.168584` s; model load took `329.758232` s and uploaded
`2740498432` resident bytes. The partial prompt operation took `0.609034` s.
No output token was produced, so decode tokens/s is unavailable. The second
warm run was not started.

QUERY/RECOVER stopped at runtime audit stage6/core0/detail11; CLOSE failed its
core0 stop audit. Detail11 is the resident NoC command/queue idle check, not a
Tensix matrix-engine idle check. The wrapper retained the cold candidate, exact
stable55 backup, and lock. The user then accidentally reset power, but the
Haiku status still reported candidate generation3 `cold1 active_owner1`, so
that reset did not clear the fault. The authorized host reboot reached the
pinned Linux recovery environment; the stable55 restore is pending the
SD-free Haiku handoff. Model files remain in place.

Raw trial output and post-failure state are in `evidence/`. This is a failed
diagnostic run, not a model performance result.
