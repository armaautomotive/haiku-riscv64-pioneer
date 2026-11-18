#!/usr/bin/env python3
"""Port measured head CSR batching onto either exact v4 profile ROOT. Build only."""
from hashlib import sha256
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

from build_qwen38_twin_native_q8_wire import WIRE_ROOT_PINS
from generate_qwen38_token_programs import authenticated, pins

HERE=Path(__file__).resolve().parent
MEASURED='5e1340efb6'
TOKEN='blackhole_qwen38_token_root.cpp'
HEAD='blackhole_qwen38_global_head_control.inc'
DATA='PioneerQ38HeadCsrBatch.h'
MEASURED_SOURCE={TOKEN:'27695c4945638ff8d871d18ffbe71cbf8264eaf5c02168865b241dc62983c65c',
                 HEAD:'09def8ada1d3ffc4a832fe5b24824447ee5ec8b8654553487ce2305fe1c505d7',
                 DATA:'aa5a79759dd2b3fb59af6a8256005376e9cdf53425909825a7e0d483dc465d4e'}
SECTIONS={'boot':4,'text':0x20000,'terminal':8,'conv':0x8000,'fast_decode':0x2000}
RAW_BATCH_PINS=(
 ('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',130696,'04cf1effcb724c45fa7f0d806e9129e880da478c4d3e4854646e7cdd76aee9e3'),
 ('terminal.bin',8,'468eb84a74c067cc328b3fb3e51b768a8440cd67d292add55eef6861c9b24de2'),
 ('conv.bin',31852,'72f0319330c59a73e1c101a0cb0d618aa07e406eb413747b967d6440306b908b'),
 ('fast_decode.bin',2652,'abb438047fad58d1998a385acdc6e4aee70bad1c549e630967bb314bad63e003'),
 ('program.elf',195468,'ce5d724c70084dc3f3e4ce9675f6c362c624d59d5df6d3437f47cc69038cf739'),
)
WIRE_BATCH_PINS=(
 ('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',131032,'1d70ee636d8459f48b08622706078dbeb5622fc9619f89fb0b551c320f3f0f8f'),
 ('terminal.bin',8,'468eb84a74c067cc328b3fb3e51b768a8440cd67d292add55eef6861c9b24de2'),
 ('conv.bin',31852,'e229b868947eda74609aadfc30ec1247f9000d0cdc3a1bc165f1bf977a1d9958'),
 ('fast_decode.bin',2652,'2c20efb87d6c3b2f584601a8583b759401e173d3fa9d66b92291684c40d11290'),
 ('program.elf',195468,'53645a41eeb035afd9805b7aa1c287048d44994eed8be53546e85e5fb624fadd'),
)
SERIAL_WHOLE_SHA={False:'d1fcd50962bc25b7bb8b086b2775a47ba2da0a5c340ace2862d8675d78448fe5',
                 True:'ae3e1d990f81d8c0faf791d5958e90bac9357af430328f2611e17b6994ba4865'}


def measured(name):
    data=subprocess.check_output(['git','-C',HERE.parents[1],'show',MEASURED+':tools/pioneer/'+name])
    assert sha256(data).hexdigest()==MEASURED_SOURCE[name],name
    return data


def overlay(source):
    token=(source/TOKEN).read_text()
    subject=measured(TOKEN).decode('ascii')
    # Copy only the profiled owner's Scope extension and head-role include.
    # These two blocks reproduce the already-measured matched A/B roles.
    scope='static __attribute__((noinline,section(".conv.token")'
    for before,after in (('#include "PioneerQ38FastDecodeProfileBuildRole.h"\n','#define PIONEER_ATTENTION_FREESTANDING'),
                         (scope,' if(p->phase==5){')):
        if before!=scope:assert token.count(before)==1
        else:assert 'void Scope' in token[token.index(scope):token.index(scope)+128]
        a,b=token.index(before),token.index(after,token.index(before))
        c,d=subject.index(before),subject.index(after,subject.index(before))
        if before==scope:assert 'void Scope(' in subject[c:d]
        token=token[:a]+subject[c:d]+token[b:]
    (source/TOKEN).write_text(token)
    for name in (HEAD,DATA):(source/name).write_bytes(measured(name))


