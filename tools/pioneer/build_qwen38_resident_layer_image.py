#!/usr/bin/env python3
"""Complete recurrent-layer device program ONLY;NO native/card/Work grant."""
import hashlib
import os
import pathlib
import re
import runpy
import struct
import subprocess
import sys

if len(sys.argv)!=3 or any(k.startswith('PIONEER_') and v!='0' for k,v in os.environ.items()):
    raise SystemExit(2)
sdk,output=map(pathlib.Path,sys.argv[1:])
assert sdk.is_absolute() and output.is_absolute() and not output.exists() and not output.is_symlink()
here=pathlib.Path(__file__).resolve().parent
kernel=here.parents[1]/'src/add-ons/kernel/drivers/misc'
clean={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
output.mkdir(mode=0o700)
saved=list(sys.argv)
try:
    # ALL current68 ancestors are rebuilt/authenticated first. None is
    # relabelled into a new native image or used as an ownership receipt.
    sys.argv=[str(here/'build_qwen38_resident_output_cohort_image.py'),str(sdk),str(output/'unchanged-output')]
    base=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
for key,(section,size,digest,_) in runpy.run_path(str(here/'generate_qwen38_resident_output_images.py'))['PINS'].items():
    path=output/'unchanged-output'
    if key.startswith('peer-'):
        path=path/'exact-companion'
    data=(path/(section+'.bin')).read_bytes()
    assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,key
# The only shared source edit is an exactly gated FFN subroutine entry.
# Rebuild the OLD default QFG3 and compare every byte to its frozen DATA.
oldffn=output/'unchanged-ffn'
subprocess.run(['sh',str(here/'build_qwen38_ffn_graph_image.sh'),str(sdk),str(oldffn)],check=True,
               env={**clean,'PIONEER_Q38_FFN_GRAPH_ROOT':'1','PIONEER_Q38_FFN_GRAPH_RESIDUAL_ROOT':'1'})
for section,suffix in (('text','Main'),('prefix','Prefix'),('graph','Graph')):
    text=(kernel/('PioneerQ38FfnResidualRoot'+suffix+'.h')).read_text()
    words=[int(w,16) for w in re.findall(r'0x([0-9a-fA-F]{8})U',text)]
    assert words and (oldffn/(section+'.bin')).read_bytes()==struct.pack('<'+'I'*len(words),*words),section
cross=base['cross'];flags=list(base['flags'])
roles=[*base['roles'],'-DPIONEER_Q38_RESIDENT_LAYER_ROOT_IMAGE=1',
       '-include',str(here/'PioneerQ38ResidentLayerRootImageRole.h')]
subprocess.run([str(cross)+'-g++',*flags,'-O3',*roles,'-DPIONEER_Q38_COHORT_POLL_BACKOFF=1',
                '-c',str(here/'blackhole_qwen38_resident_layer.cpp'),'-o',str(output/'root.o')],check=True,env=clean)
matrix_flags=['-DPIONEER_Q38_PROJECTION_MATRIX_ROOT=1','-DPIONEER_Q38_FFN_RESIDENT_MATRIX',
 '-DPIONEER_Q38_COMPACT_ACTIVATIONS','-DPIONEER_Q38_REUSE_ACTIVATIONS','-DPIONEER_Q38_WORK_COHORT_CACHE',
 '-DPIONEER_Q38_WORK_SKIP_DOT_CHECK','-DPIONEER_Q38_WORK_SPARSE_EXPORT','-DPIONEER_Q38_WORK_COMPACT_OUTPUT',
 '-DPIONEER_Q38_ROOT_CYCLE_PROFILE','-DPIONEER_Q38_WORK_SENTINEL_POLL',
 '-Dblock_main=q38_resident_output_matrix_once','-Dblock_fault_park=cohort_fault_park']
subprocess.run([str(cross)+'-g++',*flags,'-O2','-fomit-frame-pointer',*roles,*matrix_flags,
                '-c',str(here/'blackhole_qwen38_dispatch.cpp'),'-o',str(output/'matrix.o')],check=True,env=clean)
subprocess.run([str(cross)+'-g++',*flags,'-O2','-DPIONEER_Q38_RESIDENT_LAYER_NORM_IMAGE=1',
                '-c',str(here/'blackhole_qwen38_resident_layer_norm.cpp'),'-o',str(output/'norm.o')],check=True,env=clean)
ffn_flags=['-DPIONEER_Q38_FFN_GRAPH_ROOT=1','-DPIONEER_Q38_FFN_GRAPH_RESIDUAL_ROOT=1','-DPIONEER_Q38_RESIDENT_LAYER_FFN_IMAGE=1',
 '-DPIONEER_Q38_FFN_RESIDENT_MATRIX','-DPIONEER_Q38_COMPACT_ACTIVATIONS','-DPIONEER_Q38_REUSE_ACTIVATIONS',
 '-DPIONEER_Q38_WORK_COHORT_CACHE','-DPIONEER_Q38_WORK_SKIP_DOT_CHECK','-DPIONEER_Q38_WORK_SPARSE_EXPORT',
 '-DPIONEER_Q38_WORK_COMPACT_OUTPUT','-DPIONEER_Q38_ROOT_CYCLE_PROFILE','-DPIONEER_Q38_WORK_SENTINEL_POLL',
 '-Dblock_fault_park=cohort_fault_park']
subprocess.run([str(cross)+'-g++',*flags,'-O2','-fomit-frame-pointer',*ffn_flags,
                '-c',str(here/'blackhole_qwen38_ffn_graph.cpp'),'-o',str(output/'ffn.o')],check=True,env=clean)
subprocess.run([str(cross)+'-g++',*flags,'-O2','-fomit-frame-pointer',
                '-Dscalar_main=q38_ffn_graph_gather_once','-Dscalar_fault_park=cohort_fault_park',
                '-c',str(here/'blackhole_qwen38_ffn_resident_gather2.cpp'),'-o',str(output/'gather.o')],check=True,env=clean)
subprocess.run([str(cross)+'-as','-march=rv32imf','-mabi=ilp32','-mno-relax',
                str(here/'blackhole_qwen38_resident_layer_start.S'),'-o',str(output/'start.o')],check=True,env=clean)
subprocess.run([str(cross)+'-ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',
                str(here/'blackhole_qwen38_resident_layer.ld'),
                *[str(output/(n+'.o')) for n in ('start','root','matrix','norm','ffn','gather')],
                str(output/'unchanged-output/unchanged-cohort/root_conv.o'),'-Map',str(output/'program.map'),
                '-o',str(output/'program.elf')],check=True,env=clean)
sections=('boot','text','conv','matrix','cohort','norm','ffn')
for section in sections:
    subprocess.run([str(cross)+'-objcopy','-O','binary','-j','.'+section,
                    str(output/'program.elf'),str(output/(section+'.bin'))],check=True,env=clean)
for tool,args,name in (('nm',['-n'],'program.symbols'),('nm',['-u'],'program.undefined'),
                       ('objdump',['-d'],'program.disassembly'),('objdump',['-h'],'program.sections')):
    with (output/name).open('w') as stream:
        subprocess.run([str(cross)+'-'+tool,*args,str(output/'program.elf')],stdout=stream,check=True,env=clean)
assert not (output/'program.undefined').read_bytes()
symbols=(output/'program.symbols').read_text();disassembly=(output/'program.disassembly').read_text()
for symbol,address in (('cohort_entry',0x72000),('cohort_done',0x7200c),('cohort_park',0x72038),('cohort_fault_park',0x72060)):
    assert re.search('^'+format(address,'08x')+' T '+symbol+'$',symbols,re.M),(symbol,address)
terminal=(output/'text.bin').read_bytes()
assert len(terminal)==128 and terminal[0x38:0x40]==struct.pack('<II',0x0000006f,0xffdff06f)
assert terminal[0x40:0x60]==bytes(32)
assert terminal[0x24:0x2c]==struct.pack('<II',0x31525337,0xc5130313) # NEW QLR1,never old QCO1
assert terminal[0x68:0x70]==struct.pack('<II',0x46525337,0xc5130313) # NEW QLRF,not old QCOF
for name in ('q38_resident_cohort_main','q38_resident_layer_prefix_once','q38_resident_layer_norm_once','q38_resident_layer_ffn_once','q38_ffn_graph_matrix_once','q38_ffn_graph_gather_once'):
    assert re.search(r'^[0-9a-f]+ T '+name+'$',symbols,re.M),name
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s)\b',disassembly)
# Two independently linked matrix components each contain the SAME three
# read-only mcycleh/mcycle/mcycleh words. The old single-component gate
# remains unchanged;do not mistake a count mismatch for an illegal CSR.
csr=[]
for line in disassembly.splitlines():
    match=re.match(r'\s*([0-9a-f]+):\s+([0-9a-f]{8})\s',line)
    if not match:
        continue
    address,word=int(match[1],16),int(match[2],16)
    if word&127!=0x73:
        continue
    assert (word>>12)&7==2 and (word>>15)&31==0 and word>>20 in (0xb00,0xb80) and (word>>7)&31,hex(word)
    csr.append((address,word>>20))
