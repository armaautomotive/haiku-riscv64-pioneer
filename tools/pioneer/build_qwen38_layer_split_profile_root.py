#!/usr/bin/env python3
"""Build a v4 layer-phase diagnostic against the measured vector92 sources."""
import os
from pathlib import Path
import shutil
import subprocess
import sys


SEED='5a95c1c42983751153df493c2540d9b6501520d8'
FLAG='PIONEER_Q38_LAYER_SPLIT_PROFILE_IMAGE'
FAST='PIONEER_Q38_FAST_DECODE_PROFILE_IMAGE'


def overlay(frozen, current, name, replacements):
    source=(frozen/name).read_text()
    tracked=(current/name).read_text()
    for old,new in replacements:
        assert source.count(old)==1 and tracked.count(new)==1,(name,old)
        source=source.replace(old,new)
    (frozen/name).write_text(source)


def main():
    assert __debug__ and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
    if len(sys.argv)!=3:
        raise SystemExit('use SDK NEW_OUTPUT')
    sdk,out=map(Path,sys.argv[1:])
    assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
    here=Path(__file__).resolve().parent
    out.mkdir(mode=0o700)
    source=out/'frozen';source.mkdir(mode=0o700)
    with subprocess.Popen(['git','-C',here.parents[1],'archive',SEED,'tools/pioneer',
                           'src/add-ons/kernel/drivers/misc'],stdout=subprocess.PIPE) as archive:
        with archive.stdout:
            subprocess.run(['tar','-x','-C',source],stdin=archive.stdout,check=True)
        assert archive.wait()==0
    frozen=source/'tools/pioneer'
    control=out/'vector92'
    subprocess.run([sys.executable,frozen/'build_qwen38_full_offload_batched_root.py',sdk,control],cwd=out,check=True)
    original=frozen/'build_qwen38_fast_decode_profile_root.py'
    subprocess.run([sys.executable,original,sdk,control,out/'v3-control'],cwd=out,check=True)
    for name in ('PioneerQ38FastDecodeProfileData.h','PioneerQ38FastDecodeProfileBuildRole.h',original.name):
        shutil.copyfile(here/name,frozen/name)

    old=f'#ifdef {FAST}\n PioneerQ38FastDecodeProfileScope timing(PioneerQ38FastDecodeProfileData::MatrixPrepare);'
    enter=f'#if defined({FLAG})\n PioneerQ38FastDecodeProfileScope timing(PioneerQ38FastDecodeProfileData::Matrix);\n'
    projections=[(old,enter+old.replace('#ifdef '+FAST,'#elif defined('+FAST+')',1))]
    for phase in ('MatrixDispatch','MatrixGatherWait','MatrixFinish'):
        old=f'#ifdef {FAST}\n timing.Next(PioneerQ38FastDecodeProfileData::{phase});'
        projections.append((old,old.replace('#ifdef '+FAST,f'#if defined({FAST}) && !defined({FLAG})',1)))
    overlay(frozen,here,'blackhole_qwen38_mixed_projection_root.cpp',projections)

    def start(phase):
        return f'#ifdef {FLAG}\n {{\n PioneerQ38FastDecodeProfileScope timing(PioneerQ38FastDecodeProfileData::{phase});\n#endif\n'

    end=f'#ifdef {FLAG}\n }}\n#endif\n'
    old=' StageConv(tracked); // actual complete predecessor,never a caller-supplied receipt\n'
    transition=f'#ifdef {FLAG}\n timing.Next(PioneerQ38FastDecodeProfileData::GdnStage);\n#endif\n'
    after=' StageGdn(tracked);\n#endif\n'
    overlay(frozen,here,'blackhole_qwen38_mixed_recurrent_cohort.cpp',
            [(old,start('ConvStage')+old+transition),(after,after+end)])
    first=' Joined::Prepare(p);Joined::BindDense(p);'
    last=' Joined::AppendTokenKv(p);\n#endif\n'
    overlay(frozen,here,'blackhole_qwen38_mixed_live_block_root.cpp',
            [(first,start('DenseAttention')+first),(last,last+end)])
    subprocess.run([sys.executable,original,sdk,control,out/'v4-diagnostic','--layer-split-inspection'],
                   cwd=out,check=True)
    validate(out,here)


def validate(out,here):
    tests=here/'tests'
    env=dict(os.environ,UBSAN_OPTIONS='halt_on_error=1')
    for version,image in ((3,'v3-control'),(4,'v4-diagnostic')):
        flags=['-D'+FLAG+'=1'] if version==4 else []
        for name in ('data','image'):
            test=out/f'v{version}-{name}-test'
            subprocess.run([os.getenv('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                            '-Wno-unused-function','-Wno-misleading-indentation','-fsanitize=address,undefined',
                            *flags,tests/f'pioneer_q38_fast_decode_profile_{name}_test.cpp','-o',test],
                           cwd=out,check=True)
            subprocess.run([test,*([out/image] if name=='image' else [])],
                           cwd=out,env=env,check=True,timeout=120)
        subprocess.run([sys.executable,tests/'pioneer_q38_fast_decode_profile_role_test.py',out/image],
                       cwd=out,check=True,timeout=60)


if __name__=='__main__':
    main()
