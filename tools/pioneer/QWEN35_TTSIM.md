# Qwen3.5 rewrite on the Blackhole simulator

The simulator can execute the compiled Tensix service and role instructions
without accessing the Pioneer. It does not establish physical ownership,
recovery, inference correctness on the P100A, or hardware performance.

The checked simulator source was Tenstorrent `ttsim` commit
`71a685c91f864223682680df021f779042f7135d`. On this macOS arm64 host it
builds with `python3 make.py src/_out/bh/libttsim.so`. Use the sharing-enabled
non-loadable driver build to generate the two image directories, then run:

```sh
python3 -B tools/pioneer/tests/pioneer_qwen35_ttsim_service_test.py \
  /path/to/ttsim/src/_out/bh/libttsim.so \
  /path/to/build/qwen35-dataflow-service-image \
  /path/to/build/qwen35-shared-role-images
```

The test stages the actual images, checks the private-RAM snapshot handoff,
and starts and stops BRISC, NCRISC and TRISC0 on 120 simulated workers. The
simulator starts from its own reset state; this is not evidence that the
physical card's existing firmware, caches, or RAM can be taken over safely.

The distinct resident graph image can also be checked with synthetic graph
templates. Build the fixture and use a new output path for each file:

```sh
c++ -std=c++17 -O2 -Wall -Wextra -Werror \
  tools/pioneer/tests/pioneer_qwen35_ttsim_resident_fixture.cpp -o /tmp/q35-resident-fixture
/tmp/q35-resident-fixture /tmp/q35-resident-templates.bin /tmp/q35-resident-command.bin
python3 -B tools/pioneer/tests/pioneer_qwen35_ttsim_service_test.py \
  /path/to/unmodified-libttsim.so /path/to/build/qwen35-resident-service-image \
  /path/to/build/qwen35-shared-role-images --resident-templates /tmp/q35-resident-templates.bin
```

This uses the sharing-enabled fused-recurrence image and checks initialization
and clean idle stop of its compiled resident entrypoint on all 120 simulated
workers. Add `--resident-corrupt-core 67` to corrupt one template: that worker
must retain a startup fault while the other services and all reader/writer
roles stop. Neither test starts a model graph.

For a graph fault test, use the simulator with the zero-upper-length patch
described below, the same resident image and fixture, and add
`--resident-fault-command /tmp/q35-resident-command.bin`. The test explicitly
zeros each synthetic stream's first record and submits one compact command to
the root. The device's resident fanout reaches all 120 workers, which report
and retain the expected first-record fetch fault; the root must not publish a
completion candidate. Upstream `ttsim` stops at the unsupported upper-length
register in this path. The test proves neither successful graph execution nor
physical fault recovery, and it does not exercise model weights or performance.

An authenticated model fixture exercises six dependent first-layer graph
steps: embedding, residual save, attention RMS, fused QKV/Alpha/Beta, the
recurrent gate projection and the recurrent convolution.
Use a new empty directory and the corrected resident image built after the
SrcA handoff fix described below:

```sh
c++ -std=c++17 -O2 -Wall -Wextra -Werror \
  tools/pioneer/tests/pioneer_qwen35_ttsim_resident_sequence_fixture.cpp \
  tools/pioneer/PioneerModelWeightCache.cpp -o /tmp/q35-resident-sequence
/tmp/q35-resident-sequence /path/to/Qwen3.5-4B-Q4_K_M.gguf /path/to/empty-directory 0
python3 -B tools/pioneer/tests/pioneer_qwen35_ttsim_service_test.py \
  /path/to/experimental-libttsim.so /path/to/build/qwen35-resident-service-image \
  /path/to/build/qwen35-shared-role-images --resident-sequence-fixture /path/to/empty-directory
```

The fixture authenticates the full model and stages token 42's Q6_K embedding,
the first layer's F32 attention norm and convolution weights, and its full
Q5_K/Q8_0 and Q4_K gate projections. It uses the production binders, triple
binder, sharing planner and stream encoder on a synthetic one-bank layout.
The compiled resident graph fetches the six records, executes them on all 120
workers, and completes a device barrier after each. A deliberately invalid
seventh record terminates the run; all workers must report exactly six
completed steps, and no token completion candidate may appear. Independent
references check embedding, residual, RMS, QKV, Alpha, Beta, gate and convolution
outputs, including Q8_K and Q8_0 activation quantization. The output of the verified simulated RMS operation is used as
the projection reference input. This is not the full graph recorder, a full
model result, physical recovery, or a speed measurement. Matrix execution
requires both experimental simulator patches below; their instruction models
are not independently qualified.

