#!/usr/bin/env python3
"""Build matched vector92 recurrent ROOT images. No driver or device I/O."""
from hashlib import sha256
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

from build_qwen38_vector92_ffn_sender import SEED
from generate_qwen38_token_programs import authenticated, pins


ROLE='PIONEER_Q38_VECTOR92_FFN_ROOT'
MATCHED = {
    'reuse': (
        (130188,'203bb3d2b44231210526778d526704b3981e18d5367e8074aaf7809b9055686a'),
        (32680,'f99421673e4eba9092fac2a8dbea05219a7adaacaa3e540dfb122a191a3b5808'),
        (191492,'6258db4e890ec1f1719fad2248ee3994fe6ce4463ce3d938c36d96af125a2010')),
    'refetch': (
        (130076,'e4b737e072a22a86bf8d3c6e6dc1b666e0e73fab36e5602e8f73d61f72da9223'),
        (32680,'3b923e247d81e3d221b3e77e9f3a0156053de60ceeaa913996bb88e17b13869c'),
        (191492,'0fcec876160c9a8b08a584d3f60d8eacee39b0fa96bcac1ce7226b03de59493e')),
}


def replace(text,old,new):
    assert text.count(old)==1,old
    return text.replace(old,new)


def overlay(pioneer,here):
    shutil.copyfile(here/'PioneerQ38FfnSharedInputReuse.h',pioneer/'PioneerQ38FfnSharedInputReuse.h')
    projection=pioneer/'blackhole_qwen38_mixed_projection_root.cpp'
    text=replace(projection.read_text(),'#include "PioneerQ38ProjectionInput.h"',r'''#include "PioneerQ38ProjectionInput.h"
#ifdef PIONEER_Q38_VECTOR92_FFN_ROOT
#if (PIONEER_Q38_VECTOR92_FFN_ROOT != 1 && PIONEER_Q38_VECTOR92_FFN_ROOT != 2) \
 || !defined(PIONEER_Q38_LAYER_COMPONENT_RECURRENT) || !defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE) \
 || !defined(PIONEER_Q38_BATCHED_PREFETCH_ROOT_IMAGE) || !defined(PIONEER_Q38_BATCHED_OUTPUT_CLOSURE_ROOT_IMAGE) \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || defined(PIONEER_Q38_PREPARED_Q4_ROOT_IMAGE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(_KERNEL_MODE)
#error Exact inspection-only vector92 recurrent ROOT role required
#endif
#define PIONEER_Q38_FFN_SHARED_INPUT_REUSE_CANDIDATE 1
#include "PioneerQ38FfnSharedInputReuse.h"
#endif''')
    text=replace(text,' for(unsigned i=0;i<10;i++)Words(L::Scratch)[16+i]=job[i];',r''' for(unsigned i=0;i<10;i++)Words(L::Scratch)[16+i]=job[i];
#if defined(PIONEER_Q38_VECTOR92_FFN_ROOT) && PIONEER_Q38_VECTOR92_FFN_ROOT == 1
 if(d[43]==PioneerQ38FfnSharedInputReuse::UpRole)Words(L::Scratch)[25]=0;
#endif''')
    text=replace(text,'#endif\n#ifdef PIONEER_Q38_PREPARED_Q4_ROOT_IMAGE\n if(type==12){',r'''#endif
#if defined(PIONEER_Q38_VECTOR92_FFN_ROOT) && PIONEER_Q38_VECTOR92_FFN_ROOT == 1
 if(d[43]==PioneerQ38FfnSharedInputReuse::UpRole){
  reads-=PioneerQ38FfnSharedInputReuse::InputReads(false,k);
  flits-=PioneerQ38FfnSharedInputReuse::InputFlits(false,k,blocks);
 }
#endif
#ifdef PIONEER_Q38_PREPARED_Q4_ROOT_IMAGE
 if(type==12){''')
    projection.write_text(text)
    recurrent=pioneer/'blackhole_qwen38_mixed_recurrent_block_root.cpp'
    text=replace(recurrent.read_text(),' Phase(p,phase);\n unsigned n=d[11];',r''' Phase(p,phase);
#ifdef PIONEER_Q38_VECTOR92_FFN_ROOT
 // Slot 1 also exists in the four earlier recurrent projections. Only
 // the authenticated FFN-up phase may consume retained gate input.
 if(phase==11 || d[43]==PioneerQ38FfnSharedInputReuse::UpRole){
  if(phase!=11 || count!=3 || slot!=1 || !PioneerQ38FfnSharedInputReuse::PairFromRecipe(
   Words(R::RecipeCopies),d,R::Input))scalar_fault_park();
 }
#endif
 unsigned n=d[11];''')
    recurrent.write_text(text)


