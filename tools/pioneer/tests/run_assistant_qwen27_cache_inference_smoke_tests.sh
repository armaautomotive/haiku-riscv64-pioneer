#!/bin/sh
# Finite fake protocol peer ONLY, never SSH/model/device.
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/assistant-qwen27-cache-inference.XXXXXXXX")
flags='-std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined'
${CXX:-c++} $flags "$directory/assistant_qwen27_fake_worker.cpp" -o "$temporary/worker"
${CXX:-c++} $flags "-DASSISTANT_QWEN27_WORKER=\"$temporary/worker\"" \
 "$directory/pioneer_assistant_qwen27_cache_inference_smoke.cpp" "$directory/../../../src/apps/assistant/P100AQwen27.cpp" -o "$temporary/test"
for mode in pass fast rounded converted missing nobanner fatal reject badstats nan badrate validation trailer empty badclose;do
 touch "$temporary/$mode.events"
 if ASSISTANT27_TEST_LOG="$temporary/$mode.events" ASSISTANT27_TEST_MODE=cacheinfer-$mode UBSAN_OPTIONS=halt_on_error=1 \
  "$temporary/test" --cached-generate-four-only > "$temporary/$mode.stdout" 2> "$temporary/$mode.stderr";then result=0;else result=$?;fi
 case "$mode" in
  pass|fast|rounded) [ "$result" = 0 ] && [ ! -s "$temporary/$mode.stderr" ]
   [ "$(tail -n 1 "$temporary/$mode.stdout")" = 'PASS_ACTUAL_ASSISTANT_CACHED_FOUR_TOKEN_WORK_KNOWN_CLEAN;NOT_FULL_NATIVE_NOT_20TOKENS_SEC_OR_COMPARATIVE_SPEEDUP_PROOF' ];;
  fatal|reject) [ "$result" = 70 ] && [ -s "$temporary/$mode.stderr" ];;
  *) [ "$result" = 1 ] && [ -s "$temporary/$mode.stderr" ];;
 esac
 # Rejected execution's peer observes close-only asynchronously. It is
 # irrelevant to the no-second-command invariant checked below.
 case "$mode" in
  converted|missing|nobanner) want=host;;
  fatal|reject) want=unknown;;
  *) want=complete;;
 esac
 awk -v want="$want" '
 NR==1 && $0=="load"{next}
 NR==2 && want=="host" && $0=="unload"{next}
 NR==2 && want!="host" && $0=="generate"{next}
 NR==3 && want=="complete" && $0=="unload"{next}
 NR==3 && want=="unknown" && $0=="close-only"{next}
 {exit 1} END{if(NR<2 || NR>3 || (want=="host" && NR!=2) || (want=="complete" && NR!=3))exit 1}' "$temporary/$mode.events"
done
${CXX:-c++} $flags -DPIONEER_BATCHED_LOAD_TIMING=1 "-DASSISTANT_QWEN27_WORKER=\"$temporary/worker\"" \
 "$directory/pioneer_assistant_qwen27_cache_inference_smoke.cpp" "$directory/../../../src/apps/assistant/P100AQwen27.cpp" -o "$temporary/timing-test"
touch "$temporary/timing.events"
ASSISTANT27_TEST_LOG="$temporary/timing.events" ASSISTANT27_TEST_MODE=cacheinfer-timing UBSAN_OPTIONS=halt_on_error=1 \
 "$temporary/timing-test" --cached-generate-four-only > "$temporary/timing.stdout" 2> "$temporary/timing.stderr"
[ ! -s "$temporary/timing.stderr" ]
[ "$(grep -c '^BATCHED_UPLOAD_PROGRESS seconds=' "$temporary/timing.stdout")" = 2 ]
grep -q '^BATCHED_COMPLETE_SECONDS .*;INCLUDES_UPLOAD_PROMPT_AND_GENERATION$' "$temporary/timing.stdout"
printf 'PASS actual cached Assistant inference client:15 fake-peer cases;all851 cache hits before ONE fixed4-token Generate;conversion/missing banner/entry cannot Generate;unknown execution NO Unload/retry/reset/fallback;finite rounding-consistent Work-only statistics and known-clean exit required;NO model/device/hardware/throughput proof\nEvidence: %s\n' "$temporary"
