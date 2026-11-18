#!/usr/bin/env python3
"""Relink one recipe sender against the exact v4 profiling ROOT. Offline."""
from hashlib import sha256
import os
from pathlib import Path
import shutil
import subprocess
import sys

V4='7f2f2d4895c72c3278ce83676be5b9a122e0a074d319b76b86586611630627af'
ROLE='PIONEER_Q38_TOKEN_RECIPE_BATCH_IMAGE'
RECIPE_PINS=(
    ('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
    ('text.bin',130908,'89319b056a4c7a6efbba256dbd7ded5aaef89e65c1ba96e1d0c09caa927e1fd9'),
    ('terminal.bin',8,'468eb84a74c067cc328b3fb3e51b768a8440cd67d292add55eef6861c9b24de2'),
    ('conv.bin',32512,'d3ab9d353cf0e8849f93a4be20b2c1faccae673a8334b401d319e6e5ad674372'),
    ('fast_decode.bin',1496,'49cff44b420dda8d5b22eb9911b304d9ff8407cc7d4a9cb13cea9c7b883e7bf5'),
    ('program.elf',193536,'ee6e0765744b935bc299e597e1b10859aa944e9114b9bbed915e135cb299be29'),
)

def main():
    assert __debug__ and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
    if len(sys.argv)!=4:
        raise SystemExit('use SDK PINNED_V4_ROOT NEW_OUTPUT')
    sdk,control,out=map(Path,sys.argv[1:])
    assert all(p.is_absolute() for p in (sdk,control,out)) and not out.exists() and not out.is_symlink()
    here=Path(__file__).resolve().parent
    tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
    old=(control/'program.elf').read_bytes()
    assert len(old)==193672 and sha256(old).hexdigest()==V4
    args=(control/'compile-token-argv.txt').read_text().splitlines()
    assert args[0]==tool+'g++' and args.count('-c')==args.count('-o')==args.count('-MF')==1
    assert all('-D'+f+'=1' in args for f in ('PIONEER_Q38_LAYER_SPLIT_PROFILE_IMAGE',
                'PIONEER_Q38_FAST_DECODE_PROFILE_IMAGE','PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE'))
    source=Path(args[args.index('-c')+1]);original=control/'fast-token.o'
    assert source.name=='blackhole_qwen38_token_root.cpp' and args[args.index('-o')+1]==str(original)
    link=[line.split('\t') for line in (control/'build-argv.txt').read_text().splitlines()
          if line.startswith(tool+'ld\t') and line.endswith('\t'+str(control/'program.elf'))]
    assert len(link)==1 and link[0].count(str(original))==1
    link=link[0]
    assert link[link.index('-T')+1]==str(control/'program.ld')
    out.mkdir(mode=0o700)
    overlay=out/'overlay';overlay.mkdir(mode=0o700)
    metadata='blackhole_qwen38_token_metadata_control.inc'
    recipe='static __attribute__((noinline,section(".conv.token"))) void TokenRecipes('
    following='static __attribute__((noinline,section(".conv.token"))) void Metadata('
    frozen=(source.parent/metadata).read_text()
    current=(here/metadata).read_text()
    assert frozen.count(recipe)==current.count(recipe)==frozen.count(following)==current.count(following)==1
    # Only replace TokenRecipes. Later workspace revisions include an
    # unrelated GDN redistribution and must not leak into this v4 arm.
    recipe_begin='#ifdef '+ROLE+'\n'
    chunk=current[current.index(recipe_begin):current.index(following)]
    (overlay/metadata).write_text(frozen[:frozen.index(recipe)]+chunk+frozen[frozen.index(following):])
    shutil.copyfile(here/'PioneerQ38TokenRecipeBatch.h',overlay/'PioneerQ38TokenRecipeBatch.h')
    shutil.copyfile(source,overlay/source.name)
    def run(*command,**kw):
        return subprocess.run(command,cwd=out,check=True,**kw)
    def compile_token(filename,flags):
        obj=out/filename
        command=args[:]
        command[command.index('-c')+1]=str(overlay/source.name)
        command[command.index('-o')+1]=str(obj)
        command[command.index('-MF')+1]=str(obj.with_suffix('.d'))
        command[1:1]=['-I'+str(source.parent),*flags]
        run(*command)
        return obj,command
    def link_root(obj,name):
        elf=out/name
        command=[str(obj) if a==str(original) else a for a in link]
        command[command.index('-o')+1]=str(elf)
        command[command.index('-Map')+1]=str(elf.with_suffix('.map'))
        run(*command)
        return elf,command
    same,_=compile_token('unchanged-token.o',[])
    assert same.read_bytes()==original.read_bytes(),'disabled recipe policy must preserve original object'
    control_elf,_=link_root(same,'control.elf')
    assert control_elf.read_bytes()==old,'entire original v4 ELF first'
    new,compile_args=compile_token('recipe-batch.o',['-D'+ROLE+'=1'])
    for extra in (['-U'+ROLE,'-D'+ROLE+'=0'],
                  ['-UPIONEER_Q38_LAYER_SPLIT_PROFILE_IMAGE'],
                  ['-DPIONEER_Q38_PREPARED_Q4_ROOT_IMAGE=1']):
        refused=subprocess.run([*compile_args[:-4],*extra,'-E',overlay/source.name],cwd=out,
                               stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
        assert refused.returncode and 'Exact v4 layer recipe batching inspection role required' in refused.stderr, extra
    elf,link_args=link_root(new,'program.elf')
    assert not run(tool+'nm','-u',elf,capture_output=True).stdout
    for section,limit in (('boot',4),('terminal',8),('fast_decode',8192),('text',131072),('conv',32768)):
        raw=out/(section+'.bin')
        run(tool+'objcopy','-O','binary','-j','.'+section,elf,raw)
        data=raw.read_bytes()
        assert data and len(data)<=limit
        if section in ('boot','terminal','fast_decode'):
            assert data==(control/raw.name).read_bytes(),section
        print(section,len(data),sha256(data).hexdigest())
    symbols=run(tool+'nm','-S','-n','-C',elf,capture_output=True,text=True).stdout
    assert symbols.count('GlobalChain::TokenRecipes(')==1 and 'GlobalChain::Transfer(' in symbols
    frames=[row.split('\t') for row in new.with_suffix('.su').read_text().splitlines() if '::TokenRecipes(' in row]
    assert len(frames)==1 and frames[0][2]=='static' and int(frames[0][1])<=128
    (out/'program.symbols').write_text(symbols)
    (out/'linked-argv.txt').write_text('\n'.join('\t'.join(map(str,c)) for c in (compile_args,link_args))+'\n')
    print(elf.name,elf.stat().st_size,sha256(elf.read_bytes()).hexdigest())
    print('PASS linked v4 recipe-batch inspection; unchanged original ROOT, boot, STOP and timer; three role refusals; NO native or rate claim')

if __name__=='__main__':
    main()
