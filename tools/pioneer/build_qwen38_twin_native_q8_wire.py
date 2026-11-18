#!/usr/bin/env python3
"""Link the matched vector92/v4 native Q8 wire pair. No hardware or package output."""
from hashlib import sha256
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

from build_qwen38_twin_native_q8_worker import PINS as MATH_PINS
from generate_qwen38_token_programs import authenticated, pins

HERE = Path(__file__).resolve().parent
FLAG = 'PIONEER_Q38_TWIN_NATIVE_Q8_WIRE'
WIRE_ROOT_PINS = (
 ('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 ('text.bin',130960,'bbd7b4712c08aae8ff3584a4e8bd954427b70d98ec4246526b175b8f51af6611'),
 ('terminal.bin',8,'468eb84a74c067cc328b3fb3e51b768a8440cd67d292add55eef6861c9b24de2'),
 ('conv.bin',32216,'42627d7f451a8e734cc4b22f17dbb8b828fddb63b4a92c04cff5a9ed50733dd0'),
 ('fast_decode.bin',1496,'49cff44b420dda8d5b22eb9911b304d9ff8407cc7d4a9cb13cea9c7b883e7bf5'),
 ('program.elf',193672,'b5e5c5fea22d39e1120c4bb850da4c74f0666bc6e32284ebcab91fb3aebcd734'),
)
WIRE_WORKER_PINS = (
 ('boot.bin',4,'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c'),
 ('globalworker.bin',9532,'9b0ec239a371efc4aa11b5c1c09cdc0a6b326e9887dd540748467771b5d3fdd2'),
 ('precise.bin',8392,'c298cb8042bc8e58897c88a49ca2ef7e7d1ea947acb82106b3c6c79da9054d39'),
 ('dense.bin',10332,'bd667da41fb2fae0e9db85dafeef758ce1f9246faa736977353e84babfdd793b'),
 ('fetch.bin',1292,'a323fb1c314ac14c0267f2c1aabac440c2ee5eaa0f290c03bfc8e55d7ee94c57'),
 ('text.bin',8060,'d05e6e558b62bea82e4a1120898288ae66904c8419fef83863cc88b746ed66c0'),
 ('affine.bin',5020,'bae4b4ba7d48ce9ae753e86a7aa8bf08a113df06428798fd91f028a75c20f3e9'),
 ('ffn.bin',16456,'494a741ffae2aa77e0b33d06fa5894d6b8e0490f338ddf9b163b0825a1d01963'),
 ('service.bin',9888,'d7337714f91d34eb7f1539d39a590f37608149532476e3765cf6e5f765ad1eba'),
 ('gdn.bin',35944,'51fa2aed4158682284e3bf178f712e70d89e42af15a6393fece3e2e07dd039f2'),
 ('program.elf',144940,'fdfeeac324418dfd014a8ebe7caa2183d2d26e261f2db315c251049eb30b91ee'),
)


def insert(source, name, anchor, omit=''):
    """Apply just the tracked ifdef/else at this exact shared source offset."""
    tracked = (HERE/name).read_text()
    if tracked.count(anchor) != 1:
        raise ValueError((name, 'ambiguous tracked anchor', anchor))
    at = tracked.index(anchor)+len(anchor)
    selected = re.match(r'#if[^\n]*\b'+FLAG+r'\b', tracked[at:])
    assert selected, (name, anchor)
    level, end, fallback = 0, at, ''
    for line in tracked[at:].splitlines(keepends=True):
        end += len(line)
        if line.startswith('#if'):
            level += 1
        elif line.startswith('#endif'):
            level -= 1
            if not level:
                break
        elif line.startswith('#else') and level == 1:
            fallback = tracked[end:tracked.index('\n#endif',end)+1]
    assert not level
    path = source/name
    old = path.read_text()
    block=tracked[at:end]
    if omit:
        assert omit in fallback and block.count(omit)==2
        fallback=fallback.replace(omit,'');block=block.replace(omit,'')
    assert old.count(anchor+fallback)==1, (name, anchor)
    path.write_text(old.replace(anchor+fallback, anchor+block, 1))


def linked_sections(tool, image, limits, control):
    elf = image/'program.elf'
    assert not subprocess.check_output([tool+'nm','-u',elf]).strip()
    syms = subprocess.check_output([tool+'nm','-n',elf],text=True)
    assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ',syms,re.M)
    (image/'program.symbols').write_text(syms)
    report = {}
    for name, maximum in limits.items():
        file = image/(name+'.bin')
        subprocess.run([tool+'objcopy','-O','binary','-j','.'+name,elf,file],check=True)
        data = file.read_bytes()
        assert 0<len(data)<=maximum and len(data)%4==0, name
        if name in ('boot','terminal'):
            assert data == control[file.name], name
        if name in ('dense','precise'):
            # Same original link inputs; instructions calling the relocated
            # global front can change their immediate, not their footprint.
            assert len(data)==len(control[file.name]), name
        report[file.name] = (len(data),sha256(data).hexdigest())
    report[elf.name] = (elf.stat().st_size,sha256(elf.read_bytes()).hexdigest())
    print(image.name, report, flush=True)


def test_root(tool, out, candidate, control):
    original=out/'root-control'
    for name in ('text.bin','conv.bin'):
        (original/name).write_bytes(control[name])
    mappings=[]
    for image in (original,candidate):
        table=subprocess.check_output([tool+'nm','-n',image/'program.elf'],text=True)
        addresses=[]
        for name,count in (('_ZN28PioneerQ38PushedWorkerLayout9BootShapeEPKj',1),
                           ('_ZN28PioneerQ38PushedWorkerLayout8JobShapeEPKjS1_jj',1),
                           ('_ZN10ProjectionL9MatrixJobEPKjjj',2),
                           ('_ZN28PioneerQ38PushedWorkerLayout7ReceiptEPKjS1_S1_S1_',1)):
            entries=re.findall(r'^([0-9a-f]+) [tTW] '+re.escape(name)+r'$',table,re.M)
            assert len(entries)==count,(name,entries)
            addresses+=entries
        mapping=image/'test.entries';mapping.write_text(' '.join(addresses)+'\n');mappings.append(mapping)
    unit=out/'root-test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',HERE/'tests/pioneer_q38_twin_native_q8_root_test.cpp','-o',unit],check=True)
    subprocess.run([unit,original,candidate,*mappings],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})


