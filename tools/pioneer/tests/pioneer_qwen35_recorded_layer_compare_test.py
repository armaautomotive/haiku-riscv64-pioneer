#!/usr/bin/env python3
"""Check full-layer error accounting used by the offline simulator runner."""
import math
import struct
from pioneer_qwen35_ttsim_service_test import compare_recorded_layer, validate_isolated_zero_ranges

reference = [1.0] * 2560
actual = reference.copy(); actual[8] += 1e-6
pack = lambda values: struct.pack('<2560f', *values)
assert 0 < compare_recorded_layer(pack(actual), pack(reference)) < 2e-5
actual[7] = 3.0; actual[9] = -4.0
try:
    compare_recorded_layer(pack(actual), pack(reference))
    raise AssertionError('multiple bad rows accepted')
except RuntimeError as error:
    assert '2/2560' in str(error) and 'row 9' in str(error)
actual[8] = math.nan
try:
    compare_recorded_layer(pack(actual), pack(reference))
    raise AssertionError('NaN accepted')
except RuntimeError as error:
    assert 'non-finite' in str(error)
try:
    compare_recorded_layer(b'', pack(reference))
    raise AssertionError('truncated data accepted')
except ValueError:
    pass
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
