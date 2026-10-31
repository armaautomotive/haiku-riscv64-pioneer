#!/usr/bin/env python3
"""Localize RMS arithmetic using pinned CPU states, never device observations."""
import array
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile

from compare_pioneer_q38_observed_trace import HIDDEN_WORDS, load_cpu, require


def authenticated_inputs(directory):
    require(directory.is_absolute(), 'absolute CPU trace directory required')
    cpu = load_cpu(directory)  # Existing full SHA pins, same-open snapshots and full DATA checks.
    words = array.array('I', [0x31524d51, 1, 3, 65, 5120] + [0]*11)
    require(words.itemsize == 4, '32-bit host words required')
    for token in cpu:
        words.extend(token['bits'][:HIDDEN_WORDS])
    if sys.byteorder != 'little':
        words.byteswap()
    return words.tobytes()


def main(argv=None):
    args = sys.argv[1:] if argv is None else argv
    require(len(args) == 2, 'usage: MODEL_ABSOLUTE_PATH CPU_TRACE_ABSOLUTE_DIRECTORY')
    require(not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ), 'unexpected role overrides')
    model, directory = map(pathlib.Path, args)
    require(model.is_absolute() and directory.is_absolute(), 'absolute inputs required')
    data = authenticated_inputs(directory)  # Refuse changed CPU controls BEFORE compiler/model access.
    require(model.is_file(), 'full original model required; it will be same-open SHA authenticated')
    here = pathlib.Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix='pioneer-q38-cpu-trace-rms-', dir='/private/tmp') as temporary:
        root = pathlib.Path(temporary)
        snapshot, binary = root/'CPU_INPUTS_NOT_OBSERVATIONS.bin', root/'replay'
        with snapshot.open('xb') as output:
            output.write(data)
            output.flush()
            os.fsync(output.fileno())
        snapshot.chmod(0o400)
        print('CPU_PRIVATE_RMS_INPUT_SNAPSHOT', len(data), hashlib.sha256(data).hexdigest(), flush=True)
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                        '-ffp-contract=off', '-fsanitize=address,undefined',
                        str(here/'pioneer_q38_cpu_trace_rms_replay.cpp'),
                        str(here/'pioneer_q38_whole_original_bytes.cpp'), '-o', str(binary)], check=True)
        return subprocess.run([str(binary), str(model), str(snapshot)],
                              env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32',
                                   'UBSAN_OPTIONS': 'halt_on_error=1'}).returncode


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (ValueError, OSError, subprocess.SubprocessError) as error:
        print('REFUSED_CPU_TRACE_RMS_REPLAY', error, file=sys.stderr)
        raise SystemExit(2)
