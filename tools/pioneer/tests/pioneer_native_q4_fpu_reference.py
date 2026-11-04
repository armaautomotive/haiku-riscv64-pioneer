#!/usr/bin/env python3
"""Print96 independently checked DEBUG dot predictions,NEVER weight bytes/device authority."""
import contextlib
import hashlib
import io
import pathlib
import runpy
import struct
import sys
assert __debug__ and len(sys.argv) == 2
private = pathlib.Path(sys.argv[1])
# Executes the unchanged independent Fraction/recipe/header verifier. The
# emitted numbers below are32 diagnostic projections/sample,NOT weights.
with contextlib.redirect_stdout(io.StringIO()):
    runpy.run_path(str(pathlib.Path(__file__).parent / 'pioneer_native_q4_fpu_export_test.py'), run_name='__main__')
pins = ('afa8ae47d574e2a1418b6d7d35e7adf8b4871b5a39ef00c061f0b03bc28461a2',
        'ac6576bcef6d7115abae469fb99bf9d536cb7251cfb53d2d28530d22d697047f',
        '2ac55410694dbb5ce632c89b304eb1de0df8db4fc7963a592b7e0698b2b56d22')
for sample in range(3):
    raw = (private / f'sample{sample}.expected.bin').read_bytes()
    assert len(raw) == 128 and hashlib.sha256(raw).hexdigest() == pins[sample]
    for row, word in enumerate(struct.unpack('<32I', raw)):
        print(f'Q4_FRACTION_REFERENCE sample{sample} first{(0,8704,17376)[sample]} kblock{(0,10,19)[sample]} row{row} word{word:08x}')
