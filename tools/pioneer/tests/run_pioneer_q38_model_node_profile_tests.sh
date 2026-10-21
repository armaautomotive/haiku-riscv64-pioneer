#!/bin/sh
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-model-node.XXXXXXXX")
awk '/PIONEER_MODEL_NODE_OBSERVER_BEGIN/ {copying=1;next} /PIONEER_MODEL_NODE_OBSERVER_END/ {found=1;exit} copying{print} END{if(!found)exit 1}' "$directory/../pioneer_q38_inference.cpp" > "$temporary/model_node_observer.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I"$temporary" "$directory/pioneer_q38_model_node_profile_test.cpp" -o "$temporary/test"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test"
project=$(CDPATH= cd -- "$directory/../../.." && pwd)
git -C "$project/src/libs/llama.cpp" show c060ca974c773c7c3d17fd1b66dc9d312bc292c0:ggml/src/ggml-backend.cpp | awk '
 /^        if \(!sched->callback_eval\) \{$/ {copying=1;starts++}
 copying && /^        \/\/ record the event of this split/ {found=1;exit}
 copying{print} END{if(!found||starts!=1)exit 1}' > "$temporary/model_node_scheduler.inc"
${CXX:-c++} -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I"$temporary" "$directory/pioneer_q38_model_node_scheduler_test.cpp" -o "$temporary/scheduler"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/scheduler"
python3 "$directory/pioneer_q38_model_node_build_gate_test.py"
printf 'Offline node observer evidence retained at %s;NO native access\n' "$temporary"
