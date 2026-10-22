#!/usr/bin/env python3
"""Actual profile-client output/Abort/Finish branch + actual session,NO device."""
import os
import pathlib
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
client = (here.parent / 'pioneer_q38_ffn_prime.cpp').read_text()
start = ' if(okay)for(unsigned pass=0;pass<2;pass++) {'
end = '\n#endif\n if(!okay)session.Abort();'
finish = ' bool clean=session.Finish();okay=okay && clean;'
assert client.count(start) == client.count(end) == client.count(finish) == 1
actual = client.split(start, 1)[1].split(end, 1)[0]
actual = start + actual + '\n if(!okay)session.Abort();\n' + finish
fixture = (here / 'pioneer_q38_ffn_graph_session_test.cpp').read_text()
assert fixture.count('int main(){') == 1
with tempfile.TemporaryDirectory(prefix='pioneer-ffn-profile-client-') as directory:
    base = pathlib.Path(directory)
    (base / 'graph_fixture.inc').write_text(fixture.split('int main(){', 1)[0])
    (base / 'client_finish.inc').write_text(actual)
    binary = base / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off',
                    '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
                    '-DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED', '-DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR',
                    '-DPIONEER_Q38_WORK_INFERENCE', '-DPIONEER_Q38_SPAN_PRELOAD', '-DPIONEER_Q38_SPAN_FAST',
                    '-DPIONEER_Q38_FFN_PRIME=1', '-DPIONEER_Q38_FFN_MODEL_INIT_CLIENT=1',
                    '-DPIONEER_Q38_FFN_NATIVE_GRAPH_CLIENT=1', '-DPIONEER_Q38_FFN_HOST_PROFILE_CLIENT=1',
                    '-I' + str(base), '-I' + str(here),
                    str(here / 'pioneer_q38_ffn_host_profile_client_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
