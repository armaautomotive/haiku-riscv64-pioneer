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
numerical or hardware correctness evidence. No fixed-projection output
comparison has passed.
