#!/usr/bin/env python3
"""Check full-layer error accounting used by the offline simulator runner."""
import math
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
from pioneer_qwen35_ttsim_service_test import (RecordedNumericalMismatch, compare_recorded_layer,
                                               compare_recorded_layer_diagnostic, compare_recorded_token,
                                               validate_isolated_zero_ranges, validate_recorded_state_layout)
from pioneer_qwen35_ttsim_state_compare import compare_state

assert compare_recorded_token(261, 261, False) is None
assert compare_recorded_token(1850, 89598, True) == 'recorded decode token 1850 differs from reference 89598'
try:
    compare_recorded_token(1850, 89598, False)
    raise AssertionError('ordinary or nonterminal token mismatch accepted')
except RecordedNumericalMismatch:
    pass

with tempfile.TemporaryDirectory() as directory:
    runner = Path(__file__).with_name('pioneer_qwen35_ttsim_service_test.py')
    command = [sys.executable, '-B', str(runner), 'missing-library', 'missing-image', 'missing-roles',
               '--recorded-layer-fixture', 'missing-fixture', '--recorded-layer-reference', 'missing-layer',
               '--recorded-token-reference', 'missing-token']
    missing_warm = subprocess.run(command + ['--recorded-warm-snapshots', str(Path(directory) / 'new')],
                                  capture_output=True, text=True)
    assert missing_warm.returncode != 0 and 'warm snapshots require two recorded decodes' in missing_warm.stderr
    existing = subprocess.run(command + ['--recorded-warm-layer-reference', 'warm-layer',
                              '--recorded-warm-token-reference', 'warm-token',
                              '--recorded-warm-snapshots', directory], capture_output=True, text=True)
    assert existing.returncode != 0 and 'warm snapshots require two recorded decodes' in existing.stderr

state_map = 'H 0 12 0 67108864 0 8192\nD 0 65536 0 83886080 0 32\n'
state_weights = [(0, 100663296, 4096, 0)]
state_streams = [(0, 0, 117440512)]
state_outputs = [(0, 134217728, 0, 2560)]
assert len(validate_recorded_state_layout(state_map, state_weights, state_streams, [], state_outputs, 532, 1, 32)) == 2
for bad_map in (state_map.splitlines()[0], state_map + state_map.splitlines()[0] + '\n',
                state_map.replace('83886080', '100663296'), state_map.replace(' 8192\n', ' 8176\n'),
                state_map.replace('D 0 65536', 'K 0 16384')):
    try:
        validate_recorded_state_layout(bad_map, state_weights, state_streams, [], state_outputs, 532, 1, 32)
        raise AssertionError('invalid or overlapping simulator state map accepted')
    except ValueError:
        pass
assert compare_state(struct.pack('<3f', 0, 1, -2), struct.pack('<3f', 0, 1, -2), 3)['mismatches'] == 0
assert compare_state(struct.pack('<3f', 0, 1, 3), struct.pack('<3f', 0, 1, -2), 3)['worst_index'] == 2
for bad in (b'', struct.pack('<3f', 0, math.nan, 2)):
    try:
        compare_state(bad, struct.pack('<3f', 0, 1, 2), 3)
        raise AssertionError('malformed recurrent state accepted')
    except ValueError:
        pass

reference = [1.0] * 2560
actual = reference.copy(); actual[8] += 1e-6
pack = lambda values: struct.pack('<2560f', *values)
assert 0 < compare_recorded_layer(pack(actual), pack(reference)) < 2e-5
actual[7] = 3.0; actual[9] = -4.0
try:
    compare_recorded_layer(pack(actual), pack(reference))
    raise AssertionError('multiple bad rows accepted')
except RecordedNumericalMismatch as error:
    assert '2/2560' in str(error) and 'row 9' in str(error)
maximum, mismatch = compare_recorded_layer_diagnostic(pack(actual), pack(reference), True)
assert maximum is None and '2/2560' in mismatch
try:
    compare_recorded_layer_diagnostic(pack(actual), pack(reference), False)
    raise AssertionError('normal run continued after mismatch')
