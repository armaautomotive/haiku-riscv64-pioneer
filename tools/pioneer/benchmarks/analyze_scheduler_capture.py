#!/usr/bin/env python3
"""Summarize observed switch-to-switch CPU residence, not whole-run throughput."""
import collections
import csv
import re
import sys

if len(sys.argv) != 3:
    raise SystemExit("usage: analyze_scheduler_capture.py TRACE.csv THREAD_MASKS.txt")
with open(sys.argv[2]) as source:
    workers = {int(value) for value in re.findall(r"thread=(\d+)", source.read())}
if not workers:
    raise SystemExit("No worker IDs in snapshot")
last = {}
residence = collections.defaultdict(lambda: collections.defaultdict(int))
cpu_span = collections.Counter()
discontinuities = 0
with open(sys.argv[1], newline="") as source:
    for row in csv.DictReader(source):
        now, cpu, next_thread, previous, state = map(int, row.values())
        if cpu in last:
            before, running = last[cpu]
            if now < before or previous != running:
                discontinuities += 1
            else:
                elapsed = now - before
                residence[previous][cpu] += elapsed
                cpu_span[cpu] += elapsed
        last[cpu] = now, next_thread
if not cpu_span:
    raise SystemExit("No complete scheduling intervals; capture is empty or incomplete")
print(f"workers={len(workers)} discontinuities={discontinuities}")
print("thread observed_ms cpu:milliseconds")
for worker in sorted(workers):
    times = residence[worker]
    details = " ".join(f"{cpu}:{ns / 1e6:.3f}" for cpu, ns in sorted(times.items()))
    print(f"{worker} {sum(times.values()) / 1e6:.3f} {details}")
print("cpu observed_ms worker_ms other_ms")
for cpu, total in sorted(cpu_span.items()):
    work = sum(residence[worker][cpu] for worker in workers)
    print(f"{cpu} {total / 1e6:.3f} {work / 1e6:.3f} {(total - work) / 1e6:.3f}")
if discontinuities:
    raise SystemExit("Incomplete or inconsistent event sequence; do not treat as full accounting")
