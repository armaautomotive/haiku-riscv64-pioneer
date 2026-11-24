# Qwen3.5 preload snapshot fix, stage z

The one-shot preflight verified both full model hashes, idle stable55
generation2, absence of diagnostics and the exclusive lock, and pinned
candidate/recovery files. Restricted driver60 tag `2026112300030017` loaded
at generation3. The fixed client sent one 4096-byte strict preload to each of
the seven banks; every reply reported device completion, hold and exact host
readback. The [serial trace](serial-trace.txt) shows the root snapshot captured
and the image verified once, followed by seven completed, held uploads. The
[client receipt](qualify.stdout) reports `banks=7 pages=7 host_exact=1
clean_end=1`. The wrapper hot-restored stable55 generation4 without reboot;
the independent [postflight](postflight.txt) confirmed it idle, expected
driver/guard hashes, no lock or diagnostic process, and model metadata.

The physical test validates the supported-counter preload for seven fixed
4 KiB spans and clean teardown. It does not validate larger spans, the full
model, general fault recovery, Tensix projection, inference, or throughput.
Full post-run model hashes are running. Tag0017 is consumed. Driver SHA-256
`dde75fc1700b9c542ffc6b21aef8d6920d7ce27603ce984fe041782b21732011`;
client SHA-256
`696c41afab92c7808fe0a34ff4aa7ffeb3e4028e2149032a1479d2c80c803676`.
