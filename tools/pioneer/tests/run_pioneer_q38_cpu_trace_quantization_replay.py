#!/usr/bin/env python3
"""Replay input quantization on pinned CPU states; never device/model execution."""
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile

from compare_pioneer_q38_observed_trace import require
from run_pioneer_q38_cpu_trace_rms_replay import authenticated_inputs


def main(argv=None):
    args = sys.argv[1:] if argv is None else argv
    require(len(args) == 2, 'usage: MODEL_ABSOLUTE_PATH CPU_TRACE_ABSOLUTE_DIRECTORY')
    require(not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ), 'unexpected role overrides')
    model, directory = map(pathlib.Path, args)
    require(model.is_absolute() and directory.is_absolute(), 'absolute inputs required')
    # Unchanged full SHA pins/same-open snapshots/dimensions/finite DATA.
    # This is the existing CPU-private format,NEVER a device observation.
    data = authenticated_inputs(directory)
    require(model.is_file(), 'full original model required; same-open SHA is verified in the source transport')
    here = pathlib.Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix='pioneer-q38-cpu-trace-quantization-', dir='/private/tmp') as temporary:
        root = pathlib.Path(temporary)
        snapshot, binary = root/'CPU_INPUTS_NOT_OBSERVATIONS.bin', root/'replay'
        with snapshot.open('xb') as output:
            output.write(data)
            output.flush()
            os.fsync(output.fileno())
        snapshot.chmod(0o400)
        print('CPU_PRIVATE_QUANTIZATION_INPUT_SNAPSHOT', len(data), hashlib.sha256(data).hexdigest(), flush=True)
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                        '-ffp-contract=off', '-fsanitize=address,undefined',
                        str(here/'pioneer_q38_cpu_trace_quantization_replay.cpp'),
                        str(here/'pioneer_q38_whole_original_bytes.cpp'), '-o', str(binary)], check=True)
        return subprocess.run([str(binary), str(model), str(snapshot)],
                              env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32',
                                   'UBSAN_OPTIONS': 'halt_on_error=1'}).returncode


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (ValueError, OSError, subprocess.SubprocessError) as error:
        print('REFUSED_CPU_TRACE_QUANTIZATION_REPLAY', error, file=sys.stderr)
        raise SystemExit(2)