def main():
    if len(sys.argv)!=6:
        raise SystemExit('use SDK V4_ROOT VECTOR92_BUILD ISOLATED_NATIVE_Q8_BUILD NEW_OUTPUT')
    assert __debug__ and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
    sdk,root,vector,math,out=map(Path,sys.argv[1:])
    assert all(p.is_absolute() for p in (sdk,root,vector,math,out)) and not out.exists() and not out.is_symlink()
    names=('recurrent','dense','token')
    roots={k:(root/f'compile-{k}-argv.txt').read_text().splitlines() for k in names}
    frozen=Path(roots['token'][roots['token'].index('-c')+1]).parent
    assert frozen.name=='pioneer' and frozen.parent.name=='tools'
    original=authenticated(root,pins(HERE/'build_qwen38_fast_decode_profile_root.py','SPLIT_PINS'))
    raw=authenticated(vector,pins(frozen/'build_qwen38_vector_q4_metadata_worker.py','PINS'))
    isolated=authenticated(math,MATH_PINS)
    tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
    wire_root,wire_worker=out/'root',out/'worker'
    out.mkdir(mode=0o700);wire_root.mkdir();wire_worker.mkdir()
    commands=[]

    def run(args):
        args=list(map(str,args));assert args[0] in (tool+'g++',tool+'ld',tool+'objcopy')
        commands.append(args);subprocess.run(args,cwd=out,check=True)

    def option(argv,opt,value):
        assert argv.count(opt)==1, opt
        argv[argv.index(opt)+1]=str(value)

    def compile(argv,source,target,enabled):
        args=argv[:]
        if enabled:args.insert(1,'-D'+FLAG+'=1')
        for at,item in enumerate(args[:-1]):
            if item in ('-c','-include'):
                args[at+1]=str(source/Path(args[at+1]).name)
        if '-MF' in args:option(args,'-MF',target.with_suffix('.d'))
        option(args,'-o',target);run(args)
        frames=[row.split('\t') for row in target.with_suffix('.su').read_text().splitlines()]
        assert frames and all(len(r)==3 and r[2]=='static' and int(r[1])<=4096 for r in frames)

    source=out/'source/tools/pioneer'
    shutil.copytree(frozen,source)
    shutil.copytree(frozen.parents[1]/'src/add-ons/kernel/drivers/misc',out/'source/src/add-ons/kernel/drivers/misc')
    shutil.copyfile(HERE/'PioneerQ38TwinNativeQ8Wire.h',source/'PioneerQ38TwinNativeQ8Wire.h')
    both=source/'PioneerQ38PushedWorkerLayout.h'
    # Pushed and Flat have no unrelated changes from the frozen source.
    for name in (both.name,'PioneerQ38FlatMatrixGrammar.h'):
        shutil.copyfile(HERE/name,source/name)
    select='namespace PioneerQ38SelectedGlobalWorker=PioneerQ38PushedWorkerLayout;\n'
    insert(source,'PioneerQ38SelectedGlobalWorker.h',select,',PioneerQ38SelectedWorkerHybrid=0')
    for anchor in ('#include "PioneerQ38ProjectionInput.h"\n',
                   ' for(unsigned i=0;i<10;i++)Words(L::Scratch)[16+i]=job[i];\n',
                   ' for(unsigned i=0;i<16;i++)if(proof[32+i]!=Boot(w)[16+i])scalar_fault_park();\n'):
        insert(source,'blackhole_qwen38_mixed_projection_root.cpp',anchor)
    for anchor in ('#include "PioneerQ38TokenBatchedPrefetchRole.h"\n',
                   'static void Matrix(const uint32_t* j,const uint32_t* source,unsigned worker,unsigned self){\n',
                   ' unsigned reads=j[7]*\n#ifdef PIONEER_Q38_TOKEN_BATCHED_PREFETCH_WORKER_IMAGE\n',
                   ' p[30]=source[0];p[31]=self;for(unsigned i=0;i<16;i++)p[32+i]=source[i];\n'):
        insert(source,'blackhole_qwen38_global_worker.cpp',anchor)

    bats=vector/'unchanged-wide/unchanged-twin/unchanged-batched'
    sender=(bats/'compile-global-argv.txt').read_text().splitlines()
    worker_link=(math/'linked-argv.txt').read_text().splitlines()[1].split('\t')
    root_args=[line.split('\t') for line in (root/'build-argv.txt').read_text().splitlines()]
    root_steps=[a for a in root_args if a[-1] in (str(root/'components-raw.o'),str(root/'components-NOBOOT.o'))
                or a[0]==tool+'ld' and '-o' in a and a[a.index('-o')+1]==str(root/'program.elf')]
    assert [Path(a[0]).name.rsplit('-',1)[1] for a in root_steps]==['ld','objcopy','ld']
    for enabled in (False,True):
        dst=wire_root if enabled else out/'root-control';dst.mkdir(exist_ok=True)
        for k in names:
            compile(roots[k],source,dst/f'fast-{k}.o',enabled)
            if not enabled:assert (dst/f'fast-{k}.o').read_bytes()==(root/f'fast-{k}.o').read_bytes()
        substitutions={root/n:dst/n for n in ('fast-recurrent.o','fast-dense.o','fast-token.o',
                        'components-raw.o','components-NOBOOT.o','program.elf','program.map')}
        for args in root_steps:run([substitutions.get(Path(p),p) for p in args])
        if not enabled:assert (dst/'program.elf').read_bytes()==original['program.elf']
        else:linked_sections(tool,dst,{'boot':4,'text':0x20000,'terminal':8,'conv':0x8000,'fast_decode':0x2000},original)

        dst=wire_worker if enabled else out/'worker-control';dst.mkdir(exist_ok=True)
        front=dst/'batched-global.o';compile(sender,source,front,enabled)
        if not enabled:assert front.read_bytes()==(bats/front.name).read_bytes()
        assert worker_link.count(str(bats/front.name))==1
        args=[str(front) if p==str(bats/front.name) else p for p in worker_link]
        for key,value in (('-o',dst/'program.elf'),('-Map',dst/'program.map')):option(args,key,value)
        run(args)
        if not enabled:assert (dst/'program.elf').read_bytes()==isolated['program.elf'] and raw['boot.bin']==isolated['boot.bin']
        else:linked_sections(tool,dst,{'boot':4,'globalworker':0x10000,'precise':0x10000,'dense':0x10000,
             'fetch':0x700,'text':0x2000,'affine':0x1500,'ffn':0x6000,'service':0x6000,'gdn':0x10000},isolated)
    authenticated(wire_root,WIRE_ROOT_PINS);authenticated(wire_worker,WIRE_WORKER_PINS)
    test_root(tool,out,wire_root,original)
    (out/'build-argv.txt').write_text('\n'.join('\t'.join(x) for x in commands)+'\n')
    print('BUILD_ONLY native Q8 ROOT/worker wire pair; old control bytes reproduced; NO loader or execution')


if __name__=='__main__':
    main()
