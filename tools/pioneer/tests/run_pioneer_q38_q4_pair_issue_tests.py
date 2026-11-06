#!/usr/bin/env python3
"""Qualify one Q4 issue optimization offboard; no hardware/install actions."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

HERE=Path(__file__).resolve().parent
TOOLS=HERE.parent
ROOT=TOOLS.parent.parent


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
    dependencies={Path(__file__).resolve()};frozen={};exits={};commands=[]
    def collect(compiler,source,flags=()):
        output=subprocess.check_output([str(compiler),'-std=c++17',*flags,'-MM',str(source)],text=True)
        dependencies.update(Path(word).resolve() for word in output.replace('\\\n','').split(':',1)[1].split())
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
        freeze();print('CLOSED_Q4_PAIR_ISSUE_OFFBOARD',name,child.returncode,flush=True)
    tool=str(args.sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
    flags=['-march=rv32i','-mabi=ilp32','-DPIONEER_Q38_COMPACT_Q4_TIMING_IMAGE=1',
           '-DPIONEER_Q38_COMPACT_Q4_FPU_STREAM_IMAGE=1','-DPIONEER_Q4_FPU_COMPACT_STREAM_K=5120',
           '-DPIONEER_Q38_Q4_PAIR_ISSUE_IMAGE=1']
    for source in ('blackhole_q38_q4_fpu_compact_stream.cpp','blackhole_q38_q4_kernel_clock.cpp'):
        collect(tool+'g++',TOOLS/source,flags)
    for name in ('build_q38_q4_pair_issue.py','build_q38_q4_kernel_timing.py',
                 'build_q38_q4_fpu_compact_stream.py','build_q38_q4_fpu_stream.py',
                 'blackhole_q38_q4_fpu_compact_stream_start.S','blackhole_q38_q4_fpu_compact_stream.ld',
                 'blackhole_q38_q4_fpu_stream.cpp','blackhole_q38_q4_fpu_stream_start.S','blackhole_q38_q4_fpu_stream.ld'):
        dependencies.add(TOOLS/name)
    collect(tool+'g++',TOOLS/'blackhole_q38_q4_fpu_stream.cpp',
            ['-march=rv32i','-mabi=ilp32','-DPIONEER_Q4_FPU_STREAM_K=5120'])
    host=['c++','-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined']
    for name in ('pair_issue_words','pair_issue_image','kernel_timing_image','kernel_timing_fault'):
        collect(host[0],HERE/('pioneer_q38_q4_'+name+'_test.cpp'))
    model_flags=['-DPIONEER_Q4_KERNEL_TIMING_MODEL_TEST=1','-DPIONEER_Q4_PAIR_ISSUE_MODEL_TEST=1']
    collect(host[0],HERE/'pioneer_q38_q4_fpu_compact_stream_model_test.cpp',model_flags)
    collect(host[0],TOOLS/'PioneerModelWeightCache.cpp')
    for width in (5120,17408):
        run('build'+str(width),[sys.executable,TOOLS/'build_q38_q4_pair_issue.py',args.sdk,args.out/('image'+str(width)),width])
    for name in ('pair_issue_words','pair_issue_image','kernel_timing_image','kernel_timing_fault'):
        binary=args.out/(name+'-test')
        run(name+'-compile',[*host,HERE/('pioneer_q38_q4_'+name+'_test.cpp'),'-o',binary])
        if name=='pair_issue_words':run(name,[binary])
        elif name=='pair_issue_image':
            for width in (5120,17408):
                image=args.out/('image'+str(width))
                run(name+str(width),[binary,image/'unchanged-timing',image,width])
        elif name=='kernel_timing_image':
            for width in (5120,17408):run(name+str(width),[binary,args.out/('image'+str(width))/'unchanged-timing',width])
        else:run(name,[binary,args.out/'image5120'/'unchanged-timing'])
    binary=args.out/'model-test'
    run('model-compile',[*host,*model_flags,HERE/'pioneer_q38_q4_fpu_compact_stream_model_test.cpp',
                         TOOLS/'PioneerModelWeightCache.cpp','-o',binary])
    run('model',[binary,args.out/'image5120',args.out/'image17408',args.original])
    refused=0
    for macro in ('PIONEER_Q38_Q4_PAIR_ISSUE_IMAGE','PIONEER_Q38_COMPACT_Q4_TIMING_IMAGE'):
        for value in (0,2):
            selected=[arg for arg in flags if not arg.startswith('-D'+macro+'=')]
            selected.append('-D'+macro+'='+str(value))
            child=subprocess.run([tool+'g++',*selected,'-E',str(TOOLS/'blackhole_q38_q4_fpu_compact_stream.cpp')],
                                 stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
            assert child.returncode and b'error:' in child.stderr;refused+=1
    for macro in ('_KERNEL_MODE','PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT',
                  'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION',
                  'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_DRIVER_BUILD_TAG','TT_PIONEER_DRIVER_RELOAD','__FAST_MATH__'):
        for value in (0,1):
            child=subprocess.run([tool+'g++',*flags,'-D'+macro+'='+str(value),'-E',
                                  str(TOOLS/'blackhole_q38_q4_fpu_compact_stream.cpp')],
                                 stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
            assert child.returncode and b'error:' in child.stderr;refused+=1
    # A paired BUILD cannot silently become an untimed image.
    selected=[arg for arg in flags if not arg.startswith('-DPIONEER_Q38_COMPACT_Q4_TIMING_IMAGE=')]
    child=subprocess.run([tool+'g++',*selected,'-E',str(TOOLS/'blackhole_q38_q4_fpu_compact_stream.cpp')],
                         stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
    assert child.returncode and b'Paired Q4 issue requires' in child.stderr;refused+=1
    for value,timing in ((0,True),(2,True),(1,False)):
        child=subprocess.run([tool+'as','--defsym','PIONEER_Q38_Q4_PAIR_ISSUE_IMAGE='+str(value),
            *(['--defsym','PIONEER_Q38_COMPACT_Q4_TIMING_IMAGE=1'] if timing else []),
            '-march=rv32i','-mabi=ilp32',str(TOOLS/'blackhole_q38_q4_fpu_compact_stream_start.S'),
            '-o',str(args.out/'refused-start.o')],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
        assert child.returncode and b'Error:' in child.stderr;refused+=1
    (args.out/'refusals.txt').write_text(f'PASS pairedRoleRefusals{refused};NO_NATIVE_AUTHORITY\n')
    freeze()
    (args.out/'source-sha256.txt').write_text(''.join(f'{frozen[path]}  {path.relative_to(ROOT)}\n' for path in sorted(frozen)))
    (args.out/'test-argv.json').write_text(json.dumps(commands,indent=2)+'\n')
    print(f'PASS_Q4_PAIR_ISSUE_OFFBOARD children{len(exits)} strictStderr0 source{len(frozen)} roleRefusals{refused} '
          'sameOriginalFullKWords576 exactTensixStream fewerRV32Steps unchangedOldImages;NO_NATIVE_HARDWARE_TIMING_OR_TPS',flush=True)


if __name__=='__main__':main()
