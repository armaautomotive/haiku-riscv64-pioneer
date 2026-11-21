# Qwen3.5 dataflow rewrite tag0119 hardware run

Status: candidate built and its guarded trial/recovery scripts are prepared;
it has not been installed or run on the card. The last verified hardware
state is stable driver55 generation2, idle, with no inference process or
exclusive lock. The Qwen3.5-4B and preserved Qwen3.8-27B files retain their
recorded sizes. Tag0118's mismatch and recovery are recorded in
[`tag0118 RUN`](../qwen35-dataflow-tag0118-20261121/RUN.md).

Tag0119's next rewrite diagnostic is one prompt with a two-token output, with
no second warm generation and no throughput benchmark. If the read fails
again, the worker will report which selected completion counters matched,
which NoC initiators were active, and a queue-busy bitmask. This captures a
read-only snapshot; it must not modify counters, commands, or Tensix resets.

Do not start the two-warm benchmark until this diagnostic yields valid output
and the service unloads cleanly. Build details are in [`BUILD.md`](BUILD.md).
