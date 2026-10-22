#!/usr/bin/env python3
"""Extract ACTUAL RootProof BOTH ways;mock scope/physical endpoints only."""
import os
import pathlib
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
source = (here.parents[2] / 'src/add-ons/kernel/drivers/misc/PioneerQ38FfnPersistentGraphOwner.h').read_text()
begin, end = ' unsigned OutputPackets(unsigned role)const {', ' void AdvancePeers(bool stop) {'
assert source.count(begin) == source.count(end) == 1
actual = begin + source.split(begin, 1)[1].split(end, 1)[0]
with tempfile.TemporaryDirectory(prefix='pioneer-ffn-sealed-receipt-') as directory:
    base = pathlib.Path(directory)
    (base / 'actual_root_proof.inc').write_text(actual)
    for sealed in [False, True]:
        binary = base / ('sealed' if sealed else 'baseline')
        command = [os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off',
                   '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
                   '-DPIONEER_Q38_FFN_GRAPH_INPUT_ROOT=1', '-DPIONEER_Q38_FFN_PERSISTENT_GRAPH=1',
                   '-DPIONEER_Q38_GDN_MUTABLE_STATE', '-DPIONEER_Q38_FFN_HOST_PROFILE=1',
                   '-DPIONEER_Q38_TEST_WORK_DOT_ZERO_ELISION', '-DPIONEER_Q38_TEST_AFFINE_REPLAY',
                   '-DPIONEER_Q38_TEST_BLOCK_SCALE_REUSE', '-DPIONEER_Q38_TEST_INT_ACCUMULATOR',
                   '-DPIONEER_Q38_TEST_PIPELINED_MATRIX']
        if sealed:
            command += ['-DPIONEER_Q38_FFN_SEALED_PROOF=1']
        subprocess.run(command + ['-I' + str(base), str(here / 'pioneer_q38_ffn_sealed_receipt_test.cpp'),
                                  '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True,
                       env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