except RecordedNumericalMismatch:
    pass
actual[8] = math.nan
try:
    compare_recorded_layer(pack(actual), pack(reference))
    raise AssertionError('NaN accepted')
except RuntimeError as error:
    assert 'non-finite' in str(error) and not isinstance(error, RecordedNumericalMismatch)
try:
    compare_recorded_layer_diagnostic(pack(actual), pack(reference), True)
    raise AssertionError('diagnostic run continued after non-finite output')
except RecordedNumericalMismatch:
    raise AssertionError('non-finite output was classified as a finite mismatch')
except RuntimeError:
    pass
try:
    compare_recorded_layer(b'', pack(reference))
    raise AssertionError('truncated data accepted')
except ValueError:
    pass
wide = struct.pack('<9216f', *([1.0] * 9216))
assert compare_recorded_layer(wide, wide, 9216) == 0
bad_wide = bytearray(wide)
struct.pack_into('<f', bad_wide, 9215 * 4, 2.0)
try:
    compare_recorded_layer(bad_wide, wide, 9216)
    raise AssertionError('last wide-output row mismatch accepted')
except RecordedNumericalMismatch as error:
    assert '1/9216' in str(error) and 'row 9215' in str(error)
try:
    compare_recorded_layer(wide, wide)
    raise AssertionError('wide output accepted without explicit width')
except ValueError:
    pass
qkv = struct.pack('<8192f', *([1.0] * 8192))
assert compare_recorded_layer(qkv, qkv, 8192) == 0
bad_qkv = bytearray(qkv)
struct.pack_into('<f', bad_qkv, 8191 * 4, 2.0)
try:
    compare_recorded_layer(bad_qkv, qkv, 8192)
    raise AssertionError('last QKV output row mismatch accepted')
except RecordedNumericalMismatch as error:
    assert '1/8192' in str(error) and 'row 8191' in str(error)
z = struct.pack('<4096f', *([1.0] * 4096))
assert compare_recorded_layer(z, z, 4096) == 0
bad_z = bytearray(z)
struct.pack_into('<f', bad_z, 4095 * 4, 2.0)
try:
    compare_recorded_layer(bad_z, z, 4096)
    raise AssertionError('last Z output row mismatch accepted')
except RecordedNumericalMismatch as error:
    assert '1/4096' in str(error) and 'row 4095' in str(error)
scalar = struct.pack('<32f', *([1.0] * 32))
assert compare_recorded_layer(scalar, scalar, 32) == 0
bad_scalar = bytearray(scalar)
struct.pack_into('<f', bad_scalar, 31 * 4, 2.0)
try:
    compare_recorded_layer(bad_scalar, scalar, 32)
    raise AssertionError('last scalar output row mismatch accepted')
except RecordedNumericalMismatch as error:
    assert '1/32' in str(error) and 'row 31' in str(error)
weights = [(0, 0x04000000, 4096, 0)]
streams = [(0, 0, 0x04100000)]
inputs = [(0, 0x04400000, 0, 16)]
outputs = [(0, 0x04500000, 0, 16)]
safe = (0, 0x04600000, 4096)
assert validate_isolated_zero_ranges([safe], weights, streams, inputs, outputs, 524) == [safe]
for bad in ((0, 0x04000000, 4096), (0, 0x04100000, 4096),
            (0, 0x04400000, 4096), (0, 0x04500000, 4096),
            (0, 0x04600000, -4), (7, 0x04600000, 4096)):
    try:
        validate_isolated_zero_ranges([bad], weights, streams, inputs, outputs, 524)
        raise AssertionError('unsafe simulator state clear accepted')
    except ValueError:
        pass
try:
    validate_isolated_zero_ranges([safe, safe], weights, streams, inputs, outputs, 524)
    raise AssertionError('overlapping simulator state clears accepted')
except ValueError:
    pass
print('PASS full-layer aggregate comparison and non-finite rejection; NO HARDWARE')
