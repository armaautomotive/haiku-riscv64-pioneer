# Hybrid FlashDecode scheduling, offline, 2026-11-25

The existing temporal partition leaves workers with no position at short
contexts. The guarded hybrid assignment divides each KV head's 30 workers
between disjoint position ranges and disjoint 16-lane output blocks across
its four query heads. The position partition count is the largest divisor of
30 not exceeding the number of positions; the remaining factor partitions the
output. Thus every one of 120 workers has a nonempty position interval and a
nonempty output interval, including at position zero. A shard can cross a
query-head boundary, and the range helper returns each head intersection.

For positions 0 and 1 the partitions are respectively 1-by-30 and 2-by-15
per KV head. At position 29 and beyond they are 30-by-1: a logical K/V entry
can be shared among four query heads by its assigned worker. This counts
logical references in the test, not actual NoC or DRAM traffic. The scheduler
has no connected worker or transport and does not establish a speedup.

`sh tools/pioneer/tests/run_pioneer_qwen35_flash_decode_partial_tests.sh`
passed with ASan/UBSan. It checks complete, nonoverlapping position/output
coverage and nonempty assignments at the transition boundaries and maximum
position. Synthetic attention at positions through 127 exercises the hybrid
partial reductions and head-boundary slices; maximum absolute difference from
the sequential online result was `2.38418579e-7`. The preexisting double
reference was also checked for the original temporal reduction. A guarded
RV32IMF target wrapper for assignment and lane range compiled with
warnings-as-errors; each reported a 16-byte frame, not a whole-program stack
bound. The unguarded and qualification builds remain refused.

No device kernel, inter-core partial transfer, visibility/synchronization,
resident graph integration, physical test, output or throughput was proved.
The separate full-model simulator run was still active at this checkpoint.
