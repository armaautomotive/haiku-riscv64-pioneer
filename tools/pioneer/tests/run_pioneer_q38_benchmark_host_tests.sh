#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-benchmark-host-tests.XXXXXXXX")
for variant in normal profile batch4 raw frontend;do
extra=;[ "$variant" != profile ] || extra=-DPIONEER_Q38_BENCHMARK_HOST_PROFILE_QUALIFICATION
[ "$variant" != batch4 ] || extra=-DPIONEER_Q38_BENCHMARK_HOST_BATCH4_QUALIFICATION
[ "$variant" != raw ] || extra=-DPIONEER_Q38_BENCHMARK_HOST_BATCH_RAW_QUALIFICATION
[ "$variant" != frontend ] || extra=-DPIONEER_Q38_BENCHMARK_HOST_FRONTEND_QUALIFICATION
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 $extra "$directory/pioneer_q38_benchmark_host_test.cpp" -o "$temporary/policy"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/policy"
awk '/PIONEER_NATIVE_Q8_WORK_SUPPORT_BEGIN/ {copying=1;next} /PIONEER_NATIVE_Q8_WORK_SUPPORT_END/ {found=1;exit}
 copying {print} END {if(!found)exit 1}' "$directory/../PioneerQ38WorkSession.h" > "$temporary/benchmark_host_work_support.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 $extra -I"$temporary" "$directory/pioneer_q38_benchmark_host_work_support_test.cpp" -o "$temporary/support"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/support"
done
python3 "$directory/pioneer_q38_benchmark_host_build_gate_test.py"
awk '/PIONEER_Q38_BENCHMARK_FORK_BARRIER_BEGIN/ {copying=1;next} /PIONEER_Q38_BENCHMARK_FORK_BARRIER_END/ {found=1;exit}
 copying {print} END {if(!found)exit 1}' "$directory/../PioneerQ38PublicModelSource.h" > "$temporary/benchmark_fork_barrier.inc"
# ONLY target-path adaptation:actual main is unchanged except mkdtemp's
# Haiku /boot/home prefix becomes this CPU test's private temporary root.
awk '/^int main\(int argc,char\*\* argv\)/ {copying=1} copying {print} END {if(!copying)exit 1}' \
 "$directory/../pioneer_q38_benchmark_host.cpp" | sed 's@char directory\[\]="/boot/home/pioneer-q38-benchmark.XXXXXX";@char directory[1024];assert(snprintf(directory,sizeof(directory),"%s",getenv("TEST_DIRECTORY"))>0);@' > "$temporary/benchmark_host_main.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$temporary" "$directory/pioneer_q38_benchmark_host_process_test.cpp" -o "$temporary/process"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 python3 "$directory/pioneer_q38_benchmark_host_process_test.py" "$temporary/process"
printf 'Offline benchmark-host evidence retained at %s\n' "$temporary"
