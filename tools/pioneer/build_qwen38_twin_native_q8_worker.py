#!/usr/bin/env python3
"""Link an isolated signed-magnitude Q8 math arm, never install or execute."""
from hashlib import sha256
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

from generate_qwen38_token_programs import authenticated, pins

ROLE='PIONEER_Q38_TWIN_NATIVE_Q8_IMAGE'
CONTROL='4b3dc0c96d3888b9c43af683f58678cc91cade7c67e955413bad8c5e16d52870'
PINS=(
    ('boot.bin',4,'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c'),
    ('globalworker.bin',9232,'3b0c0c804f20652821004ffee2afd581f9cfd57ce8a08d49ae6583a131895d3b'),
    ('precise.bin',8392,'32c078029e1c74ec4bd9744ea494433f5003f63a936df05445d3066c722a7ecc'),
    ('dense.bin',10332,'d1749707ffe803e28dced9c7d065e763f125009678ace33b4d468109984869ad'),
    ('fetch.bin',1292,'acb094c917904a8735d9f511e808df336847e05886e0e9992e6de3da69643d51'),
    ('text.bin',8060,'d71c868f308a50d2f116f586aeb92ed8d65c6e52adef8192931a53f513633e25'),
    ('affine.bin',5020,'bae4b4ba7d48ce9ae753e86a7aa8bf08a113df06428798fd91f028a75c20f3e9'),
    ('ffn.bin',16456,'f88f5725fab5883f3133d744af59703bf2ed8a35378ddd2a0acc6eddaa73d6fe'),
    ('service.bin',9888,'2e874f9910cf2564a4d45f4b3a787707ce12aa6bcf550e560cc3484562653a57'),
    ('gdn.bin',35944,'1c1e0b64e0db85f2b886a330bb9490b394e52c595473908f48097a3f887ae7a0'),
    ('program.elf',144940,'98f144db448e42cee2f1c3dcad043bc65bb6ff94aee691b88b4bcba35883597d'),
)


