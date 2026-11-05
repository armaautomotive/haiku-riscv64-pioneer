#!/usr/bin/env python3
"""Independent Fraction FULL-K predictions of actual CLOSED RV32 outputs."""
import contextlib
import io
import pathlib
import re
import runpy
import sys
assert __debug__ and len(sys.argv)==3
with contextlib.redirect_stdout(io.StringIO()):
    r=runpy.run_path(str(pathlib.Path(__file__).parent/'pioneer_native_bfp_arithmetic_test.py'))
def half(bits):
    e,m=(bits>>10)&31,bits&1023;assert e!=31
    return (-1 if bits&0x8000 else 1)*(1024+m if e else m)*r['power_of_two'](e-25 if e else -24)
def original(trial):
    seed=0x712f3b49+trial;data=[]
    for i in range(4608):
        seed^=(seed<<13)&0xffffffff;seed^=seed>>17;seed^=(seed<<5)&0xffffffff;data.append(seed&255)
    for row in range(32):
        for at in (0,2):
            high=row*144+at+1
            if data[high]&0x7c==0x7c:data[high]^=4
            if trial==1:data[high-1]=row+1;data[high]=0x80 if row&1 else 0
    return data
blocks=[original(i) for i in range(68)];phases=words=weights=0
for path in map(pathlib.Path,sys.argv[1:]):
    text=path.read_text();actual={};width=None
    for line in text.splitlines():
        if not line.startswith('Q4_COMPACT_STREAM_REFERENCE'):continue
        m=re.fullmatch(r'Q4_COMPACT_STREAM_REFERENCE width(5120|17408) pass([01]) row(\d+) word([0-9a-f]{8})',line);assert m
        k,job,row=map(int,m.group(1,2,3));assert row<32 and (job,row) not in actual and width in (None,k)
        width=k;actual[job,row]=int(m[4],16)
    assert width and len(actual)==64 and f'PASS actual RV32 compact Q4 fullK{width} stream:two jobs/all64 exact,' in text
    for row in range(32):
        total=0
        for block in range(width//256):
            q=blocks[block][row*144:(row+1)*144];d,minimum=half(q[0]|(q[1]<<8)),half(q[2]|(q[3]<<8));a=[];b=[]
            for c in range(256):
                group,column=divmod(c,32)
                if group<4:scale,low=q[4+group]&63,q[8+group]&63
                else:scale=(q[8+group]&15)|((q[group]>>6)<<4);low=(q[8+group]>>4)|((q[4+group]>>6)<<4)
                quant=(q[16+(group//2)*32+column]>>(4*(group&1)))&15
                bits=r['encode_normal'](d*scale*quant-minimum*low);high,tail=divmod(bits,65536)
                high+=int(tail>32768 or (tail==32768 and high&1));a.append(high)
                b.append(0x3e80+(c*19+block*11)%384+(0x8000 if c&3==2 else 0));weights+=1
            for first in range(0,256,16):
                for phase in range(4):total=r['instruction'](a[first:first+16],b[first:first+16],phase,total);phases+=1
        for job in range(2):assert total==actual[job,row],(width,job,row,hex(total),hex(actual[job,row]));words+=1
    print(f'PASS independent Fraction FULL compact Q4 K{width}:all64 actual RV32 words,continuous rounded accumulation across ALL K256 blocks;no CXX codec/preparer/SFPU/FPU oracle shared',flush=True)
assert (words,weights,phases)==(128,720896,180224)
print('PASS independent compact Q4 full-K stream:128 returned words/720896 synthetic weights/180224 rounded phases;NO native/hardware/full-model-quality/speed proof')

