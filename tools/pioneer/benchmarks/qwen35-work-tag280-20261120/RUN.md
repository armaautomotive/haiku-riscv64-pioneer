# Tag80 guarded hardware result

The preflight verified the previous run was gone, stable driver55 was idle at
generation38, and the preserved 27B model was `0:1:600:18973870528`. Tag80
hot-reloaded as driver64 generation39 without a reboot and completed two exact
warm outputs: `A B C D E F G H I J K L M N O P`.

Authentication took 67.573524 s, vocabulary/tokenization took 13.956553 s,
and model load took 221.677009 s for 3,450 spans / 2,740,498,432 bytes.
Prompt and decode timings are separate:

| Warm run | Prompt tokens | Prompt seconds | Decode tokens | Decode seconds | Decode tok/s |
| --- | ---: | ---: | ---: | ---: | ---: |
| 1 | 23 | 176.897236 | 16 | 128.402747 | 0.124608 |
| 2 | 23 | 176.903661 | 16 | 128.396375 | 0.124614 |

Each run reported 5,920 full-core waves and 10,366 projection waves. The 119
peers plus ROOT all participated. Tag79 used 11,696 projection waves per run,
so Tag80 reduced waves by about 11.4%; decode throughput rose about 1.9%.
Peer wait/proof remained about 98.799 billion cycles per run, nearly unchanged
from Tag79's 99.126 billion cycles. Repeated peer completion/proof remains the
largest measured dispatch cost. The 100 tok/s target is still about 802 times
above the measured decode rate.

The client reported `unload_clean=1 state=2 active=0`. The wrapper restored
stable driver55 idle at generation40 without reboot. The 27B model remained
intact. Raw outputs, state, and checksums are in `evidence/`.
