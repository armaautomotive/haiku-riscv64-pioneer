#!/usr/bin/env python3
"""Audit actual linked call edges,NOT execution/closure/NoC timing authority."""
import ast
import hashlib
import os
import pathlib
import re
import struct
import subprocess
import sys
assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
old,new=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
def literal(path,name):
    return next(ast.literal_eval(n.value) for n in ast.parse(path.read_text()).body
                if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id==name for t in n.targets))
for directory,expected in ((old,literal(here.parent/'build_qwen38_token_fault_marker_image.py','FRONTIER_PINS')),
                            (new,literal(here.parent/'build_qwen38_retained_cohort_root.py','PINS'))):
    assert directory.is_absolute() and not directory.is_symlink() and expected
    for name,size,digest in expected:
        data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
unresolved_jumps=[]
def sign(value,bits):return value-(1<<bits) if value&(1<<(bits-1)) else value
def functions(directory):
    result={}
    for line in subprocess.check_output([tool+'nm','-S','-nC','--defined-only',str(directory/'program.elf')],text=True).splitlines():
        row=line.split(None,3)
        if len(row)==4 and row[2] in ('t','T','w','W'):
            start,size=int(row[0],16),int(row[1],16)
            if size:result[start]=(size,row[3])
    # GNU nm -S omits the size column entirely for these assembly labels.
    plain=subprocess.check_output([tool+'nm','-nC','--defined-only',str(directory/'program.elf')],text=True)
    for start,name in ((0x6000,'_start'),(0x40000,'scalar_entry'),
                       (0x60000,'scalar_fault_park'),(0x60004,'q38_global_chain_stopped')):
        assert re.search('^'+format(start,'08x')+' T '+name+'$',plain,re.M)
        result.setdefault(start,(0,name))
    return result
def edges(directory,needle):
    table=functions(directory);matches=[(start,size,name) for start,(size,name) in table.items() if name.startswith(needle)]
    assert len(matches)==1,(needle,matches)
    start,size,name=matches[0];assert size and not size&3
    base=0x40000 if start<0x60000 else 0x64000
    data=(directory/('text.bin' if base==0x40000 else 'conv.bin')).read_bytes()
    words=struct.unpack('<'+'I'*(size//4),data[start-base:start-base+size]);result=[]
    for index,word in enumerate(words):
        pc=start+index*4;opcode=word&127;rd=(word>>7)&31;target=None
        if opcode==0x6f:
            immediate=((word>>31)<<20)|(((word>>21)&1023)<<1)|(((word>>20)&1)<<11)|(((word>>12)&255)<<12)
            target=(pc+sign(immediate,21))&0xffffffff
            if not rd and start<=target<start+size:continue
            assert rd in (0,1)
        elif opcode==0x67:
            rs1=(word>>15)&31;immediate=sign(word>>20,12)
            if rd==0 and rs1==1 and not immediate:continue # actual return,not a call
            assert index and rd in (0,1)
            previous=words[index-1]
            if previous&127!=0x17 or ((previous>>7)&31)!=rs1:
                # Indexed switch in StageConv is NOT an ABI call. Keep it
                # explicitly unresolved;this is a direct-call-site audit,
                # never whole-function control-flow/path feasibility proof.
                assert rd==0 and name.startswith('MixedRecurrentCohort::StageConv(')
                assert previous&127==3 and ((previous>>7)&31)==rs1
                unresolved_jumps.append((str(directory),name,pc));continue
            target=(pc-4+sign(previous&0xfffff000,32)+immediate)&0xfffffffe
        if target is not None:
            assert target in table,(name,hex(pc),hex(target),'not an actual function entry')
            result.append((pc,table[target][1]))
    return result
prefix='MixedRecurrentCohort::'
warm=[prefix+name+'(' for name in ('Wait','BootSeal','Closed','CounterSeal')]
old_edges=edges(old,prefix+'StageGdn(');new_edges=edges(new,prefix+'StageGdnAfterClosedConv(')
for name in warm:
    assert sum(callee.startswith(name) for _,callee in old_edges)==1,name
    assert not any(callee.startswith(name) for _,callee in new_edges),name
for directory in (old,new):
    finish=edges(directory,prefix+'Finish(')
    for name in warm:assert sum(callee.startswith(name) for _,callee in finish)==1,(directory,name)
    conv=edges(directory,prefix+'StageConv(')
    assert sum(callee.startswith(prefix+'Finish(') for _,callee in conv)==1
    wrapper=edges(directory,'q38_mixed_recurrent_cohort')
    calls=[callee for _,callee in wrapper]
    conv_at=next(i for i,name in enumerate(calls) if name.startswith(prefix+'StageConv('))
    gdn_at=next(i for i,name in enumerate(calls) if name.startswith(prefix+('StageGdn(' if directory==old else 'StageGdnAfterClosedConv(')))
    assert conv_at<gdn_at
# Release is inlined in both StageGdn bodies;the real instruction-history
# tests,not an imaginary out-of-line call edge,check the96 actual releases.
for name in (prefix+'Finish(',prefix+'Audit('):
    assert any(callee.startswith(name) for _,callee in new_edges),name
assert len(unresolved_jumps)==2
print('PASS actual linked old951/new48b2 RV32 call targets/old warm pre-GDN Wait+BootSeal+Closed+CounterSeal each1 call site/new each0/'
      'both Conv still call Finish,both Finish still call ALL4 completion checks/actual wrapper Conv-before-GDN order;'
      '2 StageConv indexed switch jumps explicitly UNRESOLVED;static direct edges ONLY,NOT path feasibility,actual96/119 loop execution,numerical,NoC counts,physical speed or Work proof')