The five- and six-step runs used compatible NoC-response role images (other role
builds can fail simulator startup or access an unsupported register),
`/private/tmp/q35-ten3868-final-resident-image`, and the matrix-exploratory
`libttsim.so` SHA-256 `1dbc4a6b269aa743870dcb7be5d89e9c6db142f27140296ea68bc00e41ec6d67`.
Two simulator role builds both completed with maximum normalized gate error
`9.035e-8` across all 4,096 rows (`/private/tmp/q35-noc-response-final-20261125-five-run.log`
and `/private/tmp/q35-noc-response-20261125-five-run.log`). The gate's tiled
SHA-256 `9e3596cdad68531922f9a5e8775e576d4254ba45f66281c203c3baa64d0cadbb`
was independently reproduced by transposing the raw GGUF rows. The fixture
generator passed ASan/UBSan and produced identical bytes to the optimized
build. A deliberately changed gate byte was rejected before sequence
execution. The six-step test additionally verifies all 8,192 convolution
outputs and 24,576 history entries. Generate a second fixture in a fresh
directory with the optional final argument `1` to use nonzero synthetic old
history instead of the first-token behavior (`0`, the default). Independent
references matched both cases, with maximum absolute errors `1.806e-7` and
`1.494e-7` respectively (`/private/tmp/q35-resident-six-position0-final-20261125.log`
and `/private/tmp/q35-resident-six-position1-final-20261125.log`). The first
case deliberately supplies nonzero old history and confirms it is ignored;
the second checks the shift. The position-one generator passed ASan/UBSan and
matched optimized fixture bytes. A changed synthetic history byte was rejected
before execution, and an independent raw-GGUF read matched convolution-weight
SHA-256 `e0ff35e774d276fa65d1ccde5e6667345ed74da153f906d27b688fda88f155cd`.
These are separate simulations with synthetic history, not sequential warm
model runs or performance measurements.


The same authenticated sequence can continue through the sharded recurrent
update and normalization. Compile its fixture generator with the additional
`-DPIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY=1` option, generate a fresh fixture
with position `0` or `1`, and pass that directory to the same simulator command
with the sharded resident image (`delta-shard.marker`) and compatible roles.
The image and fixture marker check rejects a sharded/non-sharded mismatch.
The current fixture runs the complete first recurrent layer: 17 model
operations and four diagnostic copies of intermediate values before buffer
reuse. The twenty-second record deliberately faults without a token candidate.
For both positions, all 120 simulated workers reached all 21 barriers. The test reads the authenticated layer-0 `ssm_a`, `ssm_dt.bias` and
`ssm_norm.weight` tensors, and compares all 524,288 recurrent-state values
and 4,096 gated outputs to an independent double-precision reference. Position
0 ignores a supplied nonzero synthetic old state; position 1 uses it. Maximum
absolute state/output errors were `1.260e-7`/`3.536e-7` and
`2.005e-7`/`1.075e-6`, respectively. The final Q5_K recurrent output projection
consumes those 4,096 outputs and matches an independent quantized reference
across 2,560 values; maximum normalized errors were `5.035e-8` and `8.177e-8`
(`/private/tmp/q35-resident-proj-position0-20261125.log` and
`/private/tmp/q35-resident-proj-position1-20261125.log`). The pinned tiled
`ssm_out.weight` SHA-256 `25d73bfd7e111e9a0f06f78ed11e4531cf645c3909aa7fa954d414181404dd13`
matched an independent raw-GGUF transpose. The subsequent attention residual,
post-attention RMS, Q4_K feed-forward gate/up, gated SiLU, Q6_K down projection
and final residual were checked against independent references. All 9,216 up
and gated activation values and all 2,560 final outputs matched; the worst
normalized final-output errors were `1.045e-7` for position 0 and `1.338e-7`
for position 1 (`/private/tmp/q35-resident-layer-position0-20261125.log` and
`/private/tmp/q35-resident-layer-position1-20261125.log`). The gate, up and
down tiled weights were independently transposed from the raw GGUF, and the
post-attention norm was independently checked from the file. The position-one
fixture generator passed ASan/UBSan and reproduced the optimized output byte
for byte; corrupted feed-forward weights were rejected. A non-sharded fixture
still reproduces its earlier bytes. Previous checks also rejected corrupted
recurrent parameters and a sharded fixture paired with a non-sharded image. The F32 parameter
hashes were independently checked against the raw GGUF. The sharded image
used here is `/private/tmp/q35-delta-shard-helper-final4/driver/qwen35-resident-service-image`;
its text and dispatch SHA-256 values are `e32fdf37720ac2abb88d13abeeb581becc756c7687cf2000ef9e42450f00b4f7`
and `55e2ae86fd22717cf5ea55866dd77ebb48cdf579df3eddfc4c98a3edafca6881`.
These are separate simulations using the unqualified exploratory simulator,
not sequential warm model runs, full model output or hardware performance.

