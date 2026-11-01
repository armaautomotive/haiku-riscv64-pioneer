#!/usr/bin/env python3
"""Build/inspect a separate raw-frontend USER client;NEVER install/run it."""
import hashlib
import os
import pathlib
import runpy
import subprocess
import sys

OBJECT_SHA = '76e5a770f3cdf56bfd47d763e5ea56833403c347f5f6558ecb2cc6e7dcd13020'
BINARY_SHA = '718d0ceac7eb5caad771413aefa32903fabf1e81b3dc6ad34793fd8ad0b7b77c'
assert len(sys.argv) in (3, 4) and not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
inspect = len(sys.argv) == 4
assert not inspect or sys.argv[3] == '--inspect-candidate'
assert inspect or OBJECT_SHA and BINARY_SHA
build, out = map(pathlib.Path, sys.argv[1:3])
here = pathlib.Path(__file__).resolve().parent
assert build.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
saved = sys.argv[:]
try:
    sys.argv = [str(here / 'build_qwen38_frontier_benchmark_client.py'), str(build), str(out / 'unchanged-frontier-client')]
    old = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
# Reproduce ENTIRE old4b+055+allshort/timed/hidden controls FIRST.
cxx, flags, includes, libraries, release, tool = (old[key] for key in (
    'cxx', 'flags', 'includes', 'libraries', 'release', 'tool'))
obj = out / 'frontend-profile-client.o'
command = [cxx, *flags, '-DPIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT=1',
           '-DPIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT=1', '-DPIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT=1',
           *includes, '-c', str(here / 'pioneer_q38_frontend_profile.cpp'), '-o', str(obj)]
subprocess.run(command, check=True)
(out / 'compile-argv.txt').write_text('\n'.join(command) + '\n')
binary = out / 'pioneer-q38-FRONTEND34-PROFILE-DIAGNOSTIC-ONLY'
subprocess.run([cxx, *includes, *libraries, '-B' + str(release / 'system/glue') + '/',
                '-L' + str(release / 'system/libroot'), str(obj), '-o', str(binary)], check=True)
symbols = subprocess.check_output([str(tool) + 'nm', '-C', str(binary)], text=True)
for required in ('FrontierBenchmarkTransport', 'PioneerQ38TokenHiddenValidationSession', 'PioneerQ38PublicModelSource', 'ioctl'):
    assert required in symbols, required
for forbidden in ('llama_', 'ggml_', 'OriginalSource', 'ReferenceProducts', 'PioneerSessionIO',
                  'PioneerStoreContext', 'Q38WorkControl', 'NativeQ38PeerInstructionModel', 'SyntheticIO', 'SyntheticSource'):
    assert forbidden not in symbols, forbidden
data = binary.read_bytes()
for required in (b'--profile-frontends-34-positions\0', b'--preflight-source-only\0', b'/bin/sha256sum\0',
                 b'Q38_FRONTEND_PROFILE_ZERO_EXTRA_ACCELERATOR_CALLS_RAW_NOT_RETIRED_PC_OR_CYCLES',
                 b'Q38_FRONTEND_ROWS_BEGIN', b'Q38_FRONTEND_ROWS_END', b'Q38_FRONTEND_ROW index',
                 b'POST_PROMPT_COUNT_WINDOW_NOT_ISOLATED_TOKEN_LATENCY_OR_QUALITY_PROOF'):
    assert required in data, required
for forbidden in (b'--benchmark-34-positions\0', b'--generate-34-positions\0', b'--generate-three-positions\0',
                  b'--capture-first-layer\0', b'--work\0', b'--cpu\0'):
    assert forbidden not in data, forbidden
rows = [row.split('\t') for row in obj.with_suffix('.su').read_text().splitlines()]
assert rows and all(len(row) == 3 and row[2] == 'static' and int(row[1]) <= 32768 for row in rows)
for path, expected in ((obj, OBJECT_SHA), (binary, BINARY_SHA)):
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    if not inspect:
        assert digest == expected, (path, digest)
    print(digest + ' ' + path.name)
print('PASS_BUILD_ONLY separate raw frontend USER profile/wholeold4b+055+short+timed+hidden unchanged/'
      'same6607+6608 source+FD+generation+Finish+Close/noCPU oracle,native,Work or MOCK imports/'
      'maxIndividualUSERFrame' + str(max(int(row[1]) for row in rows)))
print('Profile copies existing Observe response PCs ONLY;heap samples printed AFTER cleanFinish+Close+records;'
      'NO retiredPC,cycle,function-time,model-quality,physical or deployment qualification')
