# Tag0110 hardware run

Tag0110 has not been run on the Pioneer. Per the work order, the rewrite is
being completed offline before any further benchmark attempt.

The last hardware run, tag0109, authenticated the model in `100.467732` s,
tokenized the prompt in `13.243073` s, and loaded `2,740,498,432` resident
bytes in `334.616263` s. Generation did not reach the model service, so there
is no prompt-processing time, valid output, decode timing, or tokens/s.
Tag0109's client and 27B diagnostic exited; its guarded recovery helper
restored stable driver55 generation2 and verified both model files unchanged.
See the tag0109 evidence before scheduling the next guarded trial.

The target remains two valid warm outputs, at least 100 decode tokens/s,
separate model-load/prompt/decode timings, and clean unload across all 120
Tensix cores.
