#!/usr/bin/env python3
"""Pure format/metric tests; no bypass of main's pinned CPU trace authentication."""
import importlib.util
import math
import pathlib
import struct
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('trace_compare', here/'compare_pioneer_q38_observed_trace.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

def fixture(observed):
    h = [0x31444f51 if observed else 0x31524651, 1, 0, 248319, 17, 0, 0x3f800000, 0, 65, 5120, 248320, 64 if observed else 0, 0, 0, 0, 0]
    words = h+[0]*module.HIDDEN_WORDS+[0]*module.VOCAB_WORDS
    words[16+module.HIDDEN_WORDS+17] = 0x3f800000
    if observed:
        record = [module.TOKEN_MAGIC, 1, 0, 248319, 17, 0x3f800000, 1, 1, 1, 1, 64, 973]+[0]*52
        record[44:47] = [1, 48, 119]
        words += record
    words += [0x454e444f if observed else 0x454e4451]
    return bytearray(struct.pack(f'<{len(words)}I', *words))

cpu_bytes, observed_bytes = fixture(False), fixture(True)
cpu = module.decode_trace(cpu_bytes, 0, False)
actual = module.decode_trace(observed_bytes, 0, True)
assert module.difference(cpu, actual, 0, module.HIDDEN_WORDS) == (0, 0, 0)
assert module.difference(cpu, actual, module.HIDDEN_WORDS, module.VOCAB_WORDS) == (0, 0, 0)
changed = observed_bytes.copy()
struct.pack_into('<I', changed, (16+3*5120+7)*4, 0x3f800000)
different = module.decode_trace(changed, 0, True)
differences, maximum, relative = module.difference(cpu, different, 3*5120, 5120)
assert differences == 1 and maximum == 1 and math.isclose(relative, 1e150)
for bad in range(9):
    data = observed_bytes.copy()
    if bad == 0:
        data.pop()
    if bad == 1:
        data += b'\0'
    if bad == 2:
        struct.pack_into('<I', data, 0, 0x31524651)
    if bad == 3:
        struct.pack_into('<I', data, 8*4, 64)
    if bad == 4:
        struct.pack_into('<I', data, 4*4, 18)
    if bad == 5:
        struct.pack_into('<I', data, (16+module.HIDDEN_WORDS+module.VOCAB_WORDS)*4, 0)
    if bad == 6:
        struct.pack_into('<I', data, (16+7)*4, 0x7fc00001)
    if bad == 7:
        struct.pack_into('<I', data, len(data)-4, 0)
    if bad == 8:
        struct.pack_into('<I', data, (16+module.HIDDEN_WORDS+module.VOCAB_WORDS+47)*4, 1)
    try:
        module.decode_trace(data, 0, True)
    except ValueError:
        continue
    raise AssertionError(f'bad trace accepted: {bad}')
print('PASS posthoc trace format/bitexact+one-word metrics/9 truncation+extra+family+dimension+greedy+record+reserved+NaN+terminal refusals;SYNTHETIC DATA only,CPU authentication in main is NOT bypassed')
assert len(sys.argv) in (1, 2)
if len(sys.argv) == 2:
    directory = pathlib.Path(sys.argv[1])
    controls = module.load_cpu(directory)
    assert [t['header'][4] for t in controls] == [70253, 17, 17]
    with tempfile.TemporaryDirectory(prefix='pioneer-trace-auth-refusal-', dir='/private/tmp') as tmp:
        target = pathlib.Path(tmp)
        for position in range(3):
            data = (directory/f'token-{position}.bin').read_bytes()
            if position == 0:
                changed = bytearray(data)
                changed[64] ^= 1
                data = changed
            (target/f'token-{position}.bin').write_bytes(data)
        try:
            module.load_cpu(target)
        except ValueError as error:
            assert 'independent fixed original-model control' in str(error)
        else:
            raise AssertionError('changed CPU numerical control accepted')
    print('PASS SAME-open pinned actual CPU controls/all3 full traces/one changed numerical word refuses authentication;NOT emitted/GPU numerical match')
