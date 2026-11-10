#!/usr/bin/env python3
"""Independent Fraction arithmetic over PRIVATE generated real-model bands."""
import contextlib
import hashlib
import io
import pathlib
import re
import runpy
import struct
import sys
if not __debug__ or len(sys.argv) not in (2,3) or (len(sys.argv)==3 and sys.argv[2]!='--qgwd-dense'):
    raise SystemExit('Use non-optimized Python PRIVATE_EXPORT_DIRECTORY [--qgwd-dense]')
here = pathlib.Path(__file__).resolve().parent
folder = pathlib.Path(sys.argv[1])
data = (folder / 'PioneerNativeBfpModelBandData.h').read_bytes()
metadata = (folder / 'PioneerNativeBfpModelBandMetadata.h').read_bytes()
dense = len(sys.argv)==3
first_rows = (0,512,992) if dense else (0,8704,17376)
if dense:
    assert b'Width=5120,Rows=1024,Tensor=45,Samples=3' in metadata
    assert b'const unsigned rows[3]={0,512,992}' in metadata
def array(source, name, count):
    match = re.search(r'const uint32_t '+name+r'\['+str(count)+r'\]=\{([^}]+)\}', source.decode())
    assert match
    words = [int(x, 16) for x in re.findall(r'0x([0-9a-f]{8})', match.group(1))]
    assert len(words) == count
    return words
inputs = struct.pack('<3200I', *array(data, 'input', 3200))
with contextlib.redirect_stdout(io.StringIO()):
    rational = runpy.run_path(str(here / 'pioneer_native_bfp_arithmetic_test.py'))
instructions = 0
for sample in range(3):
    weights = struct.pack('<44160I', *array(data, 'w'+str(sample), 44160))
    expected = array(metadata, 'g'+str(sample), 32)
    digest = hashlib.sha256(weights + inputs).hexdigest()
    assert f'if(sample=={sample})return "{digest}";' in metadata.decode()
    for tile in range(160):
        assert weights[tile * 1104:tile * 1104 + 16] == bytes(16)
        assert inputs[tile * 80:tile * 80 + 16] == bytes(16)
    for row in range(32):
        total = 0
        for tile in range(160):
            packed = weights[tile * 1104 + 16:tile * 1104 + 1104]
            a = []
            for c in range(32):
                group = (row // 16) * 32 + (c // 16) * 16 + row % 16
                code = packed[64 + ((row // 16) * 2 + c // 16) * 256 + (row % 16) * 16 + c % 16]
                magnitude = code & 127
                if not magnitude:
                    assert code == 0
                    a.append(0)
                else:
                    top = magnitude.bit_length() - 1
                    exponent = packed[group] - 6 + top
                    assert 0 < exponent < 255
                    a.append(((code & 128) << 8) | (exponent << 7) | ((magnitude << (7 - top)) - 128))
            b = struct.unpack('<32H', inputs[tile * 80 + 16:tile * 80 + 80])
            for first in (0, 16):
                for phase in range(4):
                    total = rational['instruction'](a[first:first + 16], b[first:first + 16], phase, total)
                    instructions += 1
        assert total == expected[row], (sample, row, hex(total), hex(expected[row]))
        print(f'LEARNED_FRACTION_REFERENCE sample{sample} first{first_rows[sample]} row{row} word{total:08x}')
    print(f'PASS_PRIVATE_LEARNED_FRACTION sample{sample} all32_words exact payload_sha256{digest};NO_DEVICE_MODEL_QUALITY_SPEED_PROOF', flush=True)
print(f'PASS_PRIVATE_LEARNED_FRACTION samples3 words96 phase_instructions{instructions};NO_CXX_PACK_UNPACK_OR_INTEGER_REFERENCE_CALL,NO_PRIVATE_MODEL_BYTES_PRINTED')
