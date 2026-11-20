# Tag77 guarded hardware run

Before the run, the process list contained no inference or 27B diagnostic,
stable driver55 was idle at generation32, and the preserved Qwen3.8-27B model
was `0:1:600:18973870528`. Tag77 driver64 loaded at generation33 without a
reboot. It loaded 3,450 model spans (`2,740,498,432` bytes).

Authentication took `67.610783` s, vocabulary/tokenization took `13.139845`
s, and model load took `221.389018` s. Prompt and decode measurements are
separate:

| Warm run | Prompt tokens | Prompt seconds | Decode tokens | Decode seconds | Decode tokens/s |
| --- | ---: | ---: | ---: | ---: | ---: |
| 1 | 23 | 183.025978 | 16 | 132.393194 | 0.120852 |
| 2 | 23 | 183.034334 | 16 | 132.383380 | 0.120861 |

Both outputs were exactly `A B C D E F G H I J K L M N O P`. Each run
reported 7,136 full-core waves and 14,128 projection waves, confirming use of
the full 120-core ROOT-plus-119-peer Tensix layout. The 100 tok/s goal remains
about 827 times above the measured rate.

The tile lookahead removed the tag76 fault but barely changed throughput.
The dispatcher still measured about `55.8001` s in issue plus ROOT matrix
work, `96.7523` s in peer completion/proof, and `6.2346` s in result/final
proof per run. The measured decode rate is roughly 0.25% above tag75's
0.12055 tok/s and does not represent a meaningful speedup. The next change
must address repeated per-wave dispatch and return of intermediate results,
not only weight-read overlap.

The client reported a clean unload (`state=2 active=0 cold=0`); the wrapper
restored stable driver55 idle at generation34 without reboot. The 27B model
remained intact. Raw preflight, workload, and postflight evidence is in
`evidence/`.

| File | SHA-256 |
| --- | --- |
| `state` | `507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e` |
| `work.exit` | `9a271f2a916b0b6ee6cecb2426f0b3206ef074578be55d9bc94f6f3fe3ab86aa` |
| `work.stdout` | `81ade518bafb98214e11dcca591a051f544f080b60bca440b74bc0e4865592bc` |
| `work.stderr` | `a76737e27f6e6f85a7ec9bae5ea1bc58bdc869bfd840da641922d3d64000f24f` |
| `preflight.txt` | `1eea36d9ee06f85a7cd77febf5f7b21efdbef92eae72f57b34239a7ecf0d99a5` |
| `postflight.txt` | `f4f9957a9c27e87613e250dc894ddc04fed9b26c9dccb5a5446fa177d6131605` |