def main():
    assert __debug__ and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
    if len(sys.argv)!=3:
        raise SystemExit('use SDK NEW_OUTPUT')
    sdk,out=map(Path,sys.argv[1:])
    assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
    here=Path(__file__).resolve().parent
    tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
    out.mkdir(mode=0o700)
    frozen=out/'frozen';frozen.mkdir(mode=0o700)
    with subprocess.Popen(['git','-C',here.parents[1],'archive',SEED,'tools/pioneer',
                           'src/add-ons/kernel/drivers/misc'],stdout=subprocess.PIPE) as archive:
        with archive.stdout:
            subprocess.run(['tar','-x','-C',frozen],stdin=archive.stdout,check=True)
        assert archive.wait()==0
    pioneer=frozen/'tools/pioneer'
    build=pioneer/'build_qwen38_full_offload_batched_root.py'
    original=out/'vector92'
    subprocess.run([sys.executable,build,sdk,original],cwd=out,check=True)
    old=authenticated(original,pins(build,'PINS'))
    assert sha256(old['program.elf']).hexdigest()=='e93c1a57c21de073233689741e00ddc9e55ad20b1fd74d0758aa3100ce313f66'
    closure=original/'unchanged-output-closure'
    cc=(closure/'compile-recurrent-argv.txt').read_text().splitlines()
    ld=(original/'build-argv.txt').read_text().splitlines()[1].split('\t')
    shared=[p for p in closure.rglob('root_conv_sections.o') if p.parent.name=='unchanged-feedback']
    assert cc[0]==tool+'g++' and ld[0]==tool+'ld' and len(shared)==1
    assert cc[cc.index('-c')+1]==str(pioneer/'blackhole_qwen38_return_recurrent_layer.cpp')
    assert ld.count(str(closure/'components-NOBOOT.o'))==1
    overlay(pioneer,here)

    def build_variant(name,value):
        work=out/name;work.mkdir(mode=0o700)
        commands=[]
        def run(*values):
            call=list(map(str,values));commands.append(call)
            subprocess.run(call,cwd=work,check=True)
        obj=work/'recurrent.o'
        selector=['-D'+ROLE+'='+str(value)] if value else []
        command=[cc[0],*selector,*cc[1:cc.index('-o')],'-o',str(obj)]
        run(*command)
        if not value:
            assert obj.read_bytes()==(closure/'output-closure-recurrent.o').read_bytes()
        raw,components=work/'components-raw.o',work/'components-NOBOOT.o'
        run(tool+'ld','-m','elf32lriscv','--no-relax','-r','--gc-sections',
            '-u','q38_return_recurrent_layer','-u','q38_return_dense_layer',
            '-u','q38_global_chain_learned_rms','-u','q38_global_chain_quantize14',
            obj,closure/'output-closure-dense.o',shared[0],'-o',raw)
        dead=('mixed_recurrent_block_wait','mixed_recurrent_block_stopped',
              'mixed_live_block_wait','mixed_live_block_stopped')
        relocations=subprocess.check_output([tool+'objdump','-r',raw],text=True)
        assert all(not re.search(r'\b'+name+r'\b',relocations) for name in dead)
        run(tool+'objcopy',*['--strip-symbol='+name for name in dead],raw,components)
        assert subprocess.check_output([tool+'nm','-u',components],text=True).split()==['U','scalar_fault_park']
        if not value:
            assert components.read_bytes()==(closure/'components-NOBOOT.o').read_bytes()
        elf=work/'program.elf'
        link=[str(components) if a==str(closure/'components-NOBOOT.o') else a for a in ld]
        link[link.index('-o')+1]=str(elf);link[link.index('-Map')+1]=str(work/'program.map')
        run(*link)
        assert not subprocess.check_output([tool+'nm','-u',elf]).strip()
        symbols=subprocess.check_output([tool+'nm','-nC',elf],text=True)
        assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
        (work/'program.symbols').write_text(symbols)
        frames=[line.split('\t') for line in obj.with_suffix('.su').read_text().splitlines()]
        assert frames and all(len(r)==3 and r[2]=='static' and int(r[1])<=4096 for r in frames)
        manifest={}
        for section,bound in (('boot',4),('text',131072),('terminal',8),('conv',32768)):
            target=work/(section+'.bin');run(tool+'objcopy','-O','binary','-j','.'+section,elf,target)
            data=target.read_bytes();assert 0<len(data)<=bound and len(data)%4==0
            if not value or section in ('boot','terminal'):
                assert data==old[target.name]
            manifest[section]=[len(data),sha256(data).hexdigest()]
        if not value:
            assert elf.read_bytes()==old['program.elf']
        manifest['program.elf']=[elf.stat().st_size,sha256(elf.read_bytes()).hexdigest()]
        if value:
            assert tuple(tuple(manifest[key]) for key in ('text','conv','program.elf'))==MATCHED[name]
            run(sys.executable,pioneer/'tests/pioneer_q38_full_offload_eog_isa_test.py',sdk,work)
        (work/'manifest.json').write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n')
        (work/'build-argv.json').write_text(json.dumps(commands,indent=2)+'\n')
        print(name,manifest,flush=True)
        return manifest

    build_variant('unchanged',0)
    a,b=build_variant('reuse',1),build_variant('refetch',2)
    assert a['text']!=b['text']
    pos=cc.index('-c')
    for value,extra in ((0,()),(3,()),(1,('-DPIONEER_Q38_WORK_INFERENCE=1',)),
                        (2,('-D_KERNEL_MODE=1',)),(1,('-DPIONEER_Q38_CPU_VALIDATION=1',)),
                        (2,('-UPIONEER_Q38_LAYER_COMPONENT_RECURRENT',)),
                        (1,('-DPIONEER_Q38_PREPARED_Q4_ROOT_IMAGE=1',))):
        test=subprocess.run([*cc[:pos],'-D'+ROLE+'='+str(value),*extra,'-E',cc[pos+1]],cwd=out,
                            stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
        assert test.returncode and ('vector92 recurrent ROOT role required' in test.stderr or
          'FFN shared-input reuse is the independent Q4' in test.stderr),(value,extra,test.stderr)
    print('PASS BUILD ONLY measured vector92 reproduced; only recurrent component replaced; same dense/token and ABI',flush=True)


if __name__=='__main__':
    main()
