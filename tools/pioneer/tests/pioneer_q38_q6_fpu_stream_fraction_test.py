#!/usr/bin/env python3
"""Independent original-Q6 Fraction full-K predictions of CLOSED RV32 outputs."""
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
    for i in range(6720):
        seed^=(seed<<13)&0xffffffff;seed^=seed>>17;seed^=(seed<<5)&0xffffffff;data.append(seed&255)
    for row in range(32):
        high=row*210+209
        if data[high]&0x7c==0x7c:data[high]^=4
        if trial==1:data[high-1]=row+1;data[high]=0x80 if row&1 else 0
    return data
blocks=[original(i) for i in range(24)];phases=words=weights=0;widths=set()
for path in map(pathlib.Path,sys.argv[1:]):
    text=path.read_text();actual={};width=None
    for line in text.splitlines():
        if not line.startswith('Q6_STREAM_REFERENCE'):continue
        m=re.fullmatch(r'Q6_STREAM_REFERENCE width(5120|6144) pass([01]) row(\d+) word([0-9a-f]{8})',line);assert m
        k,job,row=map(int,m.group(1,2,3));assert row<32 and (job,row) not in actual and width in (None,k)
        width=k;actual[job,row]=int(m[4],16)
    assert width and width not in widths and len(actual)==64 and f'PASS actual RV32 Q6 fullK{width} stream:two jobs/all64 exact,' in text
    widths.add(width)
    for row in range(32):
        total=0
        for block in range(width//256):
            q=blocks[block][row*210:(row+1)*210];d=half(q[208]|(q[209]<<8));a=[];b=[]
            for c in range(256):
                h,g,lane=c//128,c%128//32,c%32
                low=(q[h*64+(g&1)*32+lane]>>(g//2*4))&15
                high=(q[128+h*32+lane]>>(g*2))&3
                quant=(low|(high<<4))-32;scale=q[192+c//16];scale=scale-256 if scale>=128 else scale
                value=d*scale*quant;bits=r['encode_normal'](value)
                if not value:bits=int(bool(q[209]&128)^(scale<0)^(quant<0))<<31
                high16,tail=divmod(bits,65536);high16+=int(tail>32768 or (tail==32768 and high16&1));a.append(high16)
                b.append(0x3e80+(c*19+block*11)%384+(0x8000 if c&3==2 else 0));weights+=1
            for first in range(0,256,16):
                for phase in range(4):total=r['instruction'](a[first:first+16],b[first:first+16],phase,total);phases+=1
        for job in range(2):assert total==actual[job,row],(width,job,row,hex(total),hex(actual[job,row]));words+=1
    print(f'PASS independent Fraction FULL Q6 K{width}:all64 actual RV32 words,continuous rounded accumulation across ALL K256 blocks,original ql/qh/K16 signed scales;no CXX codec/preparer/SFPU/FPU oracle shared',flush=True)
assert widths=={5120,6144} and (words,weights,phases)==(128,360448,90112)
print('PASS independent Q6 full-K stream:128 returned words/360448 synthetic original weights/90112 rounded phases;NO native/hardware/full-model-quality/speed proof')
