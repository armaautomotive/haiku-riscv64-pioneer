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
with the upstream reference. The projection then reaches matrix setup but
terminates on the simulator's unsupported `Dstacc_override` mode. No output
comparison has passed. Do not disable or bypass that simulator check or alter
the physical matrix format merely to make the test pass.
