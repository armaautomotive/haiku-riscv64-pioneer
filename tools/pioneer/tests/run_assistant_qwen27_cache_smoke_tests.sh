#!/bin/sh
# Actual smoke client + fake protocol worker ONLY; no model/device access.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/assistant-qwen27-cache-smoke.XXXXXXXX")
flags='-std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined'
${CXX:-c++} $flags "$directory/assistant_qwen27_fake_worker.cpp" -o "$temporary/worker"
${CXX:-c++} $flags "-DASSISTANT_QWEN27_WORKER=\"$temporary/worker\"" \
 "$directory/pioneer_assistant_qwen27_cache_load_smoke.cpp" "$directory/../../../src/apps/assistant/P100AQwen27.cpp" -o "$temporary/test"
for mode in cachewarm cacheconverted cachemissing normal reject badready badclose; do
 touch "$temporary/$mode.events"
 if ASSISTANT27_TEST_LOG="$temporary/$mode.events" ASSISTANT27_TEST_MODE=$mode UBSAN_OPTIONS=halt_on_error=1 \
  "$temporary/test" --load-unload-only > "$temporary/$mode.stdout" 2> "$temporary/$mode.stderr"; then result=0;else result=$?;fi
 if [ "$mode" = cachewarm ];then
  [ "$result" = 0 ] && [ ! -s "$temporary/$mode.stderr" ]
  [ "$(tail -n 1 "$temporary/$mode.stdout")" = 'PASS_ACTUAL_ASSISTANT_CACHE_LOAD_UNLOAD verified_tensors851;NO_GENERATE_NO_WEIGHT_UPLOAD_NO_INFERENCE_SPEED_CLAIM' ]
 else
  [ "$result" = 1 ] && [ "$(wc -l < "$temporary/$mode.stderr" | tr -d ' ')" = 1 ]
  case "$mode" in
   cacheconverted) expected='CACHE_WORKER_NOT_QUALIFIED closed1 faulted0 banner1 converted1 verified_tensors851:';;
   cachemissing) expected='CACHE_WORKER_NOT_QUALIFIED closed1 faulted0 banner1 converted0 verified_tensors850:';;
   normal) expected='CACHE_WORKER_NOT_QUALIFIED closed1 faulted0 banner0 converted0 verified_tensors0:';;
   badclose) expected='CACHE_WORKER_NOT_QUALIFIED closed0 faulted1 banner0 converted0 verified_tensors0:';;
   reject) expected='CACHE_WORKER_LOAD_REFUSED faulted0:';;
   badready) expected='CACHE_WORKER_LOAD_REFUSED faulted1:';;
  esac
  case "$(cat "$temporary/$mode.stderr")" in "$expected"*) ;; *) exit 1;; esac
 fi
 want=2;case "$mode" in reject|badready) want=1;; esac
 awk -v want="$want" 'NR==1 && $0=="load"{next} NR==2 && $0=="unload"{next} {exit 1} END{if(NR!=want)exit 1}' "$temporary/$mode.events"
done
printf 'PASS actual cache load/unload smoke client:7 fake-worker cases;851 verified tensors required,conversion/missing progress and wrong/rejected/unclean peers refused;NO Generate or device IO\nEvidence: %s\n' "$temporary"
