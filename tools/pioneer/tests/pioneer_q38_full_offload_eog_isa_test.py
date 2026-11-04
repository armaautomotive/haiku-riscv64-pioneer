#!/usr/bin/env python3
"""Execute the ACTUAL emitted RV32 EOG predicate over every vocabulary ID.

This tiny integer interpreter is not an accelerator,model reference or a
native lifecycle test. It independently checks the new on-card stop policy.
"""
import pathlib
import re
import subprocess
import sys

assert len(sys.argv)==3
tool=str(pathlib.Path(sys.argv[1])/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
directory=pathlib.Path(sys.argv[2])
symbols=subprocess.check_output([tool+'nm','-SC','--defined-only',str(directory/'program.elf')],text=True)
matches=re.findall(r'^([0-9a-f]+) ([0-9a-f]+) t GlobalChain::FullOffloadEog\(unsigned int\)$',symbols,re.M)
assert len(matches)==1
start,size=(int(n,16) for n in matches[0]);raw=(directory/'text.bin').read_bytes()
assert 0x40000<=start<start+size<=0x60000 and size%4==0
body=raw[start-0x40000:start-0x40000+size]
assert len(body)==size
words={start+i:int.from_bytes(body[i:i+4],'little') for i in range(0,size,4)}
mask=0xffffffff
def signed(value,bits):return value-(1<<bits) if value>>(bits-1) else value
def execute(value):
    registers=[0]*32;registers[1]=0xdeadbeec;registers[2]=0x20000;registers[8]=0x128769ab;registers[10]=value
    memory={};pc=start
    for _ in range(32):
        assert pc in words
        word=words[pc];op=word&127;rd=word>>7&31;funct=word>>12&7;rs1=word>>15&31;rs2=word>>20&31
        next_pc=pc+4;immediate=signed(word>>20,12);a,b=registers[rs1],registers[rs2]
        if op==0x37:registers[rd]=word&0xfffff000
        elif op==0x13:
            if funct==0:registers[rd]=(a+immediate)&mask
            elif funct==7:registers[rd]=a&(immediate&mask)
            else:raise AssertionError(('unexpected immediate',hex(word)))
        elif op==0x33:
            assert word>>25==0
            if funct==0:registers[rd]=(a+b)&mask
            elif funct==5:registers[rd]=a>>(b&31)
            else:raise AssertionError(('unexpected register',hex(word)))
        elif op==0x23:
            assert funct==2
            offset=signed((word>>25)<<5|(word>>7&31),12);address=(a+offset)&mask
            assert address%4==0 and 0x1fff0<=address<0x20000;memory[address]=b
        elif op==0x03:
            assert funct==2;address=(a+immediate)&mask
            assert address in memory;registers[rd]=memory[address]
        elif op==0x63:
            assert funct==6
            offset=(word>>31)<<12|((word>>7)&1)<<11|((word>>25)&63)<<5|((word>>8)&15)<<1
            if a<b:next_pc=pc+signed(offset,13)
        elif op==0x67:
            assert rd==0 and funct==0 and rs1==1 and immediate==0
            assert a==0xdeadbeec and registers[2]==0x20000 and registers[8]==0x128769ab
            return registers[10]
        else:raise AssertionError(('unexpected opcode',hex(word)))
        registers[0]=0;pc=next_pc
    raise AssertionError('predicate did not return')
expected={248044,248046,248063,248064,248065}
for value in range(248320):assert execute(value)==int(value in expected),value
for value in (248320,248321,0x7fffffff,0x80000000,0xffffffff):assert execute(value)==0,value
print('PASS actual emitted RV32 full-card EOG predicate:248320 vocabulary IDs+5 out-of-range;exact5 stop IDs;stack/callee register preserved;NO accelerator or model inference')