The fixture also supports a two-process state handoff. The first run exports
the verified position-zero convolution history and recurrent state to a new
file with `--resident-export-state`. The second reuses the same position-zero
`--resident-sequence-fixture` and adds `--resident-import-state` and
`--resident-position-one-command` pointing to the position-one fixture's
`command.bin`. The latter must differ from the original command only in
position and its checksum. The second run places the saved data in the
position-one read buffers and checks the opposite write buffers against the
independent references. `pioneer_qwen35_ttsim_state_handoff_test.py` checks
framing, corruption rejection and command validation. The snapshot hash is
an accidental-corruption check, not an authentication mechanism.

For example, with separately generated position-zero and position-one fixture
directories, a sharing-enabled sharded image, compatible role images and the
experimental simulator build:

```sh
python3 -B tools/pioneer/tests/pioneer_qwen35_ttsim_service_test.py LIBTTSIM RESIDENT_IMAGE ROLE_IMAGES \
  --resident-sequence-fixture POSITION0_DIR --resident-export-state /tmp/q35-sim-state.bin
python3 -B tools/pioneer/tests/pioneer_qwen35_ttsim_service_test.py LIBTTSIM RESIDENT_IMAGE ROLE_IMAGES \
  --resident-sequence-fixture POSITION0_DIR --resident-import-state /tmp/q35-sim-state.bin \
  --resident-position-one-command POSITION1_DIR/command.bin
```

On 2026-11-25 both simulations passed: all 120 workers completed 21 barriers,
the second run checked all 524,288 state values and its final 2,560 layer
outputs, and both intentionally faulted at the next record. See
`benchmarks/qwen35-state-handoff-sim-20261125/EVIDENCE.md` for hashes, error
bounds and limitations.

Each run starts a separate simulator and intentionally faults at record 22;
this is a state-handoff and template-patching check, not proof that a service
remains resident across tokens, a complete model, a hardware warm run or a
throughput measurement.

The separate sharded-delta witness tests a proposed two-stage recurrent
update. The helper builds the fused, sharing-enabled service and its
non-loadable driver with `--compile-delta-shard-prototype`, alongside the
existing recovery, device-dispatch, role-write, shared-role-owner,
snapshot-takeover and resident-service prototype flags. The build sets a
distinct stream version and image marker; driver/image packaging guards reject
a mixed sharded and unsharded build. Generate its synthetic fixture and run it
with the zero-upper-length patch:

```sh
c++ -std=c++17 -O2 -ffp-contract=off -Wall -Wextra -Werror \
  tools/pioneer/tests/pioneer_qwen35_ttsim_delta_shards_fixture.cpp -o /tmp/q35-delta-shards-fixture
/tmp/q35-delta-shards-fixture /path/to/empty-directory
python3 -B tools/pioneer/tests/pioneer_qwen35_ttsim_service_test.py \
  /path/to/zero-upper-length-patched-libttsim.so /path/to/sharded-resident-service-image \
  /path/to/build/qwen35-shared-role-images --delta-shards-fixture /path/to/empty-directory
```

All 120 workers update disjoint state panels, then a device graph barrier
precedes normalization by one worker per head. Another barrier completes the
second step; a deliberately invalid third record ends the test without a token
candidate. All 524,288 nonzero-state values and 8,192 QKV values matched the
existing full-head worker in simulation. Host sanitizer tests additionally
compare fresh state, reverse worker order and fault handling. This arrangement
adds one graph step per recurrent layer: the fused prompt/decode schedules
become 521/524 steps. Its effect on physical throughput is unknown. This
synthetic, non-loadable witness does not establish physical ownership,
no-reboot recovery, full-model output or the 100 tokens/sec goal.

For the fixed projection, create the fixture in an existing empty directory:

