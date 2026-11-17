#!/usr/bin/env python3
"""Rebuild measured vector92; substitute an inspection-only FFN sender."""
from hashlib import sha256
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys


SEED = 'df48ea751a5f9863a1cfe064fdfc26f60cfb49b6'
ELF = '4b3dc0c96d3888b9c43af683f58678cc91cade7c67e955413bad8c5e16d52870'
REUSE_ELF = 'aefeb0c7cad0eeb1f13867b2c77e473dca4eeb482ad7c9a97cd00d29b19a3f43'
PROGRAMS = '3ec53793460dbb937368f952da6a5d19f83e82e27d1a027e416952a309bec389'
SECTIONS = dict(boot='PeerBoot', globalworker='PeerFront', precise='PeerPrecise', dense='PeerDense',
                ffn='PeerFfn', service='PeerService', fetch='PeerFetch', affine='PeerAffine',
                text='PeerText', gdn='PeerGdn')
MARKER = 'PIONEER_Q38_VECTOR92_FFN_SENDER'


def modified_sender(base):
    def change(old, new):
        nonlocal base
        assert base.count(old) == 1, old
        base = base.replace(old, new)
    change('#include "PioneerQ38GlobalWorkerLayout.h"', '''#include "PioneerQ38GlobalWorkerLayout.h"
#ifdef PIONEER_Q38_VECTOR92_FFN_SENDER
#if PIONEER_Q38_VECTOR92_FFN_SENDER != 1 || !defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_WORKER_IMAGE) \
 || !defined(PIONEER_Q38_PRECISE_WORKER_IMAGE) || !defined(PIONEER_Q38_PUSH_COMPLETION_WORKER_IMAGE) \
 || defined(PIONEER_Q38_PREPARED_EXACT_Q4_WORKER_IMAGE) || defined(PIONEER_Q38_FFN_SHARED_INPUT_REUSE_CANDIDATE)
#error Exact inspection-only vector92 sender role required
#endif
#define PIONEER_Q38_FFN_SHARED_INPUT_REUSE_CANDIDATE 1
#include "PioneerQ38FfnSharedInputReuse.h"
#endif''')
    change('|| j[4] || j[9]!=1 || !PioneerQ38PanelTileBytes(j[1])', '''|| j[4]
#ifdef PIONEER_Q38_VECTOR92_FFN_SENDER
  || j[9]>1 || (!j[9] && (j[1]!=12 || j[5]!=5120))
#else
  || j[9]!=1
#endif
  || !PioneerQ38PanelTileBytes(j[1])''')
    change('  +(j[5]*2+16383)/16384+1;\n unsigned flits=j[7]*blocks*j[2]/64+(j[5]*2+((blocks*4+63)&~63U))/64;', '''#ifdef PIONEER_Q38_VECTOR92_FFN_SENDER
  +PioneerQ38FfnSharedInputReuse::InputReads(!j[9],j[5]);
 unsigned flits=j[7]*blocks*j[2]/64+PioneerQ38FfnSharedInputReuse::InputFlits(!j[9],j[5],blocks);
#else
  +(j[5]*2+16383)/16384+1;
 unsigned flits=j[7]*blocks*j[2]/64+(j[5]*2+((blocks*4+63)&~63U))/64;
#endif''')
    change('  unsigned request=block_peer_wait(previous);', '''  unsigned request=block_peer_wait(previous);
#ifdef PIONEER_Q38_VECTOR92_FFN_SENDER
  // The last local descriptor is private. 'previous' advances only after
  // its real Matrix, push receipt and completion; never across a reset.
  const bool fetched=previous && seal.job[2]==1 && seal.job[17]==12 && seal.job[21]==5120 && seal.job[25]==1;
  const unsigned oldLayer=fetched?seal.job[5]:64,oldPosition=fetched?seal.job[6]:0,oldRecipe=fetched?seal.job[7]:0;
#endif''')
    change('  unsigned op=seal.job[2];const bool outputJob=op==M::GlobalMatrix;', '''  unsigned op=seal.job[2];const bool outputJob=op==M::GlobalMatrix;
#ifdef PIONEER_Q38_VECTOR92_FFN_SENDER
  // ROOT still authenticates the gate/up roles and common input. The
  // worker additionally requires a fresh preceding compatible fill.
  if(op==1 && !seal.job[25] && (!fetched || seal.job[17]!=12 || seal.job[21]!=5120
   || oldLayer!=seal.job[5] || oldPosition!=seal.job[6] || !oldRecipe || oldRecipe>=0x7ffffffe
   || seal.job[7]!=oldRecipe+1))block_fault_park();
#endif''')
    return base