def main():
    if len(sys.argv)!=4:
        raise SystemExit('use SDK COMPLETE_PINNED_VECTOR92_BUILD NEW_OUTPUT')
    assert __debug__ and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
    sdk,control,out=map(Path,sys.argv[1:])
    assert all(p.is_absolute() for p in (sdk,control,out)) and not out.exists() and not out.is_symlink()
    here=Path(__file__).resolve().parent
    tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
    original=(control/'program.elf').read_bytes()
    assert len(original)==144592 and sha256(original).hexdigest()==CONTROL
    args=(control/'compile-math-argv.txt').read_text().splitlines()
    assert args[0]==tool+'g++' and args.count('-c')==args.count('-o')==1
    old=control/'vector-metadata-program.o'
    source=Path(args[args.index('-c')+1]);base=source.parent
    assert args[args.index('-o')+1]==str(old) and source.name=='blackhole_qwen38_vector_q4_metadata_math.cpp'
    assert '-DPIONEER_Q38_VECTOR_Q4_METADATA_IMAGE=1' in args and all(ROLE not in a for a in args)
    reference=authenticated(control,pins(base/'build_qwen38_vector_q4_metadata_worker.py','PINS'))
    assert reference['program.elf']==original
    command=(control/'link-argv.txt').read_text().splitlines()
    assert command[0]==tool+'ld' and command.count(str(old))==1
    assert command[command.index('-T')+1]==str(control/'program.ld')
    assert command[command.index('-o')+1]==str(control/'program.elf')
    out.mkdir(mode=0o700)
    overlay=out/'overlay';overlay.mkdir(mode=0o700)
    for name in (source.name,'blackhole_qwen38_twin_batch_prefetch_math.cpp','blackhole_qwen38_resident_operator_service.cpp'):
        shutil.copyfile(base/name,overlay/name)
    for name in ('blackhole_qwen38_rows.cpp','PioneerQ38SfpuAccumulator.h','PioneerQ38TwinNativeQ8.h'):
        shutil.copyfile(here/name,overlay/name)

    def run(*argv,**kw):
        return subprocess.run(argv,cwd=out,check=True,**kw)

    def compile_math(name,flags):
        obj=out/name
        a=[args[0],'-I'+str(base),*flags,*args[1:]]
        a[a.index('-c')+1]=str(overlay/source.name)
        a[a.index('-o')+1]=str(obj)
        run(*a)
        return obj,a

    def link(obj,script,name):
        elf=out/name
        a=[str(obj) if arg==str(old) else arg for arg in command]
        for option,value in (('-T',script),('-Map',elf.with_suffix('.map')),('-o',elf)):
            a[a.index(option)+1]=str(value)
        run(*a)
        return elf,a

    same,_=compile_math('original-math.o',[])
    assert same.read_bytes()==old.read_bytes(),'disabled candidate must preserve exact original math object'
    original_elf,_=link(same,control/'program.ld','original.elf')
    assert original_elf.read_bytes()==original,'complete original linked worker first'
    candidate,compile_args=compile_math('native-math.o',['-D'+ROLE+'=1'])
    for extra in (['-U'+ROLE,'-D'+ROLE+'=0'],['-UPIONEER_Q38_WORK_DIRECT_SRCB'],
                  ['-DPIONEER_Q38_NATIVE_Q8=1'],['-DPIONEER_Q38_CPU_VALIDATION=1']):
        neg=subprocess.run([*compile_args[:compile_args.index('-c')],*extra,'-E',overlay/source.name],cwd=out,
                           stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
        assert neg.returncode and 'Exact separate vector92 twin native Q8 inspection worker required' in neg.stderr,extra
    script=(control/'program.ld').read_text()
    slot='\t\t*(.q38_projection_data)'
    assert script.count(slot)==1
    # Relocate the new leafs and two complete small helpers into the existing
    # immutable affine prefix. Keep the original text and data address caps.
    script=script.replace(slot,slot+'\n\t\t*(.text.pioneer_q38_twin_native)'
        '\n\t\t*(.text._Z24PioneerQ38VectorAffineI18VectorPanelEmitterE*)'
        '\n\t\t*(.text._Z17PioneerSfpuLoad32I18VectorPanelEmitterE*)')
    script_path=out/'program.ld';script_path.write_text(script)
    elf,link_args=link(candidate,script_path,'program.elf')
    assert not run(tool+'nm','-u',elf,capture_output=True).stdout
    names=run(tool+'nm','-nC',elf,capture_output=True,text=True).stdout
    assert all(n in names for n in ('NativeWeightPanel(unsigned int, unsigned int)','NativeFormat(bool)','q38_compute_once'))
    for section,limit in (('boot',4),('globalworker',0x10000),('precise',0x10000),('dense',0x10000),
                          ('fetch',0x700),('text',0x2000),('affine',0x1500),('ffn',0x6000),
                          ('service',0x6000),('gdn',0x10000)):
        raw=out/(section+'.bin')
        run(tool+'objcopy','-O','binary','-j','.'+section,elf,raw)
        data=raw.read_bytes()
        assert data and len(data)<=limit
        print(section,len(data),sha256(data).hexdigest(),flush=True)
        if section in ('boot','precise','dense'):
            assert data==reference[raw.name],section
    dis=run(tool+'objdump','-d',elf,capture_output=True,text=True).stdout
    assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fmadd|fmsub|fnmadd|fnmsub|ebreak)\b',dis)
    frames=[row.split('\t') for row in candidate.with_suffix('.su').read_text().splitlines()]
    assert frames and all(len(r)==3 and r[2]=='static' and int(r[1])<=4096 for r in frames)
    authenticated(out,PINS)
    # Exercise the pinned RV32 leafs/format refusals; do not run firmware on
    # hardware or supply a fabricated native Q8 result/lease.
    entries=[]
    for symbol in ('t NativeFormat(bool)','t NativeWeightPanel(unsigned int, unsigned int)',
                   'T q38_compute_once','T block_fault_park'):
        match=re.findall(r'^([0-9a-f]+) '+re.escape(symbol)+r'$',names,re.M)
        assert len(match)==1,symbol
        entries.append(match[0])
    unit=out/'linked-panel-test'
    run(os.environ.get('CXX','c++'),'-std=c++17','-O2','-g','-Wall','-Wextra','-Werror',
        '-fsanitize=address,undefined',here/'tests/pioneer_q38_linked_native_q8_panel_test.cpp','-o',unit)
    run(unit,out,*entries,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
    (out/'program.symbols').write_bytes(run(tool+'nm','-n',elf,capture_output=True).stdout)
    (out/'linked-argv.txt').write_text('\n'.join('\t'.join(map(str,c)) for c in (compile_args,link_args))+'\n')
    print(elf.name,elf.stat().st_size,sha256(elf.read_bytes()).hexdigest(),flush=True)
    print('PASS BUILD_ONLY exact vector92 original first; separate private Q8 math branch; no ROOT/source/worker admission, device or rate claim')


if __name__=='__main__':
    main()