def main():
    wire=len(sys.argv)==5 and sys.argv[4]=='--native-q8-wire'
    if len(sys.argv)!=4 and not wire:
        raise SystemExit('use SDK V4_RAW_ROOT_OR_WIRE_BUILD NEW_OUTPUT [--native-q8-wire]')
    assert __debug__ and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
    sdk,base,out=map(Path,sys.argv[1:4]);root=base/'root' if wire else base
    assert all(p.is_absolute() for p in (sdk,base,out)) and not out.exists() and not out.is_symlink()
    table=WIRE_ROOT_PINS if wire else pins(HERE/'build_qwen38_fast_decode_profile_root.py','SPLIT_PINS')
    expected=authenticated(root,table)
    tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
    args=[line.split('\t') for line in (base/'build-argv.txt').read_text().splitlines()]

    def selected(executable,target):
        entries=[a for a in args if a[0]==tool+executable and '-o' in a and a[a.index('-o')+1]==str(root/target)]
        assert len(entries)==1 and entries[0].count('-o')==1,target
        return entries[0]

    compile_token=selected('g++','fast-token.o');relink=selected('ld','program.elf')
    exact_wire='-DPIONEER_Q38_TWIN_NATIVE_Q8_WIRE=1'
    assert (exact_wire in compile_token)==wire and '-DPIONEER_Q38_LAYER_SPLIT_PROFILE_IMAGE=1' in compile_token
    assert relink.count(str(root/'fast-token.o'))==1
    original=Path(compile_token[compile_token.index('-c')+1]).parent
    out.mkdir(mode=0o700)
    source=out/'source/tools/pioneer';shutil.copytree(original,source)
    driver='src/add-ons/kernel/drivers/misc'
    shutil.copytree(original.parents[1]/driver,out/'source'/driver)
    overlay(source)
    commands=[]
    layout=Path(relink[relink.index('-T')+1]).read_text()
    # The original timer stays at the same address and keeps its ABI.
    # Place only the single already-measured Verify() body after it.
    # A native package must journal this exact longer code extent before
    # this build can be selected. Do not enlarge a region or move STOP.
    anchor='*(.fast.decode.profile .text.*PioneerQ38FastDecodeProfileData*)'
    assert layout.count(anchor)==1
    combined=out/'batch.ld'
    combined.write_text(layout.replace(anchor,anchor+' *(.text.*PioneerQ38HeadCsrBatch6Verify*)',1))

    def run(command):
        args=list(map(str,command));commands.append(args);subprocess.run(args,check=True,cwd=out)

    dirs=[]
    for label,flag in (('control',''),('batch','PIONEER_Q38_HEAD_CSR_BATCH_ROOT_IMAGE'),
                       ('serial','PIONEER_Q38_HEAD_CSR_SERIAL_CONTROL_IMAGE')):
        d=out/label;d.mkdir(mode=0o700);dirs.append(d)
        obj,elf=d/'fast-token.o',d/'program.elf'
        args=compile_token[:]
        for i,a in enumerate(args[:-1]):
            if a in ('-c','-include'):args[i+1]=str(source/Path(args[i+1]).name)
        if '-MF' in args:args[args.index('-MF')+1]=str(d/'token.d')
        args[args.index('-o')+1]=str(obj)
        if flag:args.insert(1,'-D'+flag+'=1')
        run(args)
        rows=[r.split('\t') for r in obj.with_suffix('.su').read_text().splitlines()]
        assert rows and all(len(r)==3 and r[2]=='static' and int(r[1])<=2048 for r in rows)
        if label=='batch':
            probe=args[:args.index('-c')]
            if '-MF' in probe:del probe[probe.index('-MF'):probe.index('-MF')+2]
            if '-MMD' in probe:probe.remove('-MMD')
            for wrong in ('PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_WORK_INFERENCE',
                          'PIONEER_Q38_HEAD_CSR_SERIAL_CONTROL_IMAGE','PIONEER_Q38_FULL_OFFLOAD_NATIVE'):
                refused=subprocess.run([*probe,'-D'+wrong+'=1','-E',source/TOKEN],stdout=subprocess.DEVNULL,
                                       stderr=subprocess.PIPE,text=True,cwd=out)
                assert refused.returncode and 'Queued head CSR check' in refused.stderr,wrong
        args=[str(obj) if p==str(root/'fast-token.o') else p for p in relink]
        if label=='batch':args[args.index('-T')+1]=str(combined)
        for key,value in (('-o',elf),('-Map',d/'program.map')):
            assert args.count(key)==1;args[args.index(key)+1]=str(value)
        run(args)
        if not flag:
            assert obj.read_bytes()==(root/obj.name).read_bytes() and elf.read_bytes()==expected['program.elf']
        sym=subprocess.check_output([tool+'nm','-nC',elf],text=True)
        assert ('GlobalChain::HeadRegisterBatch(' in sym)==(label=='batch')
        assert bool(re.search(r'^0006c5d8 W bool PioneerQ38HeadCsrBatch::Verify<',sym,re.M))==(label=='batch')
        assert not subprocess.check_output([tool+'nm','-u',elf]).strip()
        assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',sym,re.M)
        (d/'program.symbols').write_text(sym)
        for name,maximum in SECTIONS.items():
            path=d/(name+'.bin');run([tool+'objcopy','-O','binary','-j','.'+name,elf,path])
            data=path.read_bytes();assert data and not len(data)%4 and len(data)<=maximum
            if name in ('boot','terminal') or (name=='fast_decode' and label!='batch'):
                assert data==expected[path.name]
            if name=='fast_decode' and label=='batch':
                assert data.startswith(expected[path.name]) and len(data)>len(expected[path.name])
            print(label,name,len(data),sha256(data).hexdigest(),flush=True)
        print(label,elf.name,elf.stat().st_size,sha256(elf.read_bytes()).hexdigest(),flush=True)
        if label=='batch':authenticated(d,WIRE_BATCH_PINS if wire else RAW_BATCH_PINS)
        if label=='serial':assert sha256(elf.read_bytes()).hexdigest()==SERIAL_WHOLE_SHA[wire]

    fixture='pioneer_q38_head_csr_linked_test.cpp'
    shutil.copyfile(HERE/'tests'/fixture,source/'tests'/fixture)
    unit=out/'head-test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-Wno-unused-function','-Wno-misleading-indentation','-fsanitize=address,undefined',
                    *([exact_wire] if wire else []),source/'tests'/fixture,'-o',unit],check=True)
    subprocess.run([unit,*dirs],check=True,timeout=75,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
    (out/'build-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands)+'\n')
    print('PASS BUILD_ONLY matched v4 head CSR pair; no driver selection, device or TPS',flush=True)


if __name__=='__main__':
    main()