```sh
c++ -std=c++17 -O2 -Wall -Wextra -Werror \
  tools/pioneer/tests/pioneer_qwen35_ttsim_fixture.cpp -o /tmp/q35-ttsim-fixture
/tmp/q35-ttsim-fixture /path/to/fixture-directory
python3 -B tools/pioneer/tests/pioneer_qwen35_ttsim_service_test.py \
  /path/to/libttsim.so /path/to/build/qwen35-dataflow-service-image \
  /path/to/build/qwen35-shared-role-images --projection-fixture /path/to/fixture-directory
```

At the pinned simulator commit, the unmodified simulator stops at the upper
half of `NOC_AT_LEN_BE`, which the rewrite sets to zero. For exploration only,
`tests/ttsim-blackhole-zero-upper-length.patch` lets the simulator read and
write that zero value while still rejecting nonzero values. Apply it to a
separate simulator checkout and rebuild; do not confuse this patched binary
with the upstream reference. The production projection then stops on a
`SETC16` index absent from the simulator's supported register model. The
actual [Blackhole register definitions](https://github.com/tenstorrent/tt-metal/blob/81989dcdb8f9b340c932ae7a71a346f4f08703eb/tt_metal/hw/inc/blackhole/cfg_defines.h)
include fields absent from that model, so the device initialization still
resets the full documented thread-state range.

The same patched simulator can exercise a non-projection operation without
changing the compiled device images. Build the synthetic SiLU fixture:

```sh
c++ -std=c++17 -O2 -Wall -Wextra -Werror \
  tools/pioneer/tests/pioneer_qwen35_ttsim_silu_fixture.cpp -o /tmp/q35-ttsim-silu-fixture
/tmp/q35-ttsim-silu-fixture /path/to/fixture-directory
python3 -B tools/pioneer/tests/pioneer_qwen35_ttsim_service_test.py \
  /path/to/patched-libttsim.so /path/to/build/qwen35-dataflow-service-image \
  /path/to/build/qwen35-shared-role-images --silu-fixture /path/to/fixture-directory
```

This runs two successive commands through the same 120 simulated services,
changes the synthetic input between runs, and compares all 9,216 output
values each time against an independent mathematical reference. The test
uses the actual NoC reader, writer and completion checks in the compiled
firmware. It does not exercise Tensix matrix projections or a language model.

The RMS fixture tests the root worker's scale calculation and distribution,
then checks the output of all 120 workers. Build and run it in the same way:

```sh
c++ -std=c++17 -O2 -Wall -Wextra -Werror \
  tools/pioneer/tests/pioneer_qwen35_ttsim_rms_fixture.cpp -o /tmp/q35-ttsim-rms-fixture
/tmp/q35-ttsim-rms-fixture /path/to/fixture-directory
python3 -B tools/pioneer/tests/pioneer_qwen35_ttsim_service_test.py \
  /path/to/patched-libttsim.so /path/to/build/qwen35-dataflow-service-image \
  /path/to/build/qwen35-shared-role-images --rms-fixture /path/to/fixture-directory
```

Two successive commands reuse the resident service, with different synthetic
inputs, and compare all 2,560 output values each time to an independent RMS
reference. These are simulated operation checks, not warm model runs.

A separate temporary experiment changed the matrix format and removed some
configuration writes/readbacks solely to investigate simulator coverage. It
advanced to an unpack operation the simulator marks untested and then stopped.
That experimental image is not the production kernel and provides no
numerical or hardware correctness evidence.

The later `tests/ttsim-blackhole-matrix-exploratory.patch` is a different,
unqualified simulator experiment, applied *after* the zero-upper-length
patch to a separate checkout at the pinned `ttsim` commit. It models a few
additional instruction modes and permits only zero writes to otherwise
unmodelled thread/configuration state. Those zero-state assumptions and the
new instruction implementations are not independently validated and cannot
be treated as a golden model. For example:

```sh
cd /path/to/separate-ttsim-checkout
patch -p1 < /path/to/HaikuOS/tools/pioneer/tests/ttsim-blackhole-zero-upper-length.patch
patch -p1 < /path/to/HaikuOS/tools/pioneer/tests/ttsim-blackhole-matrix-exploratory.patch
python3 make.py src/_out/bh/libttsim.so
```

With that experimental simulator, the original projection image stops on
`UndefinedBehavior: SrcA set valid after unpack to Dst (HW erratum TEN-3868)`.
The rewrite now uses no-flip Dst unpack followed by Blackhole's clear-and-give
form before `MOVD2A`, as described by [Tenstorrent's SrcA documentation](https://github.com/tenstorrent/tt-isa-documentation/blob/main/WormholeB0/TensixTile/TensixCoprocessor/SrcASrcB.md#hardware-bug-giving-srca-to-the-matrix-unit-after-an-unpacr-to-dst).
The new image passes that simulator check. After the exploratory simulator
was extended to model `SFPMUL` modifier 2 using its existing fused
multiply-add implementation, the fixed Q4_K projection completed twice
without reloading the service. Each run used all 120 simulated workers, ten
256-element blocks per output row, and compared 1,920 results bit-for-bit.
Before execution, a separate host reference decodes the fixture's Q4_K
blocks and checks its expected results; the second command reverses every
input sign and resets the output sentinels. A deliberately corrupted weight
was rejected by the reference check.

The fixture uses synthetic weights and inputs in one simulated DRAM bank. Its
successful result depends on the experimental simulator changes, which have
not been independently qualified. It does not prove general Q4_K accuracy,
correct bank ownership, instruction timing, full-model output, throughput,
or recovery on physical hardware.

To exercise actual 4B model weights, first create the fixed fixture as above,
then replace its synthetic weight bytes using the real tiled reader:

```sh
c++ -std=c++17 -O2 -Wall -Wextra -Werror \
  tools/pioneer/tests/pioneer_qwen35_ttsim_model_fixture.cpp \
  tools/pioneer/PioneerModelWeightCache.cpp -o /tmp/q35-ttsim-model-fixture
/tmp/q35-ttsim-model-fixture /path/to/Qwen3.5-4B-Q4_K_M.gguf /path/to/fixture-directory q4
python3 -B tools/pioneer/tests/pioneer_qwen35_ttsim_service_test.py \
  /path/to/experimental-libttsim.so /path/to/build/qwen35-dataflow-service-image \
  /path/to/build/qwen35-shared-role-images --model-projection-type q4 \
  --model-projection-fixture /path/to/fixture-directory
```

The fixture generator hashes the complete pinned GGUF on its open file,
checks file identity through tiled reads, and takes the first 1,920 rows of
the selected tensor. The available choices are `q4` for
`blk.0.ffn_gate.weight`, `q5` for `blk.0.attn_qkv.weight`, and `q6` for
`token_embd.weight` (the tied output head). Start with a fresh fixed fixture
directory for each choice, and pass the same choice to both commands.

The test independently checks the resulting tiled weight digest, decodes the
selected format on the host, and compares two resident commands, each with
2,560 synthetic input values. It accepts an absolute error of at most
`2e-6 * max(1, sum(abs(weight * input)))` per row. The largest observed
normalized errors for Q4_K, Q5_K and Q6_K were `1.573e-7`, `1.216e-7` and
`8.096e-8`, respectively, in each run. These are three tensor slices in the
experimental simulator; they do not establish full-model inference,
physical recovery, or P100A speed.

The fused fixture exercises full-width QKV, Alpha and Beta projections together:

```sh
c++ -std=c++17 -O2 -Wall -Wextra -Werror \
  tools/pioneer/tests/pioneer_qwen35_ttsim_fused_fixture.cpp \
  tools/pioneer/PioneerModelWeightCache.cpp -o /tmp/q35-ttsim-fused-fixture
/tmp/q35-ttsim-fused-fixture /path/to/Qwen3.5-4B-Q4_K_M.gguf /path/to/empty-directory
python3 -B tools/pioneer/tests/pioneer_qwen35_ttsim_service_test.py \
  /path/to/experimental-libttsim.so /path/to/build/qwen35-dataflow-service-image \
  /path/to/build/qwen35-shared-role-images --fused-model-fixture /path/to/empty-directory
```

The generator authenticates the complete pinned model and uses the production
tiled reader, triple-projection binder and activation-sharing planner. These
assign 116 workers to 8,192 Q5_K QKV rows and two each to the 32 Q8_0 Alpha
and Beta rows. Q5_K and Q8_0 use separate sharing groups. The test checks the
three independently transposed tensor digests, runs two commands with
sign-reversed synthetic activations, and compares all 8,256 outputs per run
against independent quantized references with the tolerance above. It executes
standalone services with those descriptors; it does not execute the complete
resident graph recorder or language model. The exploratory simulator remains
unqualified, and these results provide no physical recovery or performance
evidence.
