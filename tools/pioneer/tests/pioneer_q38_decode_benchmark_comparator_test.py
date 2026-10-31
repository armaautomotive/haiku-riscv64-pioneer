#!/usr/bin/env python3
"""Synthetic parser fixtures only, NEVER archived as observed accelerator DATA."""
import contextlib
import io
import os
import pathlib
import tempfile
import compare_pioneer_q38_decode_benchmark_log as c
assert not any(key.startswith(('PIONEER_','TT_PIONEER_')) for key in os.environ)
records=[]
for pos in range(c.POSITIONS):
    record=[0]*64
    record[:12]=[0x31435451,1,pos,248319 if pos==0 else 17 if pos==1 else 99+pos,
                 100+pos,0x3f800000,pos+1,(pos+1)&1,int(pos==0),int(pos<c.POSITIONS-1),64,973]
    record[12:36]=list(range(112,136));record[44:47]=[pos+1,48,119];records.append(record)
def lines_for(values):
    lines=[c.HEADER,c.OPEN,f'Q38_DECODE_BENCHMARK_UPLOADED {c.TOTAL_BYTES}/{c.TOTAL_BYTES}',c.START,c.TERMINAL]
    for pos,record in enumerate(values):
        for row in range(8):
            lines.append(f'Q38_DECODE_BENCHMARK_RECORD position{pos} word{row*8:02d}'+''.join(f' {word:08x}' for word in record[row*8:row*8+8]))
    return lines+[c.END]
accepted=refused=0
with tempfile.TemporaryDirectory(prefix='pioneer-decode34-parser-',dir='/private/tmp') as temporary:
    path=pathlib.Path(temporary)/'SYNTHETIC-NOT-OBSERVED.log'
    def check(lines,okay):
        global accepted,refused
        path.write_text('\n'.join(lines)+'\n')
        try:actual=c.records_from_log(path)
        except ValueError:
            assert not okay;refused+=1;return
        assert okay and actual==records;accepted+=1
    original=lines_for(records);check(original,True)
    for at in range(len(original)):
        check(original[:at]+original[at+1:],False)
        check(original[:at]+[original[at]]+original[at:],False)
    for pos in range(c.POSITIONS):
        for word in (0,1,2,3,6,7,8,9,10,11,12,35,38,44,45,46,47,63):
            changed=[row[:] for row in records];changed[pos][word]^=1;check(lines_for(changed),False)
        for word,value in ((4,248320),(5,0x7fc00000)):
            changed=[row[:] for row in records];changed[pos][word]=value;check(lines_for(changed),False)
    for old in ('DEBUG74_FIXED_2PROMPT_3POSITION_DIAGNOSTIC_NOT_ASSISTANT_WORK_OR_TOKEN_RATE',
                'DEBUG75_GENERATION_FIXED_2PROMPT_3POSITION_DIAGNOSTIC_NO_CPU_MODEL_REFERENCE_NOT_ASSISTANT_WORK_OR_TOKEN_RATE'):
        check([old]+original[1:],False)
    for text in ('Q38_DECODE_BENCHMARK_UPLOADED 1/19399024640','Q38_DECODE_BENCHMARK_UPLOADED 19399024640/1',
                 'Q38_DECODE_BENCHMARK_UPLOADED 19399024640/19399024640 trailing'):
        check(original[:2]+[text]+original[3:],False)
    cpu=[{'header':[0,0,pos,records[pos][3],records[pos][4],0,records[pos][5]]} for pos in range(3)]
    with contextlib.redirect_stdout(io.StringIO()):
        assert c.compare_first_three(cpu,records);accepted+=1
        for pos in range(3):
            for word in (3,4,5):
                changed=[row[:] for row in records];changed[pos][word]^=1
                assert not c.compare_first_three(cpu,changed);refused+=1
print(f'PASS DECODE34 DATA comparator/{accepted} SYNTHETIC positives/{refused} missing+duplicate+cross-family+row+feedback+flags+reserved+numeric refusals;unchanged exact FIRST-THREE control,NO later-position numerical or native/physical/Work/TPS claim')
