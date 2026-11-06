#!/usr/bin/env python3
"""Qualify timed Q4 BUILD offboard. No hardware endpoint/install authority."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

HERE=Path(__file__).resolve().parent
TOOLS=HERE.parent
ROOT=TOOLS.parent.parent
sys.path.insert(0,str(TOOLS))
from build_q38_q4_kernel_timing import audit_clock

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sdk',type=Path)
    parser.add_argument('original',type=Path)
    parser.add_argument('out',type=Path)
    args=parser.parse_args()
    assert __debug__ and not os.environ.get('PYTHONOPTIMIZE')
    assert args.sdk.is_absolute() and args.sdk.is_dir()
    assert args.original.is_absolute() and args.original.is_file() and not args.original.is_symlink()
    assert args.out.is_absolute() and not args.out.exists() and not args.out.is_symlink()
    args.out.mkdir(mode=0o700)
    dependencies={Path(__file__).resolve()};exits={};commands=[];frozen={}
    def collect(compiler,source,flags=()):
        body=subprocess.check_output([str(compiler),'-std=c++17',*flags,'-MM',str(source)],text=True)
        dependencies.update(Path(word).resolve() for word in body.replace('\\\n','').split(':',1)[1].split())
    def freeze():
        for path in dependencies:
            digest=hashlib.sha256(path.read_bytes()).hexdigest()
            if path in frozen:assert frozen[path]==digest,'source changed during qualification'
            frozen[path]=digest
    def run(name,command):
        freeze();commands.append([name,*map(str,command)])
        with (args.out/(name+'.stdout')).open('xb') as out,(args.out/(name+'.stderr')).open('xb') as err:
            child=subprocess.run(list(map(str,command)),stdout=out,stderr=err,check=False)
        exits[name]=child.returncode
        (args.out/'actual-exits.json').write_text(json.dumps(exits,sort_keys=True,indent=2)+'\n')
        assert not child.returncode and not (args.out/(name+'.stderr')).stat().st_size,name
        freeze();print('CLOSED_Q4_TIMING_OFFBOARD',name,child.returncode,flush=True)
    tool=str(args.sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
    flags=['-march=rv32i','-mabi=ilp32','-DPIONEER_Q38_COMPACT_Q4_TIMING_IMAGE=1',
           '-DPIONEER_Q38_COMPACT_Q4_FPU_STREAM_IMAGE=1','-DPIONEER_Q4_FPU_COMPACT_STREAM_K=5120']
    for source in ('blackhole_q38_q4_fpu_compact_stream.cpp','blackhole_q38_q4_kernel_clock.cpp'):
        collect(tool+'g++',TOOLS/source,flags)
    for name in ('build_q38_q4_kernel_timing.py','build_q38_q4_fpu_compact_stream.py',
                 'build_q38_q4_fpu_stream.py','blackhole_q38_q4_fpu_compact_stream_start.S',
                 'blackhole_q38_q4_fpu_compact_stream.ld','blackhole_q38_q4_fpu_stream.cpp',
                 'blackhole_q38_q4_fpu_stream_start.S','blackhole_q38_q4_fpu_stream.ld'):
        dependencies.add(TOOLS/name)
    collect(tool+'g++',TOOLS/'blackhole_q38_q4_fpu_stream.cpp',
            ['-march=rv32i','-mabi=ilp32','-DPIONEER_Q4_FPU_STREAM_K=5120'])
    host=['c++','-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined']
    for name in ('data','image','fault'):
        collect(host[0],HERE/('pioneer_q38_q4_kernel_timing_'+name+'_test.cpp'))
    collect(host[0],HERE/'pioneer_q38_q4_fpu_compact_stream_model_test.cpp',['-DPIONEER_Q4_KERNEL_TIMING_MODEL_TEST=1'])
    collect(host[0],TOOLS/'PioneerModelWeightCache.cpp')
    for width in (5120,17408):
        run('build'+str(width),[sys.executable,TOOLS/'build_q38_q4_kernel_timing.py',args.sdk,args.out/('image'+str(width)),width])
    for name in ('data','image','fault'):
        binary=args.out/(name+'-test')
        run(name+'-compile',[*host,HERE/('pioneer_q38_q4_kernel_timing_'+name+'_test.cpp'),'-o',binary])
        if name=='image':
            for width in (5120,17408):run('image'+str(width),[binary,args.out/('image'+str(width)),width])
        else:
            run(name,[binary,*([args.out/'image5120'] if name=='fault' else [])])
    binary=args.out/'model-test'
    run('model-compile',[*host,'-DPIONEER_Q4_KERNEL_TIMING_MODEL_TEST=1',
        HERE/'pioneer_q38_q4_fpu_compact_stream_model_test.cpp',TOOLS/'PioneerModelWeightCache.cpp','-o',binary])
    run('model',[binary,args.out/'image5120',args.out/'image17408',args.original])
    # The old strict machine remains CSR-free;re-run both original compact
    # binaries reproduced by the new builder,not only their digest checks.
    binary=args.out/'old-image-test'
    run('old-image-compile',[*host,HERE/'pioneer_q38_q4_fpu_compact_stream_image_test.cpp','-o',binary])
    for width in (5120,17408):
        run('old-image'+str(width),[binary,args.out/('image'+str(width))/'unchanged-compact',width])
    refused=0
    for width in (5120,17408):
        image=args.out/('image'+str(width));code=(image/'text.bin').read_bytes()
        symbols=(image/'program.symbols').read_text()
        match=re.search(r'^([0-9a-f]+) ([0-9a-f]+) T q38_q4_kernel_clock$',symbols,re.M);assert match
        start,size=(int(value,16) for value in match.groups());sites=audit_clock(code,start,size)
        at=sites[0][0]-0x70000;word=int.from_bytes(code[at:at+4],'little')
        for changed in ((word&~0x7000)|0x1000,word|(1<<15),word&~0xf80,(word&0xfffff)|(0x300<<20)):
            bad=bytearray(code);bad[at:at+4]=changed.to_bytes(4,'little')
            try:audit_clock(bad,start,size)
            except AssertionError:refused+=1
            else:raise AssertionError('bad CSR accepted')
        try:audit_clock(code,start+size,size)
        except AssertionError:refused+=1
        else:raise AssertionError('wrong sampler bounds accepted')
    forbidden=['_KERNEL_MODE','PIONEER_DRIVER_BUILD_TAG','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
               'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
               'PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT','__FAST_MATH__']
    for macro in forbidden:
        for value in (0,1):
            child=subprocess.run([tool+'g++',*flags,'-D'+macro+'='+str(value),'-E',
                str(TOOLS/'blackhole_q38_q4_kernel_clock.cpp')],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
            assert child.returncode and b'Exact separate Q4 kernel timing BUILD only' in child.stderr
            refused+=1
    for macro in ('PIONEER_Q38_COMPACT_Q4_TIMING_IMAGE','PIONEER_Q38_COMPACT_Q4_FPU_STREAM_IMAGE'):
        for value in (None,0,2):
            selected=[flag for flag in flags if not flag.startswith('-D'+macro+'=')]
            if value is not None:selected.append('-D'+macro+'='+str(value))
            child=subprocess.run([tool+'g++',*selected,'-E',str(TOOLS/'blackhole_q38_q4_kernel_clock.cpp')],
                stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
            assert child.returncode and b'Exact separate Q4 kernel timing BUILD only' in child.stderr;refused+=1
    (args.out/'refusals.txt').write_text(f'PASS timingCSRAndRoleRefusals{refused};NO_NATIVE_AUTHORITY\n')
    freeze()
    (args.out/'source-sha256.txt').write_text(''.join(f'{frozen[path]}  {path.relative_to(ROOT)}\n' for path in sorted(frozen)))
    (args.out/'test-argv.json').write_text(json.dumps(commands,indent=2)+'\n')
    print(f'PASS_Q4_TIMING_OFFBOARD children{len(exits)} strictStderr0 source{len(frozen)} refusals{refused} '
          'sameOriginalFullKWords576 clocksSynthetic8 oldImagesExact;NO_NATIVE_OWNER_HARDWARE_TIMING_OR_TPS',flush=True)

if __name__=='__main__':main()
