# Tag95 P100A hardware result

Not run yet. Before installing Tag95, the guarded preflight must confirm that the prior 27B diagnostic and inference processes have exited, the stable driver55 is idle, the 27B model identity is unchanged, and the pinned Qwen3.5-4B Q4_K_M model is present.

The trial must produce two exact warm outputs, report model load and prompt processing separately from decode tokens/sec, verify participation of all 120 cores, unload cleanly, and restore stable driver55. A successful build or offline peer-image replay does not count as hardware or throughput qualification.
