#!/usr/bin/env python3
"""Print288 independently checked DEBUG dot values,NEVER weights/GPU authority."""
import contextlib
import hashlib
import io
import pathlib
import runpy
import struct
import sys
assert __debug__ and len(sys.argv)==2
private=pathlib.Path(sys.argv[1])
with contextlib.redirect_stdout(io.StringIO()):
    runpy.run_path(str(pathlib.Path(__file__).parent/'pioneer_native_q4_fpu_stream_export_test.py'),run_name='__main__')
pins=('7e7d73b158e0032a15678e63963c481d73ffe3fa911982b23a1bce0c8fdb722b',
      '29c72879b3ce2df3209ff4d660e844af55ad35edba4c1ff1d9e1acfee1dbabb0',
      '88162c7715d7f2b93d842e8f4c022ba3328dc25936e3aa52aaa4d42c822bd1b1',
      'c4c7956ae893e6497f74ddac01546fb0224e2bdbf5c5bbc73474101e06131f48',
      'e465586fe57321921f81a3493b0015ef219fb79c12a1d00e96bea89d7cfc588b',
      '1726e706af75657bd687991ef73e00a20a50302245df70ba1b6252f484e0928b',
      'a37b56ed1e2e42532d3b8904a3fa973ccef36c161dbb29cd3ae0a43096e5c1aa',
      '214437ee7708d8126241adff5cbcd88c1b56c5a0716a06f72b11bad5e7dff8fd',
      'fac4055a71058e423ba0293b94772587aaea032c44a4296661e9d3ecca42d903')
for sample in range(9):
    raw=(private/f'sample{sample}.expected.bin').read_bytes()
    assert len(raw)==128 and hashlib.sha256(raw).hexdigest()==pins[sample]
    tensor=7 if sample<3 else 8 if sample<6 else 6
    width=5120 if sample<6 else 17408
    first=(0,8704,17376)[sample%3] if sample<6 else (0,2560,5088)[sample%3]
    for row,word in enumerate(struct.unpack('<32I',raw)):
        print(f'Q4_STREAM_FRACTION_REFERENCE sample{sample} tensor{tensor} first{first} width{width} row{row} word{word:08x}')
