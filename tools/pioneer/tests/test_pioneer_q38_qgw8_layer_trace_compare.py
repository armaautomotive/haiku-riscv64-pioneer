#!/usr/bin/env python3
"""Synthetic post-run comparison controls; no accelerator execution."""
import array
import contextlib
import hashlib
import io
import math
import os
import pathlib
import struct
import tempfile
import sys
from unittest.mock import patch
import compare_pioneer_q38_qgw8_layer_trace as target

values = array.array('f', (float(layer) for layer in range(65) for _ in range(target.WORDS)))
bits = array.array('I'); bits.frombytes(values.tobytes())
cpu = {'bits': bits, 'floats': values}
expected = array.array('f', values[target.WORDS:])
if os.sys.byteorder != 'little':
    expected.byteswap()
raw = expected.tobytes()
rows, first = target.compare(cpu, raw, 0)
assert len(rows) == 64 and first is None and all(r[1:] == (0, 0, 0) for r in rows)
altered = bytearray(raw)
struct.pack_into('<f', altered, 31 * target.WORDS * 4, 64.0)
rows, first = target.compare(cpu, altered, 0.001)
assert first == 31 and rows[31][1] == 1 and rows[30][1] == 0
assert target.compare(cpu, altered, rows[31][3])[1] is None  # threshold is strictly exceeded
for bad, threshold in ((raw[:-1], 0), (raw + b'\0', 0), (raw, -1), (raw, math.inf), (raw, math.nan)):
    try:
        target.compare(cpu, bad, threshold)
    except ValueError:
        pass
    else:
        raise AssertionError('invalid data accepted')
bad = bytearray(raw); struct.pack_into('<I', bad, 0, 0x7f800000)
try:
    target.compare(cpu, bad, 0)
except ValueError:
    pass
else:
    raise AssertionError('nonfinite data accepted')

with tempfile.TemporaryDirectory() as directory:
    root = pathlib.Path(directory); trace = root/'raw.bin'; trace.write_bytes(raw)
    digest = hashlib.sha256(raw).hexdigest()
    args = [str(root), str(trace), '--sha256', digest, '--relative-threshold', '0']
    with patch.object(target, 'load_cpu', return_value=[cpu]), contextlib.redirect_stdout(io.StringIO()):
        assert target.main(args) == 0
        try:
            target.main(args[:3] + ['0'*64] + args[4:])
        except ValueError:
            pass
        else:
            raise AssertionError('wrong digest accepted')
        trace.write_bytes(altered)
        args[3] = hashlib.sha256(altered).hexdigest()
        assert target.main(args) == 1
        os.environ['PIONEER_Q38_WORK_INFERENCE'] = '0'
        try:
            try:
                target.main(args)
            except ValueError:
                pass
            else:
                raise AssertionError('Work role accepted')
        finally:
            del os.environ['PIONEER_Q38_WORK_INFERENCE']
print('PASS 64 synthetic boundaries, first divergence, exact threshold, hash, finite/length/role refusals; NO DEVICE')
if len(sys.argv) == 2:
    pinned = target.load_cpu(pathlib.Path(sys.argv[1]))[0]
    control = array.array('f', pinned['floats'][target.WORDS:65*target.WORDS])
    if sys.byteorder != 'little':
        control.byteswap()
    rows, first = target.compare(pinned, control.tobytes(), 0)
    assert first is None and len(rows) == 64 and all(row[1] == 0 for row in rows)
    print('PASS all 64 boundaries from pinned original CPU DATA; not a device observation')
else:
    assert len(sys.argv) == 1