def main():
    assert __debug__ and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
    if len(sys.argv) != 3:
        raise SystemExit('use SDK NEW_OUTPUT')
    sdk, out = map(Path, sys.argv[1:])
    assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
    here = Path(__file__).resolve().parent
    repo = here.parents[1]
    tool = str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
    out.mkdir(mode=0o700)
    tree = out/'frozen';tree.mkdir(mode=0o700)
    with subprocess.Popen(['git','-C',repo,'archive',SEED,'tools/pioneer',
                           'src/add-ons/kernel/drivers/misc'],stdout=subprocess.PIPE) as archive:
        with archive.stdout:
            subprocess.run(['tar','-x','-C',tree],stdin=archive.stdout,check=True)
        assert archive.wait()==0
    source = tree/'tools/pioneer'
    base = out/'vector92'
    subprocess.run([sys.executable,source/'build_qwen38_vector_q4_metadata_worker.py',sdk,base],cwd=out,check=True)
    assert sha256((base/'program.elf').read_bytes()).hexdigest()==ELF
    native_source = subprocess.check_output(['git','-C',repo,'show',
                                             '113fdbd404b1a5e55d160c9b8ca5ecf15c57208e:tools/pioneer/PioneerQ38FullOffloadVectorQ4Programs.h'])
    assert sha256(native_source).hexdigest()==PROGRAMS
    arrays = {name:digest for digest,name in re.findall(rb'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]',native_source)}
    assert len(arrays)==14
    for section,array in SECTIONS.items():
        assert sha256((base/(section+'.bin')).read_bytes()).hexdigest().encode()==arrays[array.encode()]

    global_sender=source/'blackhole_qwen38_global_worker.cpp'
    global_sender.write_text(modified_sender(global_sender.read_text()))
    shutil.copyfile(here/'PioneerQ38FfnSharedInputReuse.h',source/'PioneerQ38FfnSharedInputReuse.h')
    cc=(base/'unchanged-wide/unchanged-twin/unchanged-batched/compile-global-argv.txt').read_text().splitlines()
    ld=(base/'link-argv.txt').read_text().splitlines()
    assert cc[0]==tool+'g++' and ld[0]==tool+'ld'
    sender=cc.index('-c');old=cc[cc.index('-o')+1]
    assert Path(cc[sender+1])==source/'blackhole_qwen38_batched_prefetch_global.cpp' and ld.count(old)==1
    work=out/'reuse';work.mkdir(mode=0o700)
    new=work/'reuse-global.o';elf=work/'program.elf'
    control=[*cc[:cc.index('-o')],'-o',str(work/'control-global.o')]
    subprocess.run(control,cwd=work,check=True)
    assert (work/'control-global.o').read_bytes()==Path(old).read_bytes(), 'flag-off entire original sender changed'
    compile_command=[cc[0],'-D'+MARKER+'=1',*cc[1:cc.index('-o')],'-o',str(new)]
    subprocess.run(compile_command,cwd=work,check=True)
    refusals=(('-U'+MARKER,'-D'+MARKER+'=0'),('-D_KERNEL_MODE=1',),('-DPIONEER_Q38_WORK_INFERENCE=1',),
              ('-DPIONEER_Q38_CPU_VALIDATION=1',),('-DPIONEER_Q38_PREPARED_EXACT_Q4_WORKER_IMAGE=1',))
    for args in refusals:
        result=subprocess.run([*cc[:sender],'-D'+MARKER+'=1',*args,'-E',cc[sender+1]],cwd=work,
                              stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
        assert result.returncode and ('vector92 sender role required' in result.stderr or
                                      'cannot alias old/native/ROOT/CPU oracle/Work ownership' in result.stderr),args
    ld[ld.index(old)]=str(new)
    ld[ld.index('-o')+1]=str(elf);ld[ld.index('-Map')+1]=str(work/'program.map')
    subprocess.run(ld,cwd=work,check=True)
    assert not subprocess.check_output([tool+'nm','-u',elf]).strip()
    symbols=subprocess.check_output([tool+'nm','-n',elf],text=True)
    assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',symbols,re.M)
    (work/'program.symbols').write_text(symbols)
    dis=work/'program.disassembly'
    dis.write_bytes(subprocess.check_output([tool+'objdump','-d',elf]))
    subprocess.run([sys.executable,source/'tests/pioneer_q38_gdn_rendezvous_csr_gate.py',dis],cwd=work,check=True)
    frames=[r.split('\t') for r in new.with_suffix('.su').read_text().splitlines()]
    assert frames and all(len(r)==3 and r[2]=='static' and int(r[1])<=2048 for r in frames)
    changed=[]
    manifest={}
    for section in SECTIONS:
        target=work/(section+'.bin')
        subprocess.run([tool+'objcopy','-O','binary','-j','.'+section,elf,target],check=True)
        actual=target.read_bytes()
        assert actual and len(actual)%4==0
        manifest[section]=[len(actual),sha256(actual).hexdigest()]
        if actual!=(base/target.name).read_bytes():changed.append(section)
    assert changed==['globalworker'], changed
    manifest['program.elf']=[elf.stat().st_size,sha256(elf.read_bytes()).hexdigest()]
    assert manifest['program.elf']==[144592,REUSE_ELF]
    (work/'manifest.json').write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n')
    (work/'build-argv.json').write_text(json.dumps([compile_command,ld],indent=2)+'\n')
    print('BUILD ONLY: exact measured vector92 rebuilt; only sender changed; no card or timing.',flush=True)
    print(manifest['program.elf'],flush=True)


if __name__ == '__main__':
    main()