assert len(csr)==6 and [v for _,v in csr]==[0xb80,0xb00,0xb80]*2,csr
for group,(first,last) in enumerate(((0x68000,0x6e000),(0x15c000,0x17e000))):
    a=csr[group*3:group*3+3]
    assert first<=a[0][0] and a[2][0]<last and a[1][0]==a[0][0]+4 and a[2][0]==a[0][0]+8,a
print('PASS ACTUAL layer opcodes:exactly2triplets/6 READ-only CSRRS mcycleh/mcycle/mcycleh,x0;NO CSR/control/counter writes')
rows=(output/'program.sections').read_text().splitlines()
for i,row in enumerate(rows):
    match=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+',row)
    if match and i+1<len(rows) and 'ALLOC' in rows[i+1]:
        assert match[1][1:] in sections,'unexported allocated section:'+row
assert (output/'boot.bin').read_bytes()==struct.pack('<I',0x0006c06f)
frames=base['frames'];maximum=0
inherited={}
for directory,function in ((oldffn,'q38_ffn_graph_matrix_once'),(output/'unchanged-output','q38_resident_output_matrix_once')):
    table=frames(directory/('program.su' if directory==oldffn else 'matrix.su'))
    entries=[size for name,size in table.items() if name.endswith(':unsigned int '+function+'()')]
    assert len(entries)==1,(function,entries)
    inherited[function]=entries[0]
for obj in ('root','norm','ffn','gather','matrix'):
    table=frames(output/(obj+'.su'));assert table
    for function,size in table.items():
        # The inherited matrix ROOT has a larger,previously bounded frame.
        limit=2048
        for name,old_size in inherited.items():
            if function.endswith(':unsigned int '+name+'()'):
                limit=old_size
        assert size<=limit,(obj,function,size,limit)
        maximum=max(maximum,size)
for section in sections:
    data=(output/(section+'.bin')).read_bytes()
    print(hashlib.sha256(data).hexdigest()+' '+section+' bytes'+str(len(data)))
print('PASS COMPILED complete on-card recurrent-layer coordinator:Conv/GDN/output->residual/RMS->gate/up/SiLU/down/residual;maxFrame'+str(maximum))
print('NEW footprint/native lifetime/instruction execution/original-model/physical qualification REQUIRED;NO native registration/Work/full64layer/token-loop or token-rate grant')
