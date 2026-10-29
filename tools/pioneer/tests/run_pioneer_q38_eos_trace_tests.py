#!/usr/bin/env python3
"""Synthetic EOS recorder/independent format checks,not model execution."""
import os
import pathlib
import struct
import subprocess
import tempfile
import compare_pioneer_q38_eos_trace as compare

assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-eos-trace-',dir='/private/tmp') as tmp:
    base=pathlib.Path(tmp);out=base/'output';out.mkdir(mode=0o700);binary=base/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
                    str(here/'pioneer_q38_eos_trace_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(out)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    assert sorted(p.name for p in out.iterdir())==['eos-terminal.bin','eos-token-0.bin','eos-token-1.bin']
    traces=[];refusals=0
    for position in range(2):
        data=(out/f'eos-token-{position}.bin').read_bytes()
        assert len(data)==(16+65*5120+248320+64+1)*4 and data[-4:]==bytes.fromhex('45444e45')
        header=struct.unpack_from('<16I',data)
        assert header==(0x31454f51,1,position,248319 if not position else 17,70253 if not position else 17,0,0x3f800000,0,65,5120,248320,64,2,2,70253,17)
        record=struct.unpack_from('<64I',data,(16+65*5120+248320)*4)
        assert record[8]==record[9]==int(not position)
        traces.append(compare.decode_eos(data,position))
        for offset in (0,4,8,12,16,20,24,28,32,44,48,52,56,60,64,(16+65*5120+248320)*4+9*4,len(data)-4):
            bad=bytearray(data);bad[offset]^=1
            try:compare.decode_eos(bad,position)
            except ValueError:refusals+=1
            else:
                # Numeric payload bit changes can remain structurally valid;
                # the strict CPU comparison still refuses them as non-exact.
                assert offset==64 and compare.decode_eos(bad,position)['bits']!=traces[-1]['bits']
    data=(out/'eos-terminal.bin').read_bytes()
    assert len(data)==(16+64+1)*4 and data[-4:]==bytes.fromhex('45444e45')
    assert struct.unpack_from('<16I',data)==(0x31544551,1,2,3,2,70253,17,119,64,973,0,0,0,0,0,0)
    assert struct.unpack_from('<10I',data,64)==(0x31545451,1,2,2,1,119,64,973,0,1)
    compare.decode_terminal(data,traces)
    for word in range(81):
        if 16+36<=word<16+44:continue # raw counter values have no fabricated expected completion baseline
        bad=bytearray(data);bad[word*4]^=1
        try:compare.decode_terminal(bad,traces)
        except ValueError:refusals+=1
        else:raise AssertionError(('accepted bad EOS terminal',word))
    print('PASS independent EOS header/length/footer/record/terminal family checks;NO normal-trace promotion or model correctness claim')
    print(f'PASS strict EOS parser synthetic corruption refusals{refusals};numeric differences NEVER accepted as original-model equality')
