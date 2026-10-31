#!/usr/bin/env python3
"""SYNTHETIC parser fixtures, NEVER evidence of accelerator generation/timing."""
import contextlib
import io
import os
import pathlib
import tempfile
import compare_pioneer_q38_frontier_benchmark_log as c
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
records=[]
for pos in range(c.POSITIONS):
    record=[0]*64
    record[:12]=[0x31435451,1,pos,248319 if pos==0 else 17 if pos==1 else 99+pos,
                100+pos,0x3f800000,pos+1,(pos+1)&1,int(pos==0),int(pos<c.POSITIONS-1),64,973]
    record[12:36]=list(range(112,136));record[44:47]=[pos+1,48,119];records.append(record)
rate='Q38_FRONTIER_OBSERVED_COUNT_RATE_BOUNDS sampled_tokens32 first_committed2 last_committed34 elapsed_min_us319999999 elapsed_max_us320000001 lower_tokens_per_sec0.100000000 upper_tokens_per_sec0.100000000 observations1000;POST_PROMPT_COUNT_WINDOW_NOT_ISOLATED_TOKEN_LATENCY_OR_QUALITY_PROOF'
def lines_for(values):
    lines=[c.HEADER,c.OPEN,f'Q38_FRONTIER_UPLOADED {c.TOTAL_BYTES}/{c.TOTAL_BYTES}',c.START,c.TERMINAL]
    for pos,record in enumerate(values):
        for row in range(8):
            lines.append(f'Q38_FRONTIER_RECORD position{pos} word{row*8:02d}'+''.join(f' {word:08x}' for word in record[row*8:row*8+8]))
    return lines+[c.END,rate]
accepted=refused=0
with tempfile.TemporaryDirectory(prefix='pioneer-frontier34-parser-',dir='/private/tmp') as temporary:
    path=pathlib.Path(temporary)/'SYNTHETIC-NOT-OBSERVED.log'
    def check(lines,okay,available=True):
        global accepted,refused
        path.write_text('\n'.join(lines)+'\n')
        try:actual,bounds=c.read_log(path)
        except ValueError:
            assert not okay;refused+=1;return
        assert okay and actual==records and (bounds is not None)==available;accepted+=1
    original=lines_for(records);check(original,True)
    check(original[:-1]+[c.UNAVAILABLE],True,False)
    for at in range(len(original)):
        check(original[:at]+original[at+1:],False)
        check(original[:at]+[original[at]]+original[at:],False)
    for pos in range(c.POSITIONS):
        for word in (0,1,2,3,6,7,8,9,10,11,12,35,38,44,45,46,47,63):
            changed=[row[:] for row in records];changed[pos][word]^=1;check(lines_for(changed),False)
        for word,value in ((4,248320),(5,0x7fc00000)):
            changed=[row[:] for row in records];changed[pos][word]=value;check(lines_for(changed),False)
    for old,new in (('sampled_tokens32','sampled_tokens31'),('first_committed2','first_committed1'),
                    ('last_committed34','last_committed33'),('elapsed_min_us319999999','elapsed_min_us0'),
                    ('elapsed_min_us319999999','elapsed_min_us320000002'),
                    ('elapsed_max_us320000001','elapsed_max_us9223372036854775808'),
                    ('lower_tokens_per_sec0.100000000','lower_tokens_per_sec0.200000000'),
                    ('upper_tokens_per_sec0.100000000','upper_tokens_per_sec0.100000001'),
                    ('upper_tokens_per_sec0.100000000','upper_tokens_per_secnan'),
                    ('observations1000','observations1'),('observations1000','observations18002')):
        check(original[:-1]+[rate.replace(old,new)],False)
    check([c.HEADER.replace('FRONTIER','DECODE_BENCHMARK')]+original[1:],False)
    check(original[:-2]+[c.END.replace('final_count34','final_count33'),rate],False)
    cpu=[{'header':[0,0,pos,records[pos][3],records[pos][4],0,records[pos][5]]} for pos in range(3)]
    with contextlib.redirect_stdout(io.StringIO()):
        assert c.compare_first_three(cpu,records);accepted+=1
        for pos in range(3):
            for word in (3,4,5):
                changed=[row[:] for row in records];changed[pos][word]^=1
                assert not c.compare_first_three(cpu,changed);refused+=1
print(f'PASS FRONTIER34 comparator/{accepted} SYNTHETIC positives/{refused} omissions+duplicates+record+counter+time+rate+numeric refusals;strict first3 numerical comparison unchanged;NO physical/timing/quality proof')
