# Tag79 guarded hardware result

The wrapper verified the prior diagnostic had exited, stable driver55 was
idle at generation36, and the preserved 27B model was
`0:1:600:18973870528`. Tag79 hot-reloaded as driver64 generation37 without a
reboot. It loaded the verified 4B model and completed two exact warm outputs:
`A B C D E F G H I J K L M N O P`.

Authentication took 68.126302 s, vocabulary/tokenization took 12.441510 s,
and model load took 225.966251 s for 3,450 spans / 2,740,498,432 bytes.
Prompt processing and decode are reported separately:

| Warm run | Prompt tokens | Prompt seconds | Decode tokens | Decode seconds | Decode tok/s |
| --- | ---: | ---: | ---: | ---: | ---: |
| 1 | 23 | 180.618303 | 16 | 130.825677 | 0.122300 |
| 2 | 23 | 180.623429 | 16 | 130.820073 | 0.122305 |

Each run reported 5,920 full-core waves and 11,696 projection waves, showing
the ROOT plus all 119 peers participated. Tag77 reported 14,128 projection
waves per run; Gate+Up fusion reduces those launches, but decode throughput
did not materially improve. The current rate is about 818 times below the
100 tok/s goal.

The client reported `unload_clean=1 state=2 active=0`. The guarded wrapper
restored stable driver55 idle at generation38 without reboot. The 27B model
remained intact. Raw outputs, state, and checksums are in `evidence/`.
